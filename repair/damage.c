#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#include "utils.h"
#include "devio.h"
#include "check.h"
#define _MAIN
#include "glo.h"

#define MAP_FLAG_IMAP 0
#define MAP_FLAG_ZMAP 1
int get_random_map_number(int val, int map_flag) {
    bitchunk_t *map;
    int max_index;
    if(map_flag == MAP_FLAG_IMAP) {
        map = alloc_bitmap(N_IMAP);
        rw_bitmap(imap, BLK_IMAP, N_IMAP, READ);
        max_index = sb.s_ninodes;
    } else {
        map = alloc_bitmap(N_ZMAP);
        rw_bitmap(imap, BLK_ZMAP, N_ZMAP, READ);
        max_index = sb.s_zones;
    }
    int i, count = 0;
    for(i = 1; i <= max_index; ++i) {
        if(bit_at(map,i) == val)
            count++;
    }
    int selection = rand() % count;
    count = 0;
    for(i = 1; i <= max_index && count < selection; ++i) {
        if(bit_at(map,i) == val)
            count++;
    }
    free(map);
    return i;
}

void set_imap_bit(int inode_nr, int val) {
    bitchunk_t *imap = alloc_bitmap(N_IMAP);
    rw_bitmap(imap, BLK_IMAP, N_IMAP, READ);
    bit_set(imap, inode_nr, val);
    rw_bitmap(imap, BLK_IMAP, N_IMAP, WRITE);
    free(imap);
}

void damage_imap_menu() {
    int keepOn = 1;
    while(keepOn) {
        printf("   ---- damage imap ----\n");
        printf("\n");
        printf("    What do you want to do?\n");
        printf("        [1] Get a random unallocated inode number in imap\n");
        printf("        [2] Get a random allocated inode number in imap\n");
        printf("        [3] Unallocate imap inode\n");
        printf("        [4] Allocate imap inode\n");
        printf("        [B] Back\n");
        char answer = ask("");
        int n;
        switch(answer) {
            case '1':
                printf("Random unallocated imap inode: %d\n", get_random_map_number(0, MAP_FLAG_IMAP));
                break;
            case '2':
                printf("Random allocated imap inode: %d\n", get_random_map_number(1, MAP_FLAG_IMAP));
                break;
            case '3':
                printf("Which inode? ");
                scanf("%d", &n); empty_stdin();
                set_imap_bit(n, 0);
                printf("Unallocated inode %d in imap\n", n);
                break;
            case '4':
                printf("Which inode? ");
                scanf("%d", &n); empty_stdin();
                set_imap_bit(n, 1);
                printf("Allocated inode %d in imap\n", n);
                break;
            case 'B':
            case 'b':
                keepOn = 0;
                break;
            default:
                printf("Invalid choice: %c\n", answer);
        }
    }
}

void damage_zmap_menu() {
    int keepOn = 1;
    while(keepOn) {
        printf("   ---- damage zmap ----\n");
        printf("\n");
        printf("    What do you want to do?\n");
        printf("        [1] Get a random unallocated zmap bit number\n");
        printf("        [2] Get a random allocated zmap bit number\n");
        printf("        [3] Unallocate zmap bit number\n");
        printf("        [4] Allocate zmap bit number\n");
        printf("        [B] Back\n");
        char answer = ask("");
        int n;
        switch(answer) {
            case '1':
                printf("Random unallocated zone number: %d\n", get_random_map_number(0, MAP_FLAG_ZMAP) + sb.s_firstdatazone - 1);
                break;
            case '2':
                printf("Random allocated zone number: %d\n", get_random_map_number(1, MAP_FLAG_ZMAP) + sb.s_firstdatazone - 1);
                break;
            case '3':
                printf("Which data zone? ");
                scanf("%d", &n); empty_stdin();
                set_imap_bit(n - sb.s_firstdatazone + 1, 0);
                printf("Unallocated data zone %d in zmap\n", n);
                break;
            case '4':
                printf("Which data zone? ");
                scanf("%d", &n); empty_stdin();
                set_imap_bit(n - sb.s_firstdatazone + 1, 1);
                printf("Allocated data zone %d in zmap\n", n);
                break;
            case 'B':
            case 'b':
                keepOn = 0;
                break;
            default:
                printf("Invalid choice: %c\n", answer);
        }
    }
}

void menu() {
    int keepOn = 1;
    while(keepOn) {
        printf("   ==== DAMAGE MINIX FILESYSTEM TOOL ====\n");
        printf("\n");
        printf("    What do you want to do?\n");
        printf("        [1] Damage imap\n");
        printf("        [2] Damage ilist\n");
        printf("        [Q] Quit\n");
        char answer = ask("");
        switch(answer) {
            case '1':
                damage_imap_menu();
                break;
            case '2':

                break;
            case 'Q':
            case 'q':
                keepOn = 0;
                break;
            default:
                printf("Invalid choice: %c\n", answer);
        }
    }
}

int main(int argc, char *argv[]) {
    repair = 0; force = 0; device_file = NULL; device = -1; modified = 0;
    srand(time(NULL));
    /* get arguments and check their validity */
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(!strcmp(argv[i], "--force") || !strcmp(argv[i], "-f")) {
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

    sync();
    repair = 1; // open device in read-write mode to damage it
    dev_open();
    rw_superblock(READ);

    menu();
    printf("Bye!\n");

    return EXIT_SUCCESS;
}
