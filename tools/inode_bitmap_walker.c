#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <minix/const.h>
//#include <minix/type.h>
#include "mfs/const.h"
#include "mfs/super.h"

#define INODES_PER_CHUNCK 142

struct inode_list_elt {
    unsigned int inodes[INODES_PER_CHUNCK];
    struct inode_list_elt *next;
};

int read_superblock(int dev_fd, struct super_block *sb) {
    if(lseek(dev_fd, SUPER_BLOCK_BYTES, SEEK_SET) < 0) {
        perror("Can't seek to superblock");
        return -1;
    }
    if( read(dev_fd, sb, sizeof(*sb)) != sizeof(*sb) ) {
        perror("Can't read superblock");
        return -1;
    }
    if(sb->s_magic != SUPER_V3) {
        fprintf(stderr, "Wrong magic number\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: ./inode_bitmap_walker <device special file>\n");
        fprintf(stderr, "Example: ./inode_bitmap_walker /dev/c0d0p0s0\n");
        return -1;
    }

    /* init inodes list */
    struct inode_list_elt *inodes = malloc(sizeof(*inodes));
    if(inodes == NULL) {
        fprintf(stderr, "Can't malloc inode list\n");
        return -1;
    }
    struct inode_list_elt *current_elt = inodes;
    int current_index = 0;
    current_elt->inodes[current_index] = 0;
    current_elt->next = NULL;

    /* open device */
    int dev_fd;
    if( (dev_fd = open(argv[1], O_RDWR)) < 0 ) {
        perror("Can't open device");
        return -1;
    }

    struct super_block sb;
    if(read_superblock(dev_fd, &sb) < 0)
        return -1;

    if(lseek(dev_fd, 2 * sb.s_block_size, SEEK_SET) != 2 * sb.s_block_size) {
        perror("Can't seek imap");
        return -1;
    }
    char o;
    for(int i = 0; i < sb.s_ninodes; ++i) {
        int mod = i % 8;
        if(mod == 0) {
            if(read(dev_fd, &o, 1) < 0) {
                perror("can't read imap");
                return -1;
            }
        }
        char mask = 1 << mod;
        if(o & mask) {
            current_elt->inodes[current_index] = i;
            if(current_index == INODES_PER_CHUNCK-1) {
                /* append new list elt */
                current_elt->next = malloc(sizeof(*(current_elt->next)));
                if(current_elt->next == NULL) {
                    fprintf(stderr, "Can't malloc inode list\n");
                    return -1;
                }
                current_index = 0;
                current_elt = current_elt->next;
                current_elt->next = NULL;
                current_elt->inodes[current_index] = 0;
            } else {
                current_index++;
                current_elt->inodes[current_index] = 0;
            }
        }
    }

    /* print inodes */
    for(struct inode_list_elt *it = inodes; it; it = it->next) {
        for(int i = 0; i < INODES_PER_CHUNCK && (it->inodes[i] || (i == 0 && it == inodes)); ++i) {
            printf("%d; ", it->inodes[i]);
        }
    }
    printf("\n");

    return 0;
}
