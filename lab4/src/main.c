#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "jobber.h"
#include "task.h"

/*
 * "Jobber" job spooler.
 */

int main(int argc, char *argv[])
{
    char *input = NULL;
    while(1) {

        input = sf_readline("Jobber>");
        char *buffer = NULL;

        char* input_token = strtok(input, " ");
        int count = 0;
        while(input_token != NULL){
            if (count == 1) {
                buffer = input_token;
            }
            count += 1;
            input_token = strtok(NULL, "'" );
        }
        if (count == 0){
            continue;
        }

        jobs_init();

        if (!strcmp(input, "help")) {

            printf("Available commands:\n");
            printf("help (0 args) Print this help message\n");
            printf("quit (0 args) Quit the program\n");
            printf("enable (0 args) Allow jobs to start\n");
            printf("disable (0 args) Prevent jobs from starting\n");
            printf("spool (1 args) Spool a new job\n");
            printf("pause (1 args) Pause a running job\n");
            printf("resume (1 args) Resume a paused job\n");
            printf("cancel (1 args) Cancel an unfinished job\n");
            printf("expunge (1 args) Expunge a finished job\n");
            printf("status (1 args) Print the status of a job\n");
            printf("jobs (0 args) Print the status of all jobs\n");

        }
        else if (!strcmp(input, "quit")) {
            //cancel all jobs that are not terminated
            break;
        }

        else if (!strcmp(input, "enable")) {
            jobs_set_enabled(1);
        }

        else if (!strcmp(input, "disable")) {
            jobs_set_enabled(0);
        }
        else if (!strcmp(input, "pause")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "pause");
                return -1;
            }

            return 0;
        }
        else if (!strcmp(input, "resume")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "resume");
                return -1;
            }

            return 0;
        }
        else if (!strcmp(input, "cancel")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "cancel");
                return -1;
            }
            return 0;
        }
        else if (!strcmp(input, "expunge")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "expunge");
                return -1;
            }
            return 0;
        }
        else if (!strcmp(input, "status")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "status");
                return -1;
            }
            return 0;
        }
        else if (!strcmp(input, "jobs")) {
            return 0;
        }
        else if (!strcmp(input, "spool")) {
            if (buffer == NULL) {
                printf("Wrong number of arguments(given:0, required:1) for command :%s\n",
                                                                        "spool");
                return -1;
            }

            job_create(buffer);
        }
        else {
            printf("Invalid Command...\n");
            continue;
        }
    }


    jobs_fini();
    exit(EXIT_SUCCESS);
}