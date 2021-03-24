#include "multi-lookup.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>

// void* requester(void *args);

// int *file_count;

int main(int argc, char **argv) {

    int req_num = strtol(argv[1], NULL, 10);
    int res_num = strtol(argv[2], NULL, 10);
    char *req_log_name = argv[3];
    char *res_log_name = argv[4];




    printf("req count: %d\n", req_num);
    printf("res count: %d\n", res_num);

    printf("req name: %s\n", req_log_name);
    printf("res name: %s\n", res_log_name);

    


    // /* declare variables */
    // *file_count = (int) argc - 4;    
    // int req_total = (int) (long) argv[0];
    // pthread_t req_threads[req_total];



    // /* create the requester/resolver threads and deploy them */
    // for (int i = 0; i < req_total; i++) {
    //     if (pthread_create(&req_threads[i], NULL, &requester, (void *) &file_count) != 0) {
    //         perror("Failed to create requester thread.\n");
    //         return -1;
    //     }
    // }

    // /* join the threads and destroy them */
    // for (int i = 0; i < req_total; i++) {
    //     if (pthread_join(req_threads[i], NULL) != 0) {
    //         perror("Failed to create resolver thread.\n");
    //         return -1;
    //     }
    // }

    return 0;
}


// void* requester(void *args) {

    
//     // printf("files: %d", &args);

// }


int check_args(int argc, int req_n, int res_n, char *req_log, char *res_log) {

    int return_val = 1;

    if (argc < 5) {
        perror("Not enough arguments provided.\n");
        return_val = 0;
    }
    if (req_n <= 0 || req_n > 10) {
        perror("Requester number must be a positive integer less than or equal to 10,\n");
        return_val = 0;
    }
    if (res_n <= 0 || res_n > 10) {
        perror("Resolver number must be a positive integer less than or equal to 10,\n");
        return_val = 0;
    }
    return return_val;




}