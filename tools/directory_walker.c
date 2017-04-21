#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

#include "mfs/const.h"
#define EXTERN extern
#include "mfs/super.h"

typedef struct {		/* V2.x disk inode */
  unsigned short d2_mode;		/* file type, protection, etc. */
  unsigned short d2_nlinks;		/* how many links to this file. HACK! */
  short d2_uid;			        /* user id of the file's owner. */
  unsigned short d2_gid;		/* group number HACK! */
  int d2_size;		            /* current file size in bytes */
  unsigned int d2_atime;		/* when was file data last accessed */
  unsigned int d2_mtime;		/* when was file data last changed */
  unsigned int d2_ctime;		/* when was inode data last changed */
  int d2_zone[V2_NR_TZONES];	/* block nums for direct, ind, and dbl ind */
} d2_inode;

int read_superblock(int dev_fd, struct super_block *sb) {
    if(lseek(dev_fd, SUPER_BLOCK_BYTES, SEEK_SET) < 0) {
        perror("Can't seek to superblock");
        return -1;
    }
    if( read(dev_fd, sb, sizeof(*sb)) != sizeof(*sb) ) {
        perror("Can't read superblock");
        return -1;
    }
    return 0;
}

int print_superblock(char *device) {
    int dev_fd;
    if( (dev_fd = open(device, O_RDWR)) < 0 ) {
        perror("Can't open device");
        return -1;
    }
    struct super_block sb;
    if(read_superblock(dev_fd, &sb) == -1)
        return -1;

    printf("ninodes       = %u\n", sb.s_ninodes);
    printf("nzones        = %d\n", sb.s_zones);
    printf("imap_blocks   = %u\n", sb.s_imap_blocks);
    printf("zmap_blocks   = %u\n", sb.s_zmap_blocks);
    printf("firstdatazone = %u\n", sb.s_firstdatazone_old);
    printf("log_zone_size = %u\n", sb.s_log_zone_size);
    printf("maxsize       = %d\n", sb.s_max_size);
    printf("block size    = %d\n", sb.s_block_size);
    printf("magic         = %04x\n", sb.s_magic);

    close(dev_fd);
    return 0;
}

int get_device_filename(int dev, char *str, int size) {
    char path[] = "/dev/";
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    size_t path_len;

    /* Open directory */
    dirp = opendir(path);
    if (dirp == NULL) {
        fprintf(stderr, "Can't open directory %s\n", path);
        return -1;
    }

    while ((direntp = readdir(dirp)) != NULL)
    {
        /* For every directory entry... */
        struct stat fstat;
        char full_name[_POSIX_PATH_MAX + 1];

        /* Ignore special directories. */
        if ((strcmp(direntp->d_name, ".") == 0) ||
            (strcmp(direntp->d_name, "..") == 0))
            continue;

        /* Compute full name, check we are in file length limits */
        path_len = strlen(path);
        if( (path_len + strlen(direntp->d_name) + (path[path_len-1]=='/'?0:1) ) > _POSIX_PATH_MAX) {
            fprintf(stderr, "Skipping %s: path too long\n", path);
            continue;
        }

        strcpy(full_name, path);
        if (full_name[path_len - 1] != '/')
            strcat(full_name, "/");
        strcat(full_name, direntp->d_name);

        /* Print only if it is really a file */
        if (stat(full_name, &fstat) < 0) {
            fprintf(stderr, "Can't stat %s\n", full_name);
            continue;
        }

        if(!S_ISBLK(fstat.st_mode))
            continue;
        if(fstat.st_rdev == dev) {
            if(strlen(full_name) >= size-1) {
                fprintf(stderr, "Found device file, but name buffer to small for %s\n", full_name);
                str[0] = 0;
                return -1;
            }
            strncpy(str, full_name, strlen(full_name)+1);
            break;
        }
    }

    closedir(dirp);
    return 0;
}

/* reads the inode on the specified device and prints used data zones IDs */
void print_data_zones(dev_t dev, ino_t inode_number) {
    /* get device file for reading */
    char block_device_filename[200];
    if( get_device_filename(dev, block_device_filename, 200) < 0 )
        return;
    if(!block_device_filename) {
        fprintf(stderr, "Can't find device with ID %llu\n", dev);
        return;
    }
    int dev_fd;
    if( (dev_fd = open(block_device_filename, O_RDONLY)) == -1) {
        perror("Can't open device block file: ");
        return;
    }

    /* read superblock */
    struct super_block sb;
    if(read_superblock(dev_fd, &sb) == -1)
        return;

    /* read inode */
    int inode_offset = (START_BLOCK + sb.s_imap_blocks + sb.s_zmap_blocks) * sb.s_block_size + (inode_number-1)*V2_INODE_SIZE;
    if( lseek(dev_fd, inode_offset, SEEK_SET) < 0 ) {
        perror("Can't seek to inode");
        return;
    }
    d2_inode in;
    if( read(dev_fd, &in, sizeof(in)) != sizeof(in) ) {
        perror("Can't read inode");
        return;
    }

    /* print data blocks */
    printf("%u [", in.d2_mtime);
    for(int i = 0; i < V2_NR_TZONES-1; ++i) {
        printf("%d,", in.d2_zone[i]);
    }
    printf("%d]", in.d2_zone[V2_NR_TZONES-1]);

    close(dev_fd);
}

int print_dirs(const char *path, int recursive)
{
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    size_t path_len;

    /* Open directory */
    dirp = opendir(path);
    if (dirp == NULL) {
        fprintf(stderr, "Can't open directory %s\n", path);
        return -1;
    }

    while ((direntp = readdir(dirp)) != NULL)
    {
        /* For every directory entry... */
        struct stat fstat;
        char full_name[_POSIX_PATH_MAX + 1];

        /* Ignore special directories. */
        if ((strcmp(direntp->d_name, ".") == 0) ||
            (strcmp(direntp->d_name, "..") == 0))
            continue;

        /* Compute full name, check we are in file length limits */
        path_len = strlen(path);
        if( (path_len + strlen(direntp->d_name) + (path[path_len-1]=='/'?0:1) ) > _POSIX_PATH_MAX) {
            fprintf(stderr, "Skipping %s: path too long\n", path);
            continue;
        }

        strcpy(full_name, path);
        if (full_name[path_len - 1] != '/')
            strcat(full_name, "/");
        strcat(full_name, direntp->d_name);

        /* Print only if it is really a file */
        if (stat(full_name, &fstat) < 0) {
            fprintf(stderr, "Can't stat %s\n", full_name);
            continue;
        }

        if(S_ISREG(fstat.st_mode)) printf("f:");
        else if(S_ISCHR(fstat.st_mode)) printf("c:");
        else if(S_ISBLK(fstat.st_mode)) printf("b:");
        else if(S_ISFIFO(fstat.st_mode)) printf("p:");
        else if(S_ISLNK(fstat.st_mode)) printf("l:");
        else if(S_ISSOCK(fstat.st_mode)) printf("s:");
        else if (S_ISDIR(fstat.st_mode)) printf("d:");
        else printf("u:");
        printf(" %s (%u:%u, %llu) ", full_name, major(fstat.st_dev), minor(fstat.st_dev) ,fstat.st_ino);
        print_data_zones(fstat.st_dev, fstat.st_ino);
        printf("\n");
        if( S_ISDIR(fstat.st_mode) && recursive )
            print_dirs(full_name, 1);
    }

    closedir(dirp);
    return 0;
}

int main(int argc, char *argv[]) {
    int recursive = 0;
    char *dir = NULL;

    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(argv[i][1] == 'r')
                recursive = 1;
            else {
                fprintf(stderr, "invalid option \"%s\"\n", argv[i]);
                fprintf(stderr, "Usage: ./directory_walker [-r] <directory>\n");
                exit(1);
            }
        } else if(dir == NULL) {
            dir = argv[i];
        } else {
            fprintf(stderr, "too many arguments\n");
            fprintf(stderr, "Usage: ./directory_walker [-r] <directory>\n");
            exit(1);
        }
    }

    if(dir == NULL) {
        fprintf(stderr, "missing argument\n");
        fprintf(stderr, "Usage: ./directory_walker [-r] <directory>\n");
        return -1;
    }

    /* Check input parameters. */
    size_t path_len = strlen(dir);
    if(path_len == 0) {
        fprintf(stderr, "Can't walk into empty path\n");
        return -1;
    } else if(path_len > _POSIX_PATH_MAX) {
        fprintf(stderr, "%s: path too long\n", dir);
        return -1;
    }

    print_dirs(dir, recursive);

    return 0;
}
