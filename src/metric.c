/*
    Author: Ryan Carl
    Project: ONU Baja DAQ

    metric.c - control gathering and storing of metrics to csv file

    format: metric_name metric_value timestamp
*/

#include <stdio.h>              /* printf, fgets */
#include <stdlib.h>             /* malloc, execvp, wait */

#define READ        0                /* reader fd */
#define WRITE       1                /* writer fd */
#define EXEC_ERROR -1                /* execvp error code */
#define SERIAL  "serial"
#define MCC     "mcc"

main(int argc, char *argv[]) {
    int childp[2];
    int parentp[2];

    if(pipe(parentp) || pipe(childp)) {
        perror("pipe");
        exit(1);
    }

    int pid_sfork = fork();
    if (pid_sfork == -1) {
        perror("fork");
        exit(1); 
    } else if (pid_sfork == 0) { /* child: serial */
        int in, out;
        in = childp[READ];
        out = parentp[WRITE];
    
        execvp(SERIAL);    /* start serial reader program */
        fprintf(stderr, "metric.c: %s could not be executed\n", SERIAL);
        exit(EXEC_ERROR);
    } else {        /* parent: logger */
        //close(fd[WRITE]);           /* Close unused end */
        //dup2(fd[READ], 0);          /* Duplicate used end to stdin */
        //close(fd[READ]);            /* Close original used end */
        //execvp(rargs[0], rargs);    /* Execute reader program */
        //fprintf(stderr, "metric.c: %s could not be executed\n", );
        //exit(CMDNF);
        int in, out;
        in = parentp[READ];
        out = childp[WRITE];

        int pid_mfork = fork();
        if (pid_mfork == -1) {
            perror("fork");
            exit(1); 
        } else if (pid_mfork == 0) { /* child: mcc */
            int in, out;
            in = childp[READ];
            out = parentp[WRITE];
        
            execvp(MCC);    /* start mcc reader program */
            fprintf(stderr, "metric.c: %s could not be executed\n", MCC);
            exit(EXEC_ERROR);
        } else {        /* parent: logger */
            
        }
    }
}
