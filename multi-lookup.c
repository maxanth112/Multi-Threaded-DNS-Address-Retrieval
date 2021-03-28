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
    
    int in;
    int out;

    pthread_mutex_t buff_mux;
    pthread_mutex_t ifile_mux;
    sem_t empty;
    sem_t full;

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

    buf->in = 0;
    buf->out = 0;
    
    pthread_mutex_init(&buf->buff_mux, NULL);
    pthread_mutex_init(&buf->ifile_mux, NULL);
    sem_init(&buf->empty, 0, ARRAY_SIZE);
    sem_init(&buf->full, 0, 0);

    buf->req_log = req_log;
    buf->res_log = res_log;

    pthread_t req_thread[req_num];
    pthread_t res_thread[res_num];
    
    for (int i = 0; i < (req_num + res_num); i++) {
        if (i < req_num) {
            if (pthread_create(&req_thread[i], NULL, requester, buf) != 0) printf("failed to create the %d'th requester thread\n", i);
            // else printf("created requester thread %lu\n", pthread_self());
        } 
        else {
            printf("res");
            if (pthread_create(&res_thread[i], NULL, resolver, buf) != 0) printf("failed to create the %d'th resolver thread\n", i);
            // else printf("created resolver thread %lu\n", pthread_self());
        }
    }

    for (int i = 0; i < (req_num + res_num); i++) {
        if (i < req_num) {
            if (pthread_join(req_thread[i], NULL) != 0) printf("failed to join requester thread %d\n", i);
        }
        else {
            if (pthread_join(res_thread[i], NULL) != 0) printf("failed to join resolver thread %d\n", i);
        }
    }

    pthread_mutex_destroy(&buf->buff_mux);
    pthread_mutex_destroy(&buf->ifile_mux);
    sem_destroy(&buf->full);
    sem_destroy(&buf->empty);
    free(buf);
    return 0;
}


void* requester(void *buf) {

    int ifiles_serviced = 0;
    bbuffer* buff = buf;

    while (1) {
        
        char curr_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH]; /* fix later */
        int ip_index = 0;
        int servicing_file;

        /* wait on other resolver threads before grabbing new input file */
        pthread_mutex_lock(&buff->ifile_mux);
        if (buff->curr_ifile == buff->ifiles_length) {
            printf("thread %lu serviced %d files\n", pthread_self(), ifiles_serviced);
            pthread_mutex_unlock(&buff->ifile_mux);
            return NULL;
        } else {
            int servicing_file = buff->curr_ifile;
            buff->curr_ifile++;
            printf("thread %lu grabbing file %s\n", pthread_self(), buff->ifiles[servicing_file]); /* debug */
            printf("curr ifile: %d, ifile length: %d\n", buff->curr_ifile, buff->ifiles_length);
            pthread_mutex_unlock(&buff->ifile_mux);
        }

        FILE* input_file = fopen(buff->ifiles[servicing_file], "r");
        if (input_file == NULL) printf("Unable to open the file: %s\n", buff->ifiles[servicing_file]);
        else {
            ifiles_serviced++;
            while (fgets(curr_ips[ip_index], MAX_NAME_LENGTH, input_file)) {
                printf("curr_ips[%d] = %s", ip_index, curr_ips[ip_index]);
                ip_index++;
            }
        }
        fclose(input_file);

        /* wait on other requester and resolver threads before inserting new ip address to buffer */
        for (int i = 0; i < ip_index - 1; i++) {            
            sem_wait(&buff->empty);
            pthread_mutex_lock(&buff->buff_mux);

            printf("inserting %dth ip address into buffer\n", i);
            buff->buffer[buff->in] = curr_ips[i];
            buff->in = (buff->in + 1) % ARRAY_SIZE;
            printf("buff->next_in = %d\n", buff->in);

            pthread_mutex_lock(&buff->buff_mux);
            sem_post(&buff->full);
        }

        FILE* serviced_file = fopen(buff->req_log, "a+");
        fprintf(serviced_file, "%s", buff->ifiles[servicing_file]);
        fclose(serviced_file);
    }
}


void* resolver(void *buf) {

    int ifiles_resolved = 0;
    bbuffer* buff = buf;

    while (1) {

        sem_wait(&buff->full);
        pthread_mutex_lock(&buff->buff_mux);
        
        char* ip_addr = buff->buffer[buff->out];
        buff->out = (buff->out + 1) % ARRAY_SIZE;
        printf("thread %lu took the address %s", pthread_self(), ip_addr);

        pthread_mutex_unlock(&buff->buff_mux);
        sem_post(&buff->empty);
    }

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