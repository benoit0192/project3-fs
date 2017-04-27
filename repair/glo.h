#ifndef __REPAIR_GLO_H__
#define __REPAIR_GLO_H__

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

#undef EXTERN
#define EXTERN

/* command line parameters */
EXTERN int repair;
EXTERN int force;
EXTERN char *device_file;

/* opened device file descriptor */
EXTERN int device;

/* super block of current device */
EXTERN struct super_block sb;

/*  */
EXTERN int modified;

#endif // __REPAIR_GLO_H__
