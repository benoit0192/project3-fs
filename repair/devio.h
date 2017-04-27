#ifndef __DEVIO_H__
#define __DEVIO_H__

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <minix/const.h>
#include <minix/type.h>
#include "mfs/const.h"
#include "mfs/super.h"
#include "mfs/inode.h"
#include "mfs/type.h"

#include "utils.h"
#include "glo.h"

#define READ 1
#define WRITE 2

void dev_open();
void rw_superblock(int rw_flag);
void rw_bitmap(bitchunk_t *b, block_t start, size_t size, int rw_flag);

struct dirent_list {
    struct dirent_list *next;
    ino_t ino;
    char name[MFS_DIRSIZ];
} __packed;

struct dirent_list *iterate_dir_entries(struct dirent_list *l, zone_t zones[], int nr_elts, int level);
struct dirent_list *get_dir_entries(struct inode *in);
void free_dirent_list(struct dirent_list *l);
void flush_dir_entries(ino_t i, struct inode *in, struct dirent_list *l);
int make_lost_found();

#endif // __DEVIO_H__
