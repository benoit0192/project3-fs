#include "utils.h"

int is_block_device(char *filename) {
    struct stat fstat;
    if(stat(filename, &fstat) < 0)
        die("Can't stat device %s: %s", filename, strerror(errno));
    return S_ISBLK(fstat.st_mode);
}

char *get_mountpoint(char *device) {
    char special[PATH_MAX + 1], mounted_on[PATH_MAX + 1];
	char version[MNTNAMELEN], rw_flag[MNTFLAGLEN];
    if(load_mtab("recovery.mfs") < 0) exit(EXIT_FAILURE);
	while(get_mtab_entry(special, mounted_on, version, rw_flag) == 0) {
		if(!strcmp(special, device))
            return strdup(mounted_on);
	}
    return NULL;
}

void die(const char *fmt, ...) {
    fprintf(stderr, " *** fatal error *** \n");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void empty_stdin() {
    char c;
    while((c = getchar()) != '\n' && c != EOF);
}

char ask(char *question) {
    printf("%s\n ", question);
    char a = getchar();
    empty_stdin();
    return a;
}

bitchunk_t *alloc_bitmap(int nb_blocks) {
    bitchunk_t *b;
    size_t n = nb_blocks * sb.s_block_size;
    if( !(b = malloc(n)) )
        die("Tried to allocate %dkB: not enough memory", n/1024);
    memset(b, 0, n);
    return b;
}

void iterate_zones(bitchunk_t *bitmap, zone_t zones[], int nr_elts, int level) {
    for(int z = 0; z < nr_elts; ++z) {
        if(zones[z])
            bit_set(bitmap, zones[z] - sb.s_firstdatazone + 1, 1);
        if(level > 0 && zones[z]) {
            zone_t ind_zones[sb.s_block_size / sizeof(zone_t)];
            for(int b = 0; b < BLOCK_PER_ZONE; ++b) {
                int ind_zone_offset = block2byte(zone2block(zones[z])+b);
                if(lseek(device, ind_zone_offset, SEEK_SET) != ind_zone_offset)
                    die("iterate_zones: can't seek indirect zone: %s", strerror(errno));
                if(read(device, ind_zones, sb.s_block_size) != sb.s_block_size)
                    die("iterate_zones: can't read indirect zone: %s", strerror(errno));
                iterate_zones(bitmap, ind_zones, sb.s_block_size / sizeof(zone_t), level-1);
            }
        }
    }
}
