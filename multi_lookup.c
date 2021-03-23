#include "multi_lookup.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>


/* function prototypes */
int check_args(int argc, char*argv[]);
void* producer_routine();
void* consumer_routine();


int main(int argc, char* argv[]) {

    /* error check the command line arguments */
    if (!check_args(argc, argv)) {
        return -1;
    }

    /* give arguments cooresponding names */
    int req_total = argv[0];
    int res_total = argv[1];

    char req_log[strlen(argv[2])];
    char res_log[strlen(argv[3])];
    strcpy(req_log, argv[2]);
    strcpy(res_log, argv[3]);

    /* create the requester threads */
    pthread_t req_threads[req_total];
    for (int i = 0; i < req_total; i++) {
        if (pthread_create(req_threads + i, NULL, &producer_routine, NULL) != 0) {
            perror("Failed to create requester thread.\n");
            return -1;
        }
    }

    /* create the resolver threads */
    pthread_t res_threads[res_total];
    for (int j = 0; j < res_total; j++) {
        if (pthread_create(res_threads + j, NULL, &consumer_routine, NULL) != 0) {
            perror("Failed to create resolver thread.\n");
            return -1;
        }
    }
}



void* producer_routine() {

}


void* consumer_routine() {

}














int check_args(int argc, char*argv[]) {

    /* check all arguments are provided */
    if (argc < 5) {
        printf("Not enough arguments provided.\n");
        return 0;
    }

    /* check correct data types for arguments provided */
    if (!isdigit(argv[0]) || (argv[0] < 0 || argv[0] > ARRAY_SIZE)) {
        printf("First argument should be a positive integer of at most 10.\n");
        return 0;
    }
    if (!isdigit(argv[1]) || (argv[1] < 0 || argv[1] > ARRAY_SIZE)) {
        printf("Second argument should be a positive integer of at most 10.\n");
        return 0;
    }

    return 1;
}
