#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <minix/minlib.h>
#include "glo.h"

#define div_ceil(x,y) ((((x) - 1) / (y)) + 1) /* ceiling of the division of x by y */

/* returns the value of the off-th bit of map */
#define bit_at(map,off) ( (((char*)map)[(off)/8] & (1 << ((off) % 8))) != 0 )
/* set the off-th of map to the value val */
#define bit_set(map,off,val) ( ((char*)map)[(off)/8] = (((char*)map)[(off)/8] & ~(1 << ((off) % 8))) | (((val)!=0) << ((off) % 8)) )

#define INODES_PER_BLOCK V2_INODES_PER_BLOCK(sb.s_block_size)
#define INODE_SIZE V2_INODE_SIZE

#define block2byte(b)   ((off_t)(b) * sb.s_block_size)
#define zone2block(z)   ((block_t)(z) << sb.s_log_zone_size)
#define BLOCK_PER_ZONE  ((int)zone2block(1))

#define N_IMAP      (sb.s_imap_blocks) /* # of blocks in imap */
#define N_ZMAP      (sb.s_zmap_blocks) /* # of blocks in zmap */
#define N_ILIST     div_ceil(sb.s_ninodes, INODES_PER_BLOCK) /* # of blocks in inode list */
#define N_DATA      (sb.s_zones - sb.firstdatazone) /* # of blocks in data zone */

#define BLK_IMAP    2 /* imap starting block */
#define BLK_ZMAP    (BLK_IMAP + N_IMAP) /* zmap starting block */
#define BLK_ILIST   (BLK_ZMAP + N_ZMAP) /* inode list starting block */
#define BLK_DATA    (zone2block(sb.firstdatazone)) /* data zone starting block */


int is_block_device(char *filename);
char *get_mountpoint(char *filename);
void die(const char *fmt, ...);
void print_usage(char *progname);
void empty_stdin();
char ask(char *question);
bitchunk_t *alloc_bitmap(int nb_blocks);
void iterate_zones(bitchunk_t *bitmap, zone_t zones[], int nr_elts, int level);

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


#endif // __UTILS_H__
