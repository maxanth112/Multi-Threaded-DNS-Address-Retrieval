#include "multi-lookup.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>

typedef struct {
    char** buffer;
    char** ifiles;

    int curr_ifile;
    int ifiles_length;
    
    int next_ip_address;
    int curr_bcount;

    sem_t sem_ifile;
    sem_t empty;
} bbuffer;


int check_args(int argc, int req_n, int res_n, char* req_log, char* res_log);
void* requester(bbuffer *buf);


int main(int argc, char **argv) {

    int req_num = strtol(argv[1], NULL, 10);
    int res_num = strtol(argv[2], NULL, 10);
    char* req_log = argv[3];
    char* res_log = argv[4];

    if (!check_args(argc, req_num, res_num, req_log, res_log)) {
        perror("Exiting program with return -1.\n");
        return -1;
    }


    bbuffer buf;
    buf.ifiles = &argv[5];
    buf.ifiles_length = argc - 4;
    buf.curr_ifile = 0;
    
    buf.curr_bcount = 0;
    sem_init(&buf.sem_ifile, 0, 1);
    
    pthread_t req_thread[req_num];
    for (int i = 0; i < req_num; i++) {
        if (pthread_create(&req_thread[i], NULL, requester, &buf) != 0) {
            printf("failed to create the %d'th thread", i);
        }
    }

    for (int i = 0; i < req_num; i++) {
        if (pthread_join(req_thread[i], NULL) != 0) {
            printf("failed to join thread %d", i);
        }
    }

    return 0;
}


void* requester(bbuffer *buf) {

    int ifiles_serviced = 0;
    int got_file = 0;

    while (1) {
        
        char curr_input_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH];

        /* wait on other resolver threads before grabbing new input file */
        sem_wait(&buf->sem_ifile); 
        FILE* input_file = fopen(buf->ifiles[buf->curr_ifile], "r");
        
        if (input_file == NULL) {
            printf("Unable to open the file: %s\n", buf->ifiles[buf->curr_ifile]);
            exit(1);
        } 
        else {
            
            printf("Thread %d grabbed the file: %s\n", buf->ifiles[buf->curr_ifile]);
            buf->curr_ifile++;
            ifiles_serviced++;
            got_file = 1;
            
            int i = 0;
            while (fgets(curr_input_ips[i], MAX_NAME_LENGTH, input_file)) {
                printf("%s", curr_input_ips[i]);
                i++;
            }
        }

        sem_post(&buf->sem_ifile);



        
        

        if (buf->curr_ifile == buf->ifiles_length) break;
    }

    printf("thread %s serviced %d files", ifiles_serviced);
    return NULL;
}







int check_args(int argc, int req_n, int res_n, char* req_log, char* res_log) {

    int return_val = 1;
    printf("%d\n", argc);
    if (argc <= 5) {
        perror("Not enough arguments passed.\n");
        return_val = 0;
    }
    if (argc > MAX_INPUT_FILES + 5) {
        perror("Too many input files passed.\n");
        return_val = 0;
    }
    if (req_n <= 0 || req_n > 10) {
        perror("Requester number must be a positive integer less than or equal to 10.\n");
        return_val = 0;
    }
    if (res_n <= 0 || res_n > 10) {
        perror("Resolver number must be a positive integer less than or equal to 10.\n");
        return_val = 0;
    }

    return return_val;
}