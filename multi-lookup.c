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
    sem_t sem_req_mux;
    sem_t sem_res_mux;

    char* req_log;
    char* res_log;
} bbuffer;


int check_args(int argc, int req_n, int res_n, char* req_log, char* res_log);
void* requester(void *buf);
void* resolver(void *buf);


int main(int argc, char **argv) {

    int req_num = strtol(argv[1], NULL, 10);
    int res_num = strtol(argv[2], NULL, 10);
    char* req_log = argv[3];
    char* res_log = argv[4];

    if (!check_args(argc, req_num, res_num, req_log, res_log)) {
        perror("Exiting program with return -1.\n");
        return -1;
    }

    bbuffer* buf = malloc(sizeof(bbuffer));
    buf->ifiles = &argv[5];
    buf->ifiles_length = argc - 5;
    buf->curr_ifile = 0;
    buf->curr_bcount = 0;

    sem_init(&buf->sem_ifile, 0, 1);
    sem_init(&buf->sem_req_mux, 0, 1);
    sem_init(&buf->sem_res_mux, 0, 1);

    buf->req_log = req_log;
    buf->res_log = res_log;

    pthread_t req_thread[req_num];
    pthread_t res_thread[res_num];
    
    for (int i = 0; i < req_num; i++) {
        if (pthread_create(&req_thread[i], NULL, requester, buf) != 0) {
            printf("failed to create the %d'th requester thread\n", i);
        }
    }
    for (int i = 0; i < res_num; i++) {
        if (pthread_create(&res_thread[i], NULL, resolver, buf) != 0) {
            printf("failed to create the %d'th resolver thread\n", i);
        }
    }



    for (int i = 0; i < req_num; i++) {
        if (pthread_join(req_thread[i], NULL) != 0) {
            printf("failed to join requester thread %d\n", i);
        }
    }
    for (int i = 0; i < res_num; i++) {
        if (pthread_join(res_thread[i], NULL) != 0) {
            printf("failed to join resolver thread %d\n", i);
        }
    }


    sem_destroy(&buf->sem_ifile);
    sem_destroy(&buf->sem_req_mux);
    sem_destroy(&buf->sem_res_mux);
    free(buf);
    return 0;
}


void* requester(void *buf) {

    int ifiles_serviced = 0;
    bbuffer* buff = buf;

    while (1) {
        
        printf("current file : %d\n total file : %d\n", buff->curr_ifile, buff->ifiles_length);
        char curr_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH]; /* fix later */
        int ip_index = 0;

        /* wait on other resolver threads before grabbing new input file */
        sem_wait(&buff->sem_ifile); 

        if (buff->curr_ifile == buff->ifiles_length) {
            sem_post(&buff->sem_ifile);
            break;
        }

        FILE* input_file = fopen(buff->ifiles[buff->curr_ifile], "r");
        if (input_file == NULL) {
            printf("Unable to open the file: %s\n", buff->ifiles[buff->curr_ifile]);
            exit(1);
        } 
        else {
            
            printf("Thread %lu grabbed the file: %s\n", pthread_self(), buff->ifiles[buff->curr_ifile]);
            buff->curr_ifile++;
            ifiles_serviced++;
            
            while (fgets(curr_ips[ip_index], MAX_NAME_LENGTH, input_file)) {
                printf("%s", curr_ips[ip_index]);
                ip_index++;
            }
        }

        sem_post(&buff->sem_ifile);



        /* wait on other requester and resolver threads before inserting new ip address to buffer */
        for (int i = 0; i < ip_index; i++) {
            
            sem_wait(&buff->sem_req_mux);




            sem_post(&buff->sem_req_mux);
        }
    }
    printf("thread %lu serviced %d files\n", pthread_self(), ifiles_serviced);
    return NULL;
}


void* resolver(void *buf) {


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