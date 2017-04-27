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

#endif // __DEVIO_H__
