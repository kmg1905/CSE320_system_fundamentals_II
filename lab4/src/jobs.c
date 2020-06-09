/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "helper_functions.h"
#include "signal_handlers.h"
#include "task.h"

int jobs_init(void) {
    // TO BE IMPLEMENTED
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);
    signal(SIGCHLD, sigchld_handler);
    // signal(SIGINT, sigint_handler);
    // signal(SIGINT, sigchld_handler);
    active_runners = 0;
    for(int i=0;i<MAX_JOBS;i++){
        jobs_table[i].command = NULL;
        jobs_table[i].pgid = -1;
        jobs_table[i].is_free = 1;
        jobs_table[i].is_canceled = -1;
    }
    sf_set_readline_signal_hook(handler);

    return 0;
}

void jobs_fini(void) {
    // TO BE IMPLEMENTED
    for(int i=0;i<MAX_JOBS;i++){
        job_cancel(i);
    }

    sigset_t new_mask, old_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &new_mask, &old_mask);

    pid_t runner;
    int status;
    while((runner = waitpid(-1,&status, WUNTRACED)) > 0){
        int i=0;
        while(i<MAX_JOBS && jobs_table[i].pgid != runner){
            i++;
        }

        if(i>=MAX_JOBS){
            continue;
        }

        if(WIFEXITED(status)){
            jobs_table[i].exit_status = status;
            // printf("Exit status:%d\n", jobs_table[i].exit_status);
            set_table_row_status(i, COMPLETED);
            sf_job_end(i, runner, status);
        }

        if(WIFSIGNALED(status)){
            jobs_table[i].exit_status = status;
            // printf("Exit aborted:%d\n", jobs_table[i].exit_status);
            sf_job_end(i, runner, status);
            // if(jobs_table[i].status != CANCELED){
            // }
            set_table_row_status(i, ABORTED);
        }

        active_runners--;
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    for(int i=0;i<MAX_JOBS;i++){
        if(jobs_table[i].is_free == -1){
            job_expunge(i);
        }
    }
}

int jobs_set_enabled(int val) {
    // TO BE IMPLEMENTED
    // sigset_t mask, prev_mask;
    // sigfillset(&mask);
    // sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    int prev_enabler = enabler;
    enabler = val;
    int i = 0;
    if(enabler != 0){
        while(i<MAX_JOBS && active_runners<MAX_RUNNERS){
            if(jobs_table[i].status == WAITING){
                run_task(i,jobs_table[i].command);
            }
            i++;
        }
    }
    // sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    return prev_enabler;
}

int jobs_get_enabled() {
    return enabler;
}

int job_create(char *command) {
    // TO BE IMPLEMENTED
    if(strlen(command) == 0){
        printf("%s\n", "Wrong number of args (given: 0, required: 1) for command 'spool'");
        return -1;
    }else{
        int i=0;
        while(jobs_table[i].is_free == -1 && i<MAX_JOBS){
            i++;
        }

        if(i == MAX_JOBS){
            printf("%s\n", "Error: Spool");
            return -1;
        }else{
            char *cmd = malloc(sizeof(char) * (strlen(command)+1));
            strcpy(cmd, command);
            printf("TASK: %s\n", cmd);
            sf_job_create(i);
            sf_job_status_change(i, NEW, WAITING);
            fill_table_row(i, cmd, WAITING);
            free(command);
            if(enabler == 0 || active_runners == MAX_RUNNERS){
                return 0;
            }

            run_task(i, cmd);
        }
    }

    return 0;
}

void run_task(int jobid, char *command){
    int status;
    pid_t pid_runner,pid_master,pid_pipe;


    if((pid_runner = fork())  != 0){
        sigset_t mask, prev_mask;
        sigfillset(&mask);
        sigprocmask(SIG_BLOCK, &mask, &prev_mask);

        //Setting pgid of the runner
        set_table_row_pgid(jobid,pid_runner);
        sf_job_start(jobid, pid_runner);
        set_table_row_status(jobid, RUNNING);
        active_runners++;

        // Unmasking signals
        sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }else{

        sigprocmask(SIG_SETMASK, &prev_one, NULL);
        TASK *a = parse_task(&command);
        // struct sigaction act;
        // memset(&act, 0, sizeof(act));
        // act.sa_handler = SIG_IGN;
        // sigaction(SIGCHLD, &act, NULL);

        for(int i=0;i<MAX_JOBS;i++){
            if(jobs_table[i].is_free == -1){
                free(jobs_table[i].command);
            }
        }


        setpgid(0,0);
        pid_t grp_id = getpgrp();
        PIPELINE_LIST *pipeline_ptr = a->pipelines;
        while(pipeline_ptr != NULL){
            PIPELINE *first_pipe = pipeline_ptr->first;

            //Master process for running a task pipeline
            if((pid_master = fork()) == 0 ){
                setpgid(getpid(),grp_id);
                COMMAND_LIST *cmds_ptr = first_pipe->commands;
                int exit_status = 0;
                int exit_signal = 0;
                int pipe_new[2];
                int pipe_old[2];
                int count = 0;
                char *input_path = first_pipe->input_path;
                char *output_path = first_pipe->output_path;


                while(cmds_ptr != NULL){
                    COMMAND *cmd = cmds_ptr->first;
                    WORD_LIST *words_ptr= cmd->words;
                    WORD_LIST *temp = words_ptr;
                    int len = 0;
                    while(temp != NULL){
                        len++;
                        temp = temp->rest;
                    }
                    char **arguments = malloc(sizeof(char *) * (len+1));
                    int i = 0;
                    temp = words_ptr;
                    while(temp != NULL){
                        arguments[i] = temp->first;
                        i++;
                        temp = temp->rest;
                    }
                    arguments[i] = NULL;
                    pipe(pipe_new);
                    if((pid_pipe = fork()) == 0){
                        setpgid(getpid(),grp_id);
                        if(count !=0){
                            dup2(pipe_old[0],STDIN_FILENO);
                            close(pipe_old[0]);
                            close(pipe_old[1]);
                        }else{
                            if(input_path != NULL){
                                int fdin = open(input_path, O_RDONLY, 0644);
                                dup2(fdin, STDIN_FILENO);
                                close(fdin);
                            }
                        }
                        close(pipe_new[0]);
                        if(cmds_ptr->rest != NULL){
                            dup2(pipe_new[1],STDOUT_FILENO);
                        }else{
                            if(output_path != NULL){
                                int fdout = open(output_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
                                dup2(fdout, STDOUT_FILENO);
                                close(fdout);
                            }
                        }
                        close(pipe_new[1]);
                        if(execvp(arguments[0], arguments) < 0){
                            fprintf(stderr,"%s\n", "execvp failed: No such file or directory");
                            kill(getpid(),SIGABRT);
                        }
                    }

                    if(count != 0){
                        close(pipe_old[0]);
                        close(pipe_old[1]);
                    }

                    pipe_old[0] = pipe_new[0];
                    pipe_old[1] = pipe_new[1];

                    cmds_ptr = cmds_ptr->rest;
                    free(arguments);
                    count++;

                }

                close(pipe_old[0]);
                close(pipe_old[1]);

                while(waitpid(-1,&status,0) > 0){
                    if(WIFEXITED(status)){
                        exit_status = WEXITSTATUS(status);
                    }

                    if(WIFSIGNALED(status)){
                        // printf("%s\n", "Signal exited");
                        exit_signal = WTERMSIG(status);
                    }
                }

                if(exit_signal > 0){
                    free_task(a);
                    kill(getpid(),exit_signal);
                }

                free_task(a);
                exit(exit_status);
            }else{
                int exit_status = 0;
                int exit_signal = 0;
                waitpid(-1,&status,WUNTRACED);
                if(WIFEXITED(status)){
                    exit_status = WEXITSTATUS(status);
                }

                if(WIFSIGNALED(status)){
                    exit_signal = WTERMSIG(status);
                }

                if(exit_signal > 0){
                    kill(getpid(),exit_signal);
                    free_task(a);
                }

                if(pipeline_ptr->rest == NULL){
                    free_task(a);
                    exit(exit_status);
                }

            }

            pipeline_ptr = pipeline_ptr->rest;
        }

        waitpid(-1,&status,WUNTRACED);
        free_task(a);
        exit(0);
    }
}

int job_expunge(int jobid) {
    // TO BE IMPLEMENTED
    if (jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].is_free == -1 && (jobs_table[jobid].status == COMPLETED || jobs_table[jobid].status == ABORTED))
    {
        /* code */
        free(jobs_table[jobid].command);
        jobs_table[jobid].is_free = 1;
        jobs_table[jobid].status = -1;
        jobs_table[jobid].is_canceled = -1;
        sf_job_expunge(jobid);
        return 0;
    }else{
        printf("%s\n", "Error: expunge");
        return -1;
    }
}

int job_cancel(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid >= 0 && jobid < MAX_JOBS){
        if(jobs_table[jobid].is_free == -1){
            if(jobs_table[jobid].status == RUNNING || jobs_table[jobid].status == PAUSED){
                if(killpg(jobs_table[jobid].pgid, SIGKILL) == -1){
                    return -1;
                }
                jobs_table[jobid].is_canceled = 1;
                set_table_row_status(jobid,CANCELED);
            }else if(jobs_table[jobid].status == WAITING){
                jobs_table[jobid].is_canceled = 1;
                set_table_row_status(jobid,ABORTED);
            }
        }
        return 0;
    }else{
        printf("%s\n", "Error: cancel");
        return -1;
    }
}

int job_pause(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid >= 0 && jobid < MAX_JOBS && jobs_table[jobid].status == RUNNING){
        // printf("%d\n", jobs_table[jobid].pgid);
        if(killpg(jobs_table[jobid].pgid, SIGSTOP) == -1){
            return -1;
        }
        set_table_row_status(jobid,PAUSED);
        return 0;
    }else{
        printf("%s\n", "Error: pause");
    }
    return -1;
}

int job_resume(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid >= 0 && jobid < MAX_JOBS && jobs_table[jobid].status == PAUSED){
        if(killpg(jobs_table[jobid].pgid, SIGCONT) == -1){
            return -1;
        }
        set_table_row_status(jobid,RUNNING);
        return 0;
    }else{
        printf("%s\n", "Error: pause");
    }
    return 0;
}

int job_get_pgid(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].is_free == -1
        && (jobs_table[jobid].status == RUNNING || jobs_table[jobid].status == PAUSED || jobs_table[jobid].status == CANCELED)){
        return jobs_table[jobid].pgid;
    }
    return -1;
}

JOB_STATUS job_get_status(int jobid) {

    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].is_free == -1){
        return jobs_table[jobid].status;
    }
    return -1;
}

int job_get_result(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].status == COMPLETED){
        return jobs_table[jobid].exit_status;
    }
    return -1;
}

int job_was_canceled(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].status == ABORTED && jobs_table[jobid].is_canceled == 1){
        return 1;
    }

    return 0;
}

char *job_get_taskspec(int jobid) {
    // TO BE IMPLEMENTED
    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].is_free == -1){
        return jobs_table[jobid].command;
    }
    return NULL;
}

void print_jobs(){
    if(jobs_get_enabled() == 1){
        printf("%s\n","Starting jobs is enabled");
    }else{
        printf("%s\n","Starting jobs is disabled");
    }
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs_table[i].is_free == -1){
            printf("job %d [%s]: %s\n",i,job_status_names[jobs_table[i].status],jobs_table[i].command);
        }
    }
}

void print_job(int jobid){
    if(jobid>=0 && jobid<MAX_JOBS && jobs_table[jobid].is_free == -1){
        printf("job %d [%s]: %s\n",jobid,job_status_names[jobs_table[jobid].status],jobs_table[jobid].command);
    }
}
