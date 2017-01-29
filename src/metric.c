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
#define MCI     "mci"

main(int argc, char *argv[]) {
    int serial[2];
    int logger_s[2];
    int mci[2];
    int logger_m[2];

    if(pipe(serial) || pipe(logger_s)) {
        perror("pipe");
        exit(1);
    }

    int pid_sfork = fork();
    if (pid_sfork == -1) {
        perror("fork_serial");
        exit(1); 
    } else if (pid_sfork == 0) { /* child: serial */
        int in_s, out_s;
        in_s = serial[READ];
        out_s = logger_s[WRITE];
        close(logger_s[READ]); /* close unused ends */
        close(serial[WRITE]);
        dup2(in_s,0);
        dup2(out_s,1);
    
        execvp(SERIAL);    /* start serial program */
        fprintf(stderr, "metric.c: %s could not be executed\n", SERIAL);
        exit(EXEC_ERROR);
    } else {        /* parent: logger */
        int to_s, from_s;
        from_s = logger_s[READ];
        to_s = serial[WRITE];
        close(logger_s[WRITE]); /* close unused ends */
        close(serial[READ]);

        int pid_mfork = fork();
        if (pid_mfork == -1) {
            perror("fork_mci");
            exit(1); 
        } else if (pid_mfork == 0) { /* child: mci */
            int in_m, out_m;
            in_m = mci[READ];
            out_m = logger_m[WRITE];
            close(logger_m[READ]); /* close unused ends */
            close(mci[WRITE]);
            dup2(in_m,0);
            dup2(out_m,1);
        
            execvp(MCI);    /* start mci program */
            fprintf(stderr, "metric.c: %s could not be executed\n", MCI);
            exit(EXEC_ERROR);
        } else {        /* parent: logger */
            int to_m, from_m;
            from_m = logger_m[READ];
            to_m = mci[WRITE];
            close(logger_m[WRITE]); /* close unused ends */
            close(mci[READ]);

            /* 
               wait for serial input
               poll mci
               write to file
            */
        }
    }
}
