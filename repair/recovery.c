#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "utils.h"
#include "devio.h"
#include "check.h"
#define _MAIN
#include "glo.h"


int main(int argc, char *argv[]) {
    repair = 0; force = 0; device_file = NULL; device = -1; modified = 0;
    /* get arguments and check their validity */
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(!strcmp(argv[i], "--repair") || !strcmp(argv[i], "-r")) {
                repair = 1;
            } else if(!strcmp(argv[i], "--force") || !strcmp(argv[i], "-f")) {
                force = 1;
            } else if(!strcmp(argv[i], "-rf") || !strcmp(argv[i], "-fr")) {
                repair = 1;
                force = 1;
            } else {
                fprintf(stderr, "Invalid option: %s\n", argv[i]);
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if(device_file == NULL) {
             if(is_block_device(argv[i])){
                 device_file = argv[i];
             } else {
                 fprintf(stderr, "%s is not a block device\n", argv[i]);
                 print_usage(argv[0]);
                 exit(EXIT_FAILURE);
             }
        } else {
            fprintf(stderr, "Too many arguments\n");
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if(!device_file) {
        fprintf(stderr, "Missing argument\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* given block device shouln't be mounted unless -f is specified */
    char *mountpoint;
    if( (mountpoint = get_mountpoint(device_file)) ) {
        if(!force)
            die("%s is mounted on %s.\nNothing will be done unless -f or --force is specified.\nExiting now.", device_file, mountpoint);
        else {
            printf("\n---------------------------------------------------------------\n");
            printf("%s is mounted on %s. However you asked to force the execution.\n", device_file, mountpoint);
            printf("Be advised that results might not be accurate and can damage filesystem.\n");
            printf("---------------------------------------------------------------\n");
        }
    }

    /* inform user that we are just checking for errors, not correcting them */
    if(!repair)
        printf("\n%s will look for errors, but won't correct them.\nAdd --repair or -r if you want to repair filesystem.\n", argv[0]);

    sync();
    dev_open();
    rw_superblock(READ);

    print_superblock();
    check_superblock();

    check_imap();
    check_zmap();

    sync();

    return EXIT_SUCCESS;
}
