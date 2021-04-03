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
    bbuffer* buf = create_buffer_struct();

    buf->ifile_count = argc - 5;
    for (int i = 5; i < argc; i++) {
        strcpy(buf->ifiles[i - 5], argv[i]);
    }
    strcpy(buf->req_log, req_log);
    strcpy(buf->res_log, res_log);

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
    free_buffer_struct(buf);

    /* end timer and print total time taken */
    gettimeofday(&end, NULL);
    int seconds = (end.tv_sec - start.tv_sec);
    int micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf("./multi-lookup: total time is %d.%d seconds.\n", seconds, micros);
    
    return 0;
}


void* requester(void *buf_ptr) {

    bbuffer* buf = (bbuffer*) buf_ptr;
    
    /* simplify thread ids for console reading */
    int tid = buf->req_num++;
    int serviced_count = 0;
    int exit_condition = 0;

    while (1) {
        
        char curr_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH]; 
        int ip_index = 0;
        int curr_file;

        pthread_mutex_lock(&buf->ifile_mux);
        /* ### CS START : Grabbing New Input File ### */

        if (buf->curr_ifile == buf->ifile_count) { 
            /* no more files to service, thread exits and takes one resolver with it*/
            buf->req_exited++;   
            exit_condition = 1;   

            sem_wait(&buf->empty);
            pthread_mutex_lock(&buf->buf_mux);
            /* ### CS START : Poison Pill Insertion ### */

            strcpy(buf->bufer[buf->in], POISON_PILL);
            buf->in = (buf->in + 1) % ARRAY_SIZE;
            
            pthread_mutex_lock(&buf->stdout_mux);
            printf("thread %d serviced %d files\n", tid, serviced_count);
            pthread_mutex_unlock(&buf->stdout_mux);

            /* ### CS END : Poison Pill Insertion ### */
            pthread_mutex_unlock(&buf->buf_mux);
            sem_post(&buf->data);      
        } else {
            
            curr_file = buf->curr_ifile;
            buf->curr_ifile++;            
        }
        /* ### CS END : Grabbing New Input File ### */
        pthread_mutex_unlock(&buf->ifile_mux);

        if (exit_condition == 1) return NULL;

        /* read in all ip addresses from the current file and put in local memory */
        FILE* input_file = fopen(buf->ifiles[curr_file], "r");
        if (input_file == NULL) {
            pthread_mutex_lock(&buf->stdout_mux);
            fprintf(stderr, "Unable to open the file: %s, moving to next.\n", buf->ifiles[curr_file]);
            pthread_mutex_unlock(&buf->stdout_mux);
        }
        else {
            serviced_count++;
            /* read all input file ip addresses into local memory before entering critical section */
            while (fgets(curr_ips[ip_index], MAX_NAME_LENGTH, input_file)) ip_index++;
            
            for (int i = 0; i < ip_index; i++) {      

                if (strlen(curr_ips[i]) <= 1) {
                    /* accounting for gaps between ip addresses */
                    continue;
                }
                sem_wait(&buf->empty);
                pthread_mutex_lock(&buf->buf_mux);
                /* ### CS START : Buffer IP Address Insertion ### */

                strcpy(buf->bufer[buf->in], curr_ips[i]);
                buf->in = (buf->in + 1) % ARRAY_SIZE;

                /* ### CS END : Buffer IP Address Insertion ### */
                pthread_mutex_unlock(&buf->buf_mux);
                sem_post(&buf->data);


                pthread_mutex_lock(&buf->req_output_mux);                
                /* ### CS START : Append IP Address to Log File ### */

                FILE* output_file = fopen(buf->req_log, "a+");
                if (output_file == NULL) {
                    fprintf(stderr, "Unable to open the output serviced file log.\n");
                    exit(EXIT_FAILURE);
                }
                if (curr_ips[i][strlen(curr_ips[i]) - 1] == '\n') {

                    fprintf(output_file, "%s", curr_ips[i]);
                } else {
                    fprintf(output_file, "%s\n", curr_ips[i]);
                }
                fclose(output_file);
                
                /* ### CS END : Append IP Address to Log File ### */
                pthread_mutex_unlock(&buf->req_output_mux);

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
    int exit_condition = 0;

    while (1) {
       
        char ip_addr[MAX_NAME_LENGTH];

        sem_wait(&buf->data);
        pthread_mutex_lock(&buf->buf_mux); 
        /* ### CS START : Buffer IP Address Consumption ### */
        
        /* get ip address and strip the newline/whitespace chars */
        strcpy(ip_addr, buf->bufer[buf->out]);
        buf->out = (buf->out + 1) % ARRAY_SIZE;
        
        /* exit condition for when requester threads have no more inputs */
        if (strcmp(ip_addr, POISON_PILL) == 0) {
            pthread_mutex_lock(&buf->stdout_mux);
            printf("thread %d resolved %d hostnames\n", tid, resolved_count);
            pthread_mutex_unlock(&buf->stdout_mux);
            exit_condition = 1;
        }

        /* ### CS END : Buffer IP Address Consumption ### */
        pthread_mutex_unlock(&buf->buf_mux);
        sem_post(&buf->empty);

        if (exit_condition == 1) return NULL;

        resolved_count++;
        char addr_cpy[MAX_NAME_LENGTH];
        strcpy(addr_cpy, ip_addr);

        ip_addr[strlen(ip_addr) - 1] = 0;        
        char dns_addr[MAX_IP_LENGTH];
        int lookup_status = dnslookup(ip_addr, dns_addr, MAX_NAME_LENGTH);

        pthread_mutex_lock(&buf->res_output_mux);
        /* ### CS START : Append DNS Address to Log File ### */
        
        FILE* output_file = fopen(buf->res_log, "a+");

        if (lookup_status == UTIL_SUCCESS) { 
            fprintf(output_file, "%.*s, %s\n", (int) strlen(addr_cpy) - 1, addr_cpy, dns_addr); 
        }
        else { 
            fprintf(output_file, "%s, %s\n", ip_addr, "NOT_RESOLVED"); 
        }
        
        fclose(output_file);

        /* ### CS END : Append DNS Address to Log File ### */
        pthread_mutex_unlock(&buf->res_output_mux);  
    }

    return NULL;
}


/* helper functions */
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

    return;
}


bbuffer* create_buffer_struct() {


    bbuffer* buf = (bbuffer*) malloc(sizeof(bbuffer));
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for buf struct with malloc.\n");
        exit(EXIT_FAILURE);
    }
    
    pthread_mutex_init(&buf->buf_mux, NULL);
    pthread_mutex_init(&buf->ifile_mux, NULL);
    pthread_mutex_init(&buf->req_output_mux, NULL);
    pthread_mutex_init(&buf->res_output_mux, NULL);
    pthread_mutex_init(&buf->stdout_mux, NULL);
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

    return buf;
}


void free_buffer_struct(bbuffer* buf) {

    pthread_mutex_destroy(&buf->buf_mux);
    pthread_mutex_destroy(&buf->ifile_mux);
    pthread_mutex_destroy(&buf->req_output_mux);
    pthread_mutex_destroy(&buf->res_output_mux);
    pthread_mutex_destroy(&buf->stdout_mux);
    sem_destroy(&buf->data);
    sem_destroy(&buf->empty);
    free(buf);

    return;
}