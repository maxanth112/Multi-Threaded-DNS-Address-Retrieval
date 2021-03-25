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
    char** input_files;
    
    int next_input_file;
    int next_ip_address;
    int buff_count;

    sem_t new_input_file;
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
    buf.input_files = &argv[5];
    buf.buff_count = 0;

    sem_init(&buf.new_input_file, 0, 1);
    




    requester(&buf);

    return 0;
}


void* requester(bbuffer *buf) {

   
    
    printf("%s\n", buf->input_files[0]);
    FILE* input_file = fopen(buf->input_files[0], "r");
    if (input_file == NULL) {
        perror("Unable to open the file.\n");
        exit(1);
    }


    char curr_input_ips[MAX_INPUT_FILES][MAX_NAME_LENGTH];
    int i = 0;
    while (fgets(curr_input_ips[i], MAX_NAME_LENGTH, input_file)) {
        printf("%s", curr_input_ips[i]);
        i++;
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