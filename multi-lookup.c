#include "multi-lookup.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>

int check_args(int argc, char *argv[]);
void* requester(void *args);

int *file_count;

int main(int argc, char* argv[]) {

    /* error check the command line arguments */
    if (!check_args(argc, argv)) {
        return -1;
    }


    /* declare variables and thread safe helpers */
    *file_count = (int) argc - 4;    
    int req_total = (int) (long) argv[0];
    pthread_t req_threads[req_total];



    /* create the requester/resolver threads and deploy them */
    for (int i = 0; i < req_total; i++) {
        if (pthread_create(&req_threads[i], NULL, &requester, (void *) &file_count) != 0) {
            perror("Failed to create requester thread.\n");
            return -1;
        }
    }

    /* join the threads and destroy them */
    for (int i = 0; i < req_total; i++) {
        if (pthread_join(req_threads[i], NULL) != 0) {
            perror("Failed to create resolver thread.\n");
            return -1;
        }
    }

    return 0;
}


void* requester(void *args) {

    
    printf("files: %p", &args);

}


int check_args(int argc, char *argv[]) {

    /* check all arguments are provided */
    if (argc < 5) {
        printf("Not enough arguments provided.\n");
        return 0;
    }
    if (argc > 4 + ARRAY_SIZE) {
        printf("Too many argument files provided.\n");
        return 0;
    }

    /* check correct data types for arguments provided */
    if (!isdigit(*argv[0]) || (*argv[0] < 0 || *argv[0] > ARRAY_SIZE)) {
        printf("First argument should be a positive integer of at most 10.\n");
        return 0;
    }
    if (!isdigit(*argv[1]) || (*argv[1] < 0 || *argv[1] > ARRAY_SIZE)) {
        printf("Second argument should be a positive integer of at most 10.\n");
        return 0;
    }

    return 1;
}
