/*****************************************************************************
 * logger.c - control gathering and storing of metrics to csv file           *
 *                                                                           *
 * Author: Ryan Carl - rpcarl@olivet.edu                                     *
 *                                                                           *   
 * format: metric_name metric_value timestamp                                *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/

#include <stdio.h>              /* printf, snprintf */
#include <stdlib.h>             /* exit */
#include <string.h>             /* strcpy, strcat, strlen */
#include <sys/time.h>           /* gettimeofday */
#include <pthread.h>            /* threading */

#define MAX_READ    50          /* max num chars to read */
#define MAX_LEN     100         /* max GPS string length */
#define MAX_TOKENS  13          /* max num tokens in GPS string */

int counts[8];

void *initUSBDaq(void *array);

/* test threading
void *setVals(void *array) {
    int *newArray = array; int i;
    for (i = 0; i < 8; i++) {
        newArray[i] = i;
    }   
}
*/

void writeToFile() {

}

char *waitForSerial() {
    return "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
}

char *formatMetric(int *values, int numVal, char *time) {
    /*
    // POSIX
    struct timeval t_p;
    gettimeofday(&t_p, NULL);
    snprintf(metric, MAX_LEN, "%s %i %lu:%lu", name, value, (long int)t_p.tv_sec, (long int)t_p.tv_usec) ;
    // ANSI C
    time_t t = time(NULL);
    struct time tm = *localtime(&t);
    */
    char *ptr = (char *) malloc(MAX_LEN);
    int i;
    int offset = snprintf(ptr, MAX_LEN, "%s,", time);
    for (i = 0; i < numVal; i++) {
        offset += snprintf(ptr+offset, MAX_LEN, "%i,", *values++);
    }
    snprintf(ptr+offset-1, MAX_LEN, "\0");
    return ptr;
}

main(int argc, char *argv[]) {
    /* file pointer for output */
    FILE *fp;
    int filenum = 1;
    char filename[8]; 

    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&thread, &attr, initUSBDaq,(void *)&counts)) {
        printf("ERROR in pthread_create()");
        exit(1);
    }   

    pthread_attr_destroy(&attr);
    if (pthread_join(thread, NULL)) {
        printf("ERROR in pthread_join()");
    }

    // open new file
    sprintf(filename, "%03d.csv", filenum);
    fp = fopen(filename, "a+");

    char *rawgps = (char *) malloc(MAX_LEN);
    strcpy(rawgps, waitForSerial());
    char *csum_str = (char *) malloc(strlen(rawgps));
    // copy for checksum comparison (omit $)
    strcpy(csum_str, rawgps+1);
    char **gpstok = (char **) malloc(MAX_TOKENS*sizeof(char *));
    *gpstok = strtok(rawgps, ",");    /* split string */
    int i = 0;
    while (gpstok[i] != NULL) {
        //printf("%s\n", gpstok[i]);
        gpstok[++i] = strtok(NULL, ",");
    }

    // check sentence type
    if (strcmp(gpstok[0], "$GPRMC") == 0) {
        // checksum calc
        int csum = 0;
        int csum_calc = 0;
        if (csum = (int)strtol(gpstok[11]+2, NULL, 16)) {
            int j, k;
            for (j = 0; j < strlen(csum_str); j++) {
                // skip checksum
                if (csum_str[j] != '*')
                    csum_calc = csum_calc ^ csum_str[j];
                // exit if at checksum
                else
                    break;
                //printf("%c\n", csum_str[j]);
            }
            //printf("%i = %i\n", csum_calc, csum);
        }
        // if checksum is valid, keep values
        if (csum_calc == csum) {
            // parse gps string
            char *gpsstr = (char *) malloc(MAX_LEN);
            // date and time
            strcpy(gpsstr,gpstok[1]);
            strcat(gpsstr,",");
            // if valid fix, keep coordinates
            if (strcmp(gpstok[2],"A") == 0) {
                snprintf(gpsstr+strlen(gpsstr),MAX_LEN-strlen(gpsstr),"%s %s,%s %s,",gpstok[3],gpstok[4],gpstok[5],gpstok[6]);
            } else {
                sprintf(gpsstr+strlen(gpsstr),",,");
            }
            double speed = 0;
            sscanf(gpstok[7], "%lf", &speed);
            speed = speed*6076.0/5280.0;
            snprintf(gpsstr+strlen(gpsstr),MAX_LEN-strlen(gpsstr),"%lf",speed);
            printf("%s", gpsstr);
            // format for google maps: +40  42.6142', -74  00.4168'
        } else {
            // invalid checksum
        }
    }

    // read counts from USBDaq

    // free allocated memory
    free(rawgps);
    free(gpstok);

    fclose(fp);
    /* close thread */
    pthread_exit(NULL);
}
