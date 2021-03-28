#include "multi-lookup.h"
#include "util.h"
#include "util.c"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/time.h>


int main(int argc, char **argv) {

    /* start timer */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    /* parsing args for error checking */
    int req_num = strtol(argv[1], NULL, 10);
    int res_num = strtol(argv[2], NULL, 10);

    char req_log[MAX_NAME_LENGTH];
    char res_log[MAX_NAME_LENGTH];
    strcpy(req_log, argv[3]);
    strcpy(res_log, argv[4]);
    
    check_args(argc, req_num, res_num, req_log, res_log);
     
    /* clear any existing log file contents */
    fclose(fopen(req_log, "w"));
    fclose(fopen(res_log, "w"));

    /* allocate buffer struct memory and init all buffer variables */
    bbuffer* buf = (bbuffer*) malloc(sizeof(bbuffer));
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for buf struct with malloc.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 5; i < argc; i++) {
        strcpy(buf->ifiles[i - 5], argv[i]);
    }
    strcpy(buf->req_log, req_log);
    strcpy(buf->res_log, res_log);
    pthread_mutex_init(&buf->buf_mux, NULL);
    pthread_mutex_init(&buf->ifile_mux, NULL);
    sem_init(&buf->empty, 0, ARRAY_SIZE);
    sem_init(&buf->data, 0, 0);
    buf->curr_ifile = 0;
    buf->lookup_errors = 0;
    buf->in = 0;
    buf->out = 0;
    buf->exit = 0;
    buf->req_exited = 0;
    buf->res_exited = 0;
    buf->req_num = 1;
    buf->res_num = 1;
    buf->ifile_count = argc - 5;

    /* thread creation */
    pthread_t req_thread[req_num];
    pthread_t res_thread[res_num];
    for (int i = 0; i < req_num; i++) {
        if (pthread_create(&req_thread[i], NULL, requester, buf) != 0) {
            fprintf(stderr, "failed to create the %d'th requester thread\n", i);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < res_num; i++) {
        if (pthread_create(&res_thread[i], NULL, resolver, buf) != 0) {
            fprintf(stderr, "failed to create the %d'th resolver thread\n", i);
            exit(EXIT_FAILURE);
        }
    }

    /* thread collection */
    for (int i = 0; i < req_num; i++) {
        if (pthread_join(req_thread[i], NULL) != 0) {
            fprintf(stderr, "failed to join requester thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < res_num; i++) {
        if (pthread_join(res_thread[i], NULL) != 0) {
            fprintf(stderr, "failed to join resolver thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    /* destroy and free variables and memory */
    pthread_mutex_destroy(&buf->buf_mux);
    pthread_mutex_destroy(&buf->ifile_mux);
    sem_destroy(&buf->data);
    sem_destroy(&buf->empty);
    free(buf);

    /* end timer and print total time taken */
    gettimeofday(&end, NULL);
    int seconds = (end.tv_sec - start.tv_sec);
    int micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
 
    if (buf->lookup_errors != 0) fprintf(stderr, "Total lookup errors: %d.\n", buf->lookup_errors);
    printf("./multi-lookup: total time is %d.%d seconds.\n", seconds, micros);
    
    return 0;
}


void* requester(void *buf_ptr) {

    bbuffer* buf = (bbuffer*) buf_ptr;
    
    /* simplify thread ids for console reading */
    int tid = buf->req_num++;
    int serviced_count = 0;

    while (1) {
        
        char curr_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH]; 
        int ip_index = 0;
        int curr_file;

        pthread_mutex_lock(&buf->ifile_mux);
        /* ### CS START : Grabbing New Input File ### */

        if (buf->curr_ifile == buf->ifile_count) { 
            /* no more files to service, thread exits */
            buf->req_exited++;            

            if (buf->req_exited == (buf->req_num - 1)) {
                /* last thread is exiting, alert resolvers when buffer is empty */

                while (1) {
                    int data_val;
                    sem_getvalue(&buf->data, &data_val);

                    if (data_val == 0) {
                        buf->exit = 1;
                        sem_post(&buf->data);
                        break;
                    }
                }
            }
            printf("thread %d serviced %d files\n", tid, serviced_count);
            
            /* ### CS END(1) : Grabbing New Input File ### */
            pthread_mutex_unlock(&buf->ifile_mux);
            return NULL;
        } else {
            
            curr_file = buf->curr_ifile;
            buf->curr_ifile++;            
            /* ### CS END(2) : Grabbing New Input File ### */
            pthread_mutex_unlock(&buf->ifile_mux);
        }

        /* read in all ip addresses from the current file and put in local memory */
        FILE* input_file = fopen(buf->ifiles[curr_file], "r");
        if (input_file == NULL) fprintf(stderr, "Unable to open the file: %s, moving to next.\n", buf->ifiles[curr_file]);
        else {
            serviced_count++;
            /* read all input file ip addresses into local memory before entering critical section */
            while (fgets(curr_ips[ip_index], MAX_NAME_LENGTH, input_file)) ip_index++;
            
            for (int i = 0; i < ip_index; i++) {      

                sem_wait(&buf->empty);
                pthread_mutex_lock(&buf->buf_mux);
                /* ### CS START : Buffer IP Address Insertion ### */

                strcpy(buf->bufer[buf->in], curr_ips[i]);
                buf->in = (buf->in + 1) % ARRAY_SIZE;

                /* append ip address to the log file after its inserted into the buffer */
                FILE* output_file = fopen(buf->req_log, "a");
                if (output_file == NULL) {
                    fprintf(stderr, "Unable to open the output serviced file log.\n");
                    exit(EXIT_FAILURE);
                }
                fprintf(output_file, "%s", curr_ips[i]);
                fclose(output_file);

                /* ### CS END : Buffer IP Address Insertion ### */
                pthread_mutex_unlock(&buf->buf_mux);
                sem_post(&buf->data);
            }
        }
        fclose(input_file);
    }
}


void* resolver(void *buf_ptr) {

    bbuffer* buf = (bbuffer*) buf_ptr;

    /* simplify thread ids for console reading, resolver threads are in 100s */
    int tid = 100*buf->res_num++;
    int resolved_count = 0;

    while (1) {
       
        sem_wait(&buf->data);
        pthread_mutex_lock(&buf->buf_mux);
        /* ### CS START : Buffer IP Address Consumption ### */
        
        if (buf->exit == 1) {
            sem_post(&buf->data);
            printf("thread %d resolved %d hostnames\n", tid, resolved_count);

            /* ### CS END(1) : Buffer IP Address Consumption ### */
            pthread_mutex_unlock(&buf->buf_mux);
            return NULL;
        }

        /* get ip address and strip the newline/whitespace chars */
        char ip_addr[MAX_NAME_LENGTH];
        strcpy(ip_addr, buf->bufer[buf->out]);
        ip_addr[strcspn(ip_addr, "\n")] = 0;

        buf->out = (buf->out + 1) % ARRAY_SIZE;

        char dns_addr[MAX_IP_LENGTH];
        int lookup_status = dnslookup(ip_addr, dns_addr, MAX_NAME_LENGTH);

        if (lookup_status == UTIL_SUCCESS) {
            resolved_count++;

            FILE* output_file = fopen(buf->res_log, "a+");
            fprintf(output_file, "%s\n", dns_addr);
            fclose(output_file);
        } else {
            buf->lookup_errors++;
            fprintf(stderr, "Error looking up address %s.\n", ip_addr);
        }
        
        /* ### CS END(2) : Buffer IP Address Consumption ### */
        pthread_mutex_unlock(&buf->buf_mux);
        sem_post(&buf->empty);
    }
    return NULL;
}



void check_args(int argc, int req_n, int res_n, char* req_log, char* res_log) {

    if (argc <= 5) {
        fprintf(stderr, "Need a minimum of 6 total arguments passed, only received %d.\n", argc);
        exit(EXIT_FAILURE);
    }
    if (argc > MAX_INPUT_FILES + 5) {
        int ifile_num = argc - 5;
        fprintf(stderr, "Maximum input files accepted is 100, received %d.\n", ifile_num);
        exit(EXIT_FAILURE);
    }
    if (req_n <= 0 || req_n > 10) {
        fprintf(stderr, "Requester number must be a positive integer less than or equal to 10, provided %d.\n", req_n);
        exit(EXIT_FAILURE);
    }
    if (res_n <= 0 || res_n > 10) {
        fprintf(stderr, "Resolver number must be a positive integer less than or equal to 10, provided %d.\n", res_n);
        exit(EXIT_FAILURE);
    }
}