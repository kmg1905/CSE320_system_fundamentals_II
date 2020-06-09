#include <signal.h>
#include <wait.h>
#include <stdlib.h>

#include "signal_handlers.h"
#include "helper_functions.h"

void sigchld_handler(int sig){
    sig_child_flag = 1;
}

void sigint_handler(int sig){
    sig_int_flag = 1;
}

int handler(){
    int status;
    if(sig_child_flag == 1){
        sigset_t mask, prev_mask;
        sigfillset(&mask);
        sigprocmask(SIG_BLOCK, &mask, &prev_mask);
        pid_t runner;
        while((runner = waitpid(-1,&status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){
            int i=0;
            while(i<MAX_JOBS && jobs_table[i].pgid != runner){
                i++;
            }

            if(i>=MAX_JOBS){
                return 0;
            }

            if(WIFSTOPPED(status)){
                sf_job_pause(i,runner);
                sigprocmask(SIG_SETMASK, &prev_mask, NULL);
                return 0;
            }

            if(WIFCONTINUED(status)){
                sf_job_resume(i,runner);
                sigprocmask(SIG_SETMASK, &prev_mask, NULL);
                return 0;
            }

            if(WIFEXITED(status)){
                jobs_table[i].exit_status = status;
                // printf("Exit status:%d\n", jobs_table[i].exit_status);
                sf_job_end(i, runner, status);
                set_table_row_status(i, COMPLETED);
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

        if(enabler == 1){
            jobs_set_enabled(1);
        }

        sig_child_flag = 0;
        sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }

    return 0;
}