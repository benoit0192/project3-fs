#include "check.h"


void print_superblock() {
    printf("\n ======= SUPER BLOCK =======\n");
    printf("ninodes       = %u\n", sb.s_ninodes);
    printf("nzones        = %d\n", sb.s_zones);
    printf("imap_blocks   = %u\n", sb.s_imap_blocks);
    printf("zmap_blocks   = %u\n", sb.s_zmap_blocks);
    printf("firstdatazone = %u\n", sb.s_firstdatazone_old);
    printf("log_zone_size = %u\n", sb.s_log_zone_size);
    printf("maxsize       = %d\n", sb.s_max_size);
    printf("block size    = %d\n", sb.s_block_size);
    printf("magic         = %04x\n\n", sb.s_magic);
}

void check_superblock() {
    if(sb.s_magic != SUPER_V3) {
        printf("check_superblock: wrong magic number\n");
        if(repair) {
            if(ask("Do you want to overwrite it with a Minix v3 FS magic number? [y/n]") == 'y') {
                sb.s_magic = SUPER_V3;
                modified = 1;
                printf("Magic overwritten\n");
            } else {
                die("Ok, I won't change it. Can't go further.");
            }
        } else {
            die("check_superblock: wrong magic number. Can't go further.");
        }
    }
    /* check imap size consistency */
    uint imap_blocks = div_ceil(sb.s_ninodes, sb.s_block_size * 8);
    if(sb.s_imap_blocks < imap_blocks)
        die("check_superblock: imap is said to be %d blocks long, but at least %d are required to store all inodes.", sb.s_imap_blocks, imap_blocks);
    if(sb.s_imap_blocks > imap_blocks)
        printf("check_superblock: superblock says there is %d blocks for imap, but %d are expected\n", sb.s_imap_blocks, imap_blocks);
    /* check zmap size consistency */
    int zmap_blocks = div_ceil(sb.s_zones, sb.s_block_size * 8);
    if(sb.s_zmap_blocks < zmap_blocks)
        die("check_superblock: imap is said to be %d blocks long, but at least %d are required to store all zones.", sb.s_imap_blocks, zmap_blocks);
    if(sb.s_zmap_blocks > zmap_blocks)
        printf("check_superblock: superblock says there is %d blocks for zmap, but %d are expected\n", sb.s_imap_blocks, zmap_blocks);

    if(modified) {
        rw_superblock(WRITE);
        modified = 0;
    }
}

void check_imap() {
    printf("Checking imap...\n");
    int err_count = 0;
    int fix_count = 0;

    // load imap
    bitchunk_t *imap = alloc_bitmap(N_IMAP);
    rw_bitmap(imap, BLK_IMAP, N_IMAP, READ);

    // first bit of imap should always be 1
    if( !bit_at(imap, 0) ) {
        printf("check_imap: the very first bit of the imap should always be 1. Got 0.\n");
        err_count++;
        if(repair) {
            if(ask("Do you want to set it to 1? [y/n]") == 'y') {
                bit_set(imap, 0, 1);
                modified = 1;
                fix_count++;
                printf("Bit set to 1\n");
            } else {
                printf("Ok, I won't change it.\n");
            }
        }
    }

    // check consistency between imap free/allocted inodes and ilist free/allocated inodes
    if(lseek(device, block2byte(BLK_ILIST), SEEK_SET) != block2byte(BLK_ILIST))
        die("check_imap: can't seek inode list: %s", strerror(errno));
    struct inode in;
    for(int i = 1; i <= sb.s_ninodes; ++i) {
        if(read(device, &in, INODE_SIZE) != INODE_SIZE)
            die("check_imap: can't read %d-th inode: %s", i, strerror(errno));
        int imap_allocated  = bit_at(imap,i);
        int ilist_allocated = (in.i_mode != I_NOT_ALLOC);
        if( imap_allocated && !ilist_allocated ) {
            printf("check_imap: inode %d is allocated in imap but not allocated in ilist\n", i);
            err_count++;
            if(repair) {
                char answer;
                if((answer = ask("Do you want to de-allocate it in the imap? [y/n]")) == 'y') {
                    bit_set(imap, i, 0);
                    modified = 1;
                    fix_count++;
                    printf("imap modified\n");
                } else {
                    printf("Ok, I won't change it.\n");
                }
            }
        } else if(!imap_allocated && ilist_allocated ) {
            printf("check_imap: inode %d is not allocated in imap but is allocated in ilist\n", i);
            err_count++;
            if(repair) {
                char answer;
                if((answer = ask("Do you want to allocate it in the imap? [y/n]")) == 'y') {
                    bit_set(imap, i, 1);
                    modified = 1;
                    fix_count++;
                    printf("imap modified\n");
                } else {
                    printf("Ok, I won't change it.\n");
                }
            }
        }
    }

    if(modified) {
        rw_bitmap(imap, BLK_IMAP, N_IMAP, WRITE);
        modified = 0;
    }
    free(imap);

    if(err_count == 0)
        printf("imap seems to be clean\n");
    else
        printf("%d errors found in imap. %d of them were fixed.\n", err_count, fix_count);
}

void check_zmap() {
    printf("Checking zmap...\n");
    int err_count = 0;
    int fix_count = 0;

    // load zmap
    bitchunk_t *zmap = alloc_bitmap(N_ZMAP);
    rw_bitmap(zmap, BLK_ZMAP, N_ZMAP, READ);

    // first bit of zmap should always be 1
    if( !bit_at(zmap, 0) ) {
        printf("check_zmap: the very first bit of the zmap should always be 1. Got 0.\n");
        err_count++;
        if(repair) {
            if(ask("Do you want to set it to 1? [y/n]") == 'y') {
                bit_set(zmap, 0, 1);
                modified = 1;
                fix_count++;
                printf("Bit set to 1\n");
            } else {
                printf("Ok, I won't change it.\n");
            }
        }
    }

    // check consistency between zmap free/allocted zones and zones used in inodes i_zone[] attribute
    // we build a zmap according to zones used in inodes:
    bitchunk_t *zmap_built = alloc_bitmap(N_ZMAP);
    ((char*)zmap_built)[0] |= 1; // set first bit to 1
    struct inode in;
    for(int i = 1; i <= sb.s_ninodes; ++i) {
        int inode_offset = block2byte(BLK_ILIST) + (i-1) * INODE_SIZE;
        if(lseek(device, inode_offset, SEEK_SET) != inode_offset)
            die("check_zmap: can't seek inode list: %s", strerror(errno));
        if(read(device, &in, INODE_SIZE) != INODE_SIZE)
            die("check_zmap: can't read %d-th inode: %s", i, strerror(errno));
        if(in.i_mode == I_NOT_ALLOC)
            continue;
        iterate_zones(zmap_built, in.i_zone, V2_NR_DZONES, 0);
        iterate_zones(zmap_built, &(in.i_zone[V2_NR_DZONES]), 1, 1);
        iterate_zones(zmap_built, &(in.i_zone[V2_NR_DZONES+1]), 1, 2);
    }

    // compare both zmaps
    char *zmap_char = (char*)zmap;
    char *zmap_built_char = (char*)zmap_built;
    for(int k = 0; k < N_ZMAP * sb.s_block_size; ++k) {
       if(zmap_char[k] != zmap_built_char[k]) {

           printf("%d-th zmap byte are differents!\n", k);
       }
    }

    if(modified) {
        rw_bitmap(zmap, BLK_ZMAP, N_ZMAP, WRITE);
        modified = 0;
    }
    free(zmap);
    free(zmap_built);

    if(err_count == 0)
        printf("zmap seems to be clean\n");
    else
        printf("%d errors found in zmap. %d of them were fixed.\n", err_count, fix_count);
}
