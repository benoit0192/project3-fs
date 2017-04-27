#include "devio.h"

void dev_open() {
    if( (device = open(device_file, repair ? O_RDWR : O_RDONLY)) < 0 )
        die("Can't open device %s: %s", device_file, strerror(errno));
}

void rw_superblock(int rw_flag) {
    if(lseek(device, SUPER_BLOCK_BYTES, SEEK_SET) < 0)
        die("rw_superblock: can't seek superblock: %s", strerror(errno));
    if(rw_flag == READ) {
        if( read(device, &sb, sizeof(sb)) != sizeof(sb) )
            die("rw_superblock: can't read superblock: %s", strerror(errno));
         // it is not on disk, but it will be useful
        sb.s_firstdatazone = (BLK_ILIST + N_ILIST + BLOCK_PER_ZONE - 1) >> sb.s_log_zone_size;
    } else if(rw_flag == WRITE) {
        if( write(device, &sb, sizeof(sb)) != sizeof(sb) )
            die("rw_superblock: can't write superblock: %s", strerror(errno));
    } else {
        die("rw_superblock: invalid rw_flag");
    }
}

/* load in 'b' the bitmap starting at block 'start' and 'size' blocks long if 'rw_flag is READ'*/
/* write bitmap 'b' on disk at starting block 'start', and 'size' blocks long if 'rw_flag' is WRITE */
void rw_bitmap(bitchunk_t *b, block_t start, size_t size, int rw_flag) {
    off_t bitmap_start = block2byte(start);
    if( lseek(device, bitmap_start, SEEK_SET) != bitmap_start)
        die("Can't seek bitmap start");
    size_t bitmap_len = block2byte(size);
    if(rw_flag == READ) {
        if( read(device, b, bitmap_len) != bitmap_len )
            die("Can't read bitmap");
    } else {
        if( write(device, b, bitmap_len) != bitmap_len )
            die("Can't write bitmap");
    }
}

/* l must always be allocated. if list is empty, pass a dirent_list structure. first list element will never be used */
struct dirent_list *iterate_dir_entries(struct dirent_list *l, zone_t zones[], int nr_elts, int level) {
    for(int z = 0; z < nr_elts; ++z) {
        if(level == 0 && zones[z]) {
            // direct zone. we can directly read entries
            int zone_offset = block2byte(zone2block(zones[z]));
            if(lseek(device, zone_offset, SEEK_SET) != zone_offset)
                die("iterate_dir_entries: can't seek directory zone: %s", strerror(errno));
            struct direct dir;
            for(int d = 0; d < (BLOCK_PER_ZONE * sb.s_block_size) / sizeof(dir); ++d) {
                if(read(device, &dir, sizeof(dir)) != sizeof(dir))
                    die("iterate_dir_entries: can't read directory zone: %s", strerror(errno));
                if(dir.mfs_d_ino /* entry is not null? */) {
                    // add it to the list
                    l->next = malloc(sizeof(*l));
                    if(!(l->next))
                        die("iterate_dir_entries: can't allocate memory");
                    l = l->next;
                    l->next = NULL;
                    l->ino = dir.mfs_d_ino;
                    for(int k = 0; k < MFS_DIRSIZ; ++k)
                        l->name[k] = dir.mfs_d_name[k];
                } else {
                    //break; // NO! there are empty entries in a directory file. we must read the whole file.
                }
            }
        } else if(level > 0 && zones[z]) {
            zone_t ind_zones[sb.s_block_size / sizeof(zone_t)];
            for(int b = 0; b < BLOCK_PER_ZONE; ++b) {
                int ind_zone_offset = block2byte(zone2block(zones[z])+b);
                if(lseek(device, ind_zone_offset, SEEK_SET) != ind_zone_offset)
                    die("iterate_dir_entries: can't seek indirect zone: %s", strerror(errno));
                if(read(device, ind_zones, sb.s_block_size) != sb.s_block_size)
                    die("iterate_dir_entries: can't read indirect zone: %s", strerror(errno));
                iterate_dir_entries(l, ind_zones, sb.s_block_size / sizeof(zone_t), level-1);
            }
        }
    }
    return l;
}

struct dirent_list *get_dir_entries(struct inode *in) {
    if((in->i_mode & I_TYPE) != I_DIRECTORY) {
        printf("get_dir_entries: not a directory inode\n");
        return NULL;
    }
    struct dirent_list *l = malloc(sizeof(*l));
    if(!l)
        die("get_dir_entries: cannot allocate memory");
    struct dirent_list *tmp;
    tmp = iterate_dir_entries(l, in->i_zone, V2_NR_DZONES, 0); // direct zones
    tmp = iterate_dir_entries(tmp, &(in->i_zone[V2_NR_DZONES]), 1, 1); // simple indirect
    iterate_dir_entries(tmp, &(in->i_zone[V2_NR_DZONES+1]), 1, 2); // double indirect
    tmp = l->next; // skip unused first element
    free(l);
    return tmp;
}

struct dirent_list *flush_dir_entries_zone(struct dirent_list *l, zone_t zones[], int nr_elts, int level) {
    for(int z = 0; z < nr_elts; ++z) {
        if(l && !zones[z]) {
            // not enough zones allocated to fit all entries. allocate a new one
            die("data zone allocation for directory entries not implemented yet.\nfilesystem might be damaged");
        }
        // now we are sure that there is an allocated zone here
        if(level == 0 && zones[z]) {
            // direct zone. we can directly write entries
            int zone_offset = block2byte(zone2block(zones[z]));
            if(lseek(device, zone_offset, SEEK_SET) != zone_offset)
                die("flush_dir_entries_zone: can't seek directory zone: %s", strerror(errno));
            struct direct dir;
            for(int d = 0; d < (BLOCK_PER_ZONE * sb.s_block_size) / sizeof(dir); ++d) {
                if(l) {
                    dir.mfs_d_ino = l->ino;
                    for(int k = 0; k < MFS_DIRSIZ; ++k)
                        dir.mfs_d_name[k] = l->name[k];
                    l = l->next;
                } else {
                    // clear all entries
                    dir.mfs_d_ino = 0;
                    for(int k = 0; k < MFS_DIRSIZ; ++k)
                        dir.mfs_d_name[k] = 0;
                }
                if(write(device, &dir, sizeof(dir)) != sizeof(dir))
                    die("flush_dir_entries_zone: can't write directory zone: %s", strerror(errno));
            }
        } else if(level > 0 && zones[z]) {
            zone_t ind_zones[sb.s_block_size / sizeof(zone_t)];
            for(int b = 0; b < BLOCK_PER_ZONE; ++b) {
                int ind_zone_offset = block2byte(zone2block(zones[z])+b);
                if(lseek(device, ind_zone_offset, SEEK_SET) != ind_zone_offset)
                    die("flush_dir_entries_zone: can't seek indirect zone: %s", strerror(errno));
                if(read(device, ind_zones, sb.s_block_size) != sb.s_block_size)
                    die("flush_dir_entries_zone: can't read indirect zone: %s", strerror(errno));
                l = flush_dir_entries_zone(l, ind_zones, sb.s_block_size / sizeof(zone_t), level-1);
            }
        }
    }
    return l;
}

void flush_dir_entries(ino_t i, struct inode *in, struct dirent_list *l) {
    struct dirent_list *tmp = flush_dir_entries_zone(l, in->i_zone, V2_NR_DZONES, 0);
    tmp = flush_dir_entries_zone(tmp, &(in->i_zone[V2_NR_DZONES]), 1, 1);
    flush_dir_entries_zone(tmp, &(in->i_zone[V2_NR_DZONES+1]), 1, 2);
}

void free_dirent_list(struct dirent_list *l) {
    while(l) {
        struct dirent_list *tmp = l->next;
        free(l);
        l = tmp;
    }
}

int make_lost_found() {
    char name[MFS_DIRSIZ] = "lost+found";
    // get root inode
    int inode_offset = block2byte(BLK_ILIST);
    if(lseek(device, inode_offset, SEEK_SET) != inode_offset)
        die("make_lost_found: can't seek inode list: %s", strerror(errno));
    struct inode in;
    if(read(device, &in, INODE_SIZE) != INODE_SIZE)
        die("make_lost_found: can't read root inode: %s", strerror(errno));
    struct dirent_list *l = get_dir_entries(&in);
    int lost_found_inode = 0;
    for(struct dirent_list *it = l; it; it = it->next) {
        if(!strcmp(it->name, name)) {
            lost_found_inode = it->ino;
            break;
        }
    }
    if(!lost_found_inode) die("please create lost+found");
    /* we stop here for now */
    if(!lost_found_inode) {
        struct dirent_list *new = malloc(sizeof(*new));
        if(!new) die("make_lost_found: can't allocate memory");
        for(int k = 0; k < MFS_DIRSIZ; ++k)
            new->name[k] = name[k];
        lost_found_inode = 0; // TODO allocate inode
        new->ino = lost_found_inode;
        new->next = l;
        l = new;
        flush_dir_entries(1, &in, l);
    }
    free_dirent_list(l);
    return lost_found_inode;
}
