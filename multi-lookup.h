#ifndef ARGS
#define ARGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <errno.h>


#define ARRAY_SIZE 10
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 255
#define MAX_IP_LENGTH INET6_ADDRSTRLEN


/* struct to keep all shared memory together */
typedef struct {
    char bufer[ARRAY_SIZE][MAX_NAME_LENGTH];
    char ifiles[MAX_INPUT_FILES][MAX_NAME_LENGTH];
    char req_log[MAX_NAME_LENGTH];
    char res_log[MAX_NAME_LENGTH];

    int ifile_count;
    int curr_ifile;
    int in;
    int out;
    int exit;

    int req_num;
    int res_num;
    int req_exited;
    int res_exited;
    int lookup_errors;

    pthread_mutex_t buf_mux;
    pthread_mutex_t ifile_mux;
    pthread_mutex_t req_output_mux;
    pthread_mutex_t res_output_mux;
    sem_t empty;
    sem_t data;
} bbuffer;


/* error checking function for command line args */
void check_args(int argc, 
        int req_n, 
        int res_n, 
        char* req_log, 
        char* res_log);


/* producer & consumer funcitons for the bounded buffer */
void* resolver(void *buf);
void* requester(void *buf);


#endif

