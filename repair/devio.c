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
