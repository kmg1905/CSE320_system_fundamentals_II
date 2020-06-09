#include <helper_functions.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void help_text(){
    fprintf(stdout, "%s\n",\
"Available commands:\n"\
"help (0 args) Print this help message\n"\
"quit (0 args) Quit the program\n"\
"enable (0 args) Allow jobs to start\n"\
"disable (0 args) Prevent jobs from starting\n"\
"spool (1 args) Spool a new job\n"\
"pause (1 args) Pause a running job\n"\
"resume (1 args) Resume a paused job\n"\
"cancel (1 args) Cancel an unfinished job\n"\
"expunge (1 args) Expunge a finished job\n"\
"status (1 args) Print the status of a job\n"\
"jobs (0 args) Print the status of all jobs");
}

// void convert_commands(TASK *a){

// }

void fill_table_row(int i, char *command, JOB_STATUS status){
    jobs_table[i].command = command;
    jobs_table[i].status = status;
    jobs_table[i].is_free = -1;
    jobs_table[i].exit_status = -1;
}

void set_table_row_pgid(int i,pid_t pgid){
    jobs_table[i].pgid = pgid;
}

void set_table_row_status(int i, JOB_STATUS new_status){
    sf_job_status_change(i, jobs_table[i].status, new_status);
    jobs_table[i].status = new_status;
}

char *parse_command(char *cmd, char *args, int *is_first_quote_present, int *is_last_quote_present, int *words_in_first_token){
    const char s[2] = "'";
    char *temp = args;
    while(isspace(*temp)){
        temp++;
    }

    char *first_non_space = temp;
    *is_first_quote_present = *temp == '\''? 1 : 0;
    while(*temp != '\0'){
        temp++;
    }

    temp--;

    while(isspace(*temp)){
        *temp = '\0';
        temp--;
    }

    *is_last_quote_present = *(temp) == '\''? 1: 0;

    char *args_copy = malloc(sizeof(char) * (strlen(first_non_space)+1));
    strcpy(args_copy, first_non_space);


    // printf("First quote present: %d\n", *is_first_quote_present);
    // printf("Last quote present:%d\n", *is_last_quote_present);
    // printf("Length of args:%d\n", len);
    char *token = strtok(args_copy, s);
    char *first_token = token;
    int count = 0;
    while( token != NULL ) {
        token = strtok(NULL, s);
        count++;
    }

    token = strtok(first_token, " ");
    while(token != NULL){
        *words_in_first_token += 1;
        token = strtok(NULL, " ");
    }

    free(args_copy);

    //printf("%d\n", count);

    if(*is_first_quote_present && *is_last_quote_present){
        if(count == 1){
            *(first_non_space + (strlen(first_non_space)-1)) = '\0';
            return (first_non_space+1);
        }else{
            printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", count,cmd);
            return NULL;
        }
    }else if(*is_first_quote_present && *is_last_quote_present == 0){
        printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", count, cmd);
        return NULL;
    }else if(*is_first_quote_present == 0 && *is_last_quote_present){
        printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", count, cmd);
        return NULL;
    }else{
        if(*words_in_first_token != 1){
            printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", *words_in_first_token, cmd);
            return NULL;
        }else{
            return first_non_space;
        }
    }

    // printf("Words in first list:%d\n", *words_in_first_token);
    // printf("Number of lists: %d\n", count);
}