#ifndef __CHECK_H__
#define __CHECK_H__

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
#include "devio.h"
#include "glo.h"

void print_superblock();
void check_superblock();
void check_imap();
void check_zmap();

#endif // __CHECK_H__
