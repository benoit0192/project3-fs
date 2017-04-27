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
           // bytes do not match. look for rebel bits
           char mask = 1;
           for(int b = 0; b < 8; ++b, mask <<= 1) {
               int bit_off = k * 8 + b;
               int data_zone = bit_off + sb.s_firstdatazone - 1;
               char zmap_bit = zmap_char[k] & mask;
               char zmap_built_bit = zmap_built_char[k] & mask;
               if( zmap_bit && !zmap_built_bit ) {
                   printf("check_zmap: data zone %d is allocated in zmap but no inode references it\n", data_zone);
                   err_count++;
                   if(repair) {
                       char answer;
                       if((answer = ask("Do you want to de-allocate it in the zmap? [y/n]")) == 'y') {
                           bit_set(zmap, bit_off, 0);
                           modified = 1;
                           fix_count++;
                           printf("zmap modified\n");
                       } else {
                           printf("Ok, I won't change it.\n");
                       }
                   }
               } else if( !zmap_bit && zmap_built_bit ) {
                   printf("check_zmap: data zone %d is not allocated in zmap but is referenced by at least one inode\n", data_zone);
                   err_count++;
                   if(repair) {
                       char answer;
                       if((answer = ask("Do you want to allocate it in the zmap? [y/n]")) == 'y') {
                           bit_set(zmap, bit_off, 1);
                           modified = 1;
                           fix_count++;
                           printf("zmap modified\n");
                       } else {
                           printf("Ok, I won't change it.\n");
                       }
                   }
               }
           }
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

void check_dots() {
    printf("Checking dots...\n");
    int err_count = 0;
    int fix_count = 0;

    // look for directories, and check their . and .. values
    struct inode in;
    for(int i = 1; i <= sb.s_ninodes; ++i) {
        int inode_offset = block2byte(BLK_ILIST) + (i-1) * INODE_SIZE;
        if(lseek(device, inode_offset, SEEK_SET) != inode_offset)
            die("check_dots: can't seek inode list: %s", strerror(errno));
        if(read(device, &in, INODE_SIZE) != INODE_SIZE)
            die("check_dots: can't read %d-th inode: %s", i, strerror(errno));
        if((in.i_mode & I_TYPE) != I_DIRECTORY)
            continue;
        // check directory content
        struct dirent_list *l = get_dir_entries(&in);
        struct dirent_list *it = l; // iterator
        while(it) {
            // do not open recursively . and ..
            if(!strcmp(it->name, ".")) {
                it = it->next;
                continue;
            } else if(!strcmp(it->name, "..")) {
                it = it->next;
                continue;
            }
            // read subdirectories to check their . and .. values
            struct inode child_inode;
            int child_inode_offset = block2byte(BLK_ILIST) + (it->ino-1) * INODE_SIZE;
            if(lseek(device, child_inode_offset, SEEK_SET) != child_inode_offset)
                die("check_dots: can't seek child inode: %s", strerror(errno));
            if(read(device, &child_inode, INODE_SIZE) != INODE_SIZE)
                die("check_dots: can't read child inode: %s", strerror(errno));
            if((child_inode.i_mode & I_TYPE) != I_DIRECTORY) {
                it = it->next;
                continue;
            }
            struct dirent_list *l_child = get_dir_entries(&child_inode);
            struct dirent_list *child_it = l_child;
            int has_dot = 0, has_dot_dot = 0;
            while(child_it) {
                if(!strcmp(child_it->name, ".")) {
                    has_dot = 1;
                    if(child_it->ino != it->ino) {
                        err_count++;
                        printf(". entry of directory inode %llu is not pointing to itself\n", it->ino);
                        if(repair) {
                            if(ask("Do you want to set it to its own inode value? [y/n]") == 'y') {
                                child_it->ino = it->ino;
                                fix_count++;
                                modified = 1;
                                printf(". entry fixed\n");
                            } else {
                                printf("Ok, I won't change it.\n");
                            }
                        }
                    }
                } else if(!strcmp(child_it->name, "..")) {
                    has_dot_dot = 1;
                    if(child_it->ino != i) {
                        printf(".. entry of directory inode %llu is not pointing to its parent %d\n", it->ino, i);
                        if(repair) {
                            if(ask("Do you want to set it to its parent inode value? [y/n]") == 'y') {
                                child_it->ino = i;
                                fix_count++;
                                modified = 1;
                                printf(".. entry fixed\n");
                            } else {
                                printf("Ok, I won't change it.\n");
                            }
                        }
                    }
                }
                child_it = child_it->next;
            }
            if(!has_dot) {
                printf("directory inode %llu doesn't have a . entry!\n", it->ino);
                err_count++;
                if(repair) {
                    if(ask("Do you want to add a . entry? [y/n]") == 'y') {
                        struct dirent_list *new = malloc(sizeof(*new));
                        new->ino = it->ino;
                        for(int k = 0; k < MFS_DIRSIZ; ++k)
                            new->name[k] = 0;
                        new->name[0] = '.';
                        new->next = l_child;
                        l_child = new;
                        fix_count++;
                        modified = 1;
                        printf(". entry fixed\n");
                    } else {
                        printf("Ok, I won't change it.\n");
                    }
                }
            }
            if(!has_dot_dot) {
                printf("directory inode %d doesn't have a .. entry!\n", i);
                err_count++;
                if(repair) {
                    if(ask("Do you want to add a .. entry? [y/n]") == 'y') {
                        struct dirent_list *new = malloc(sizeof(*new));
                        new->ino = i;
                        for(int k = 0; k < MFS_DIRSIZ; ++k)
                            new->name[k] = 0;
                        new->name[0] = '.'; new->name[1] = '.';
                        new->next = l_child;
                        l_child = new;
                        fix_count++;
                        modified = 1;
                        printf(".. entry fixed\n");
                    } else {
                        printf("Ok, I won't change it.\n");
                    }
                }
            }
            if(modified) {
                flush_dir_entries(it->ino, &child_inode, l_child);
                modified = 0;
            }
            free_dirent_list(l_child);
            it = it->next;
        }
        free_dirent_list(l);
    }

    if(err_count == 0)
        printf("directory dots seem to be clean\n");
    else
        printf("%d errors found in directory dots. %d of them were fixed.\n", err_count, fix_count);
}

void check_directories() {
    printf("Checking directories...\n");
    int err_count = 0;
    int fix_count = 0;

    // access map. each inode gets a 1 if there is an entry for it in a directory
    char *amap = (char*)alloc_bitmap(N_IMAP);
    amap[0] |= 0b11; // inode 0 and root inode are always reachable

    // iterate over directory inodes and look at their contents
    struct inode in;
    for(int i = 1; i <= sb.s_ninodes; ++i) {
        int inode_offset = block2byte(BLK_ILIST) + (i-1) * INODE_SIZE;
        if(lseek(device, inode_offset, SEEK_SET) != inode_offset)
            die("check_directory: can't seek inode list: %s", strerror(errno));
        if(read(device, &in, INODE_SIZE) != INODE_SIZE)
            die("check_directory: can't read %d-th inode: %s", i, strerror(errno));
        if((in.i_mode & I_TYPE) != I_DIRECTORY)
            continue;
        struct dirent_list *l = get_dir_entries(&in);
        for(struct dirent_list *it = l; it; it = it->next) {
            // do not open recursively . and ..
            if(!strcmp(it->name, ".")) continue;
            if(!strcmp(it->name, "..")) continue;
            // all inodes in this directory are reachable
            bit_set(amap, it->ino, 1);
        }
        free_dirent_list(l);
    }

    // now we are looking for inodes which are either:
    //  - not allocated but reachable (we remove the entry reaching them)
    //  - allocated but not reachable (we add an inode for them in /lost+found)
    char *imap = (char*)alloc_bitmap(N_IMAP);
    rw_bitmap((bitchunk_t*)imap, BLK_IMAP, N_IMAP, READ);
    for(int i = 1; i <= sb.s_ninodes; ++i) {
        if(!bit_at(imap, i) && bit_at(amap, i)) {
            printf("inode %d is not allocated but is reachable\n", i);
            err_count++;

        } else if(bit_at(imap, i) && !bit_at(amap, i)) {
            printf("inode %d is allocated but is not reachable\n", i);
            err_count++;
        }
    }

    if(err_count == 0)
        printf("directories seem to be clean\n");
    else
        printf("%d errors found in directories. %d of them were fixed.\n", err_count, fix_count);
}
