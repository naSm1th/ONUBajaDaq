/*****************************************************************************
 * logger.c - control gathering and storing of metrics to csv file           *
 *                                                                           *
 * Author: Ryan Carl - rpcarl@olivet.edu                                     *
 *                                                                           *   
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>              /* printf, snprintf */
#include <stdlib.h>             /* exit */
#include <string.h>             /* strcpy, strcat, strlen */
#include <sys/time.h>           /* time() */
#include <pthread.h>            /* threading */
#include <sys/stat.h>           /* stat */
#include <signal.h>             /* SIGTERM */
#include <math.h>               /* fabs */
#include <unistd.h>             /* sleep() */

#define MAX_LEN     100         /* max GPS string length */
#define MAX_TOKENS  13          /* max num tokens in GPS string */
#define LOG_DIR     "."         /* directory for logs (flash drive) */

int counts[8];      // usbdaq shared array
char *dirname;      // directory for session
pthread_t thread;   // thread for usbdaq

void *initUSBDaq(void *array);

// temp var to test NMEA strings
int randomnum;
char *waitForSerial() {
    printf("getting GPS coordinates...\n");
    switch (randomnum) {
        case 0:
            randomnum = 1;
            return "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
            break;
        case 1:
            randomnum = 2;
            return "$GPRMC,084836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*67";
            break;
        case 2:
            randomnum = 3;
            return "$GPRMC,092836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*60";
            break;
        case 3:
            randomnum = 4;
            return "$GPRMC,093836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*61";
            break;
        default:
            return "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
            break;
    }
}

static void cleanup(int sig) {
    printf("\nLogging session terminated\n");
    free(dirname);
    fcloseall();
    /* close thread */
    pthread_exit(NULL);
    /* raise signal */
    signal(sig, SIG_DFL);
    raise(sig);
    /* wait for thread */
    if (pthread_join(thread, NULL)) {
        printf("ERROR in pthread_join()");
    }

    fflush(stdout);
    exit(0);
}

int main(int argc, char *argv[]) {
    FILE *fp;           // output file pointer
    int filenum = 1;    // current file
    char *filepath;     // full file path
    int first = 1;      // boolean for first time
    struct tm tm;       // time for str to time 
    time_t newtime, oldtime;    // times for comparison
    char *gpstime, *gpsdate;  // date and time
    char *latitude, *longitude; // coordinates
    char *rawgps;       // unparsed string
    char **gpstok;      // parsed string
    char *csum_str;     // checksum check
    char *gpsstr;       // string to write to file
    double interval;

    // catch ctrl-C 
    struct sigaction sa;
    sa.sa_handler = cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGINT, &sa, NULL) == -1)
        perror("Problem with SIGINT catch");

    // random for testing
    randomnum = 0;
    
    /* thread USB daq */
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&thread, &attr, initUSBDaq,(void *)&counts)) {
        printf("ERROR in pthread_create()");
        exit(1);
    }   
    /* destroy thread */
    //pthread_attr_destroy(&attr);
  
    while (1) {
        rawgps = (char *) malloc(MAX_LEN);
        gpstok = (char **) malloc(MAX_TOKENS*sizeof(char *)); 
        /* wait for input from GPS */
        strcpy(rawgps, waitForSerial());
        if (rawgps && strlen(rawgps) > 0) {
            csum_str = (char *) malloc(strlen(rawgps));
            /* copy for checksum comparison (omit $) */
            strcpy(csum_str, rawgps+1);
            /* split GPS string */
            *gpstok = strtok(rawgps, ",");
            int i = 0;
            while (gpstok[i] != NULL) {
                //printf("%s\n", gpstok[i]);
                gpstok[++i] = strtok(NULL, ",");
            }

            /* check sentence type */
            if (strcmp(gpstok[0], "$GPRMC") == 0) {
                /* checksum calculations */
                int csum = 0;
                int csum_calc = 0;
                if ((csum = (int)strtol(gpstok[11]+2, NULL, 16))) {
                    int j;
                    for (j = 0; j < strlen(csum_str); j++) {
                        // skip checksum
                        if (csum_str[j] == '*')
                            break;
                        csum_calc = csum_calc ^ csum_str[j];
                    }
                }
                /* if checksum is valid, process GPS string */
                if (csum_calc == csum) {
                    gpsstr = (char *) malloc(MAX_LEN);
                    // date and time
                    snprintf(gpsstr,8,"%s,",gpstok[1]);
                    gpsdate = (char *) malloc(13);
                    gpstime = (char *) malloc(10);
                    strcpy(gpsdate,gpstok[9]);
                    strcpy(gpstime,gpstok[1]);
                    // GPS coordinates 
                    latitude = (char *) malloc(12);
                    longitude = (char *) malloc(13);
                    (strcmp(gpstok[4],"N") == 0) ? strcpy(latitude, "+") : strcpy(latitude, "-");
                    (strcmp(gpstok[6],"E") == 0) ? strcpy(longitude, "+") : strcpy(longitude, "-");
                    strncat(latitude, gpstok[3], 2);
                    strncat(longitude, gpstok[5], 3);
                    strcat(latitude, " ");
                    strcat(longitude, " ");
                    strncat(latitude, gpstok[3]+2, 7);
                    strncat(longitude, gpstok[5]+3, 7);
                    // if valid fix, keep coordinates
                    if (strcmp(gpstok[2],"A") == 0) {
                        sprintf(gpsstr+strlen(gpsstr),"%s,%s,",latitude,longitude);
                    } else {
                        sprintf(gpsstr+strlen(gpsstr),",,");
                    }
                    /* first time for this logging session */
                    if (first) {
                        /* date_time: ddmmyy_hhmmss */
                        dirname = (char *) malloc(15);
                        strcpy(dirname,gpsdate);
                        strcat(dirname,"_");
                        strncat(dirname,gpstime,6);
                        strptime(dirname, "%d%m%y_%H%M%S", &tm);
                        struct stat st = {0};
                        // make new directory
                        if (stat(dirname, &st) == -1) {
                            mkdir(dirname,0700);
                        }
                        first = 0; 
                        // set time for update
                        oldtime = mktime(&tm);
                        newtime = mktime(&tm);
                        // open new file
                        filepath = (char *) malloc(25);
                        sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, filenum);
                        fp = fopen(filepath, "a");
                        // insert file header
                        fprintf(fp, "%s Logging Session\nStart time:,%s\n", gpsdate, gpstime);
                        // format for google maps: +40  42.6142', -74  00.4168'
                        fprintf(fp, "Copy and paste lat and long cells separated by commas into a Google search bar to verify starting coordinates\n");
                    } else {
                        strcat(gpsdate,gpstime);
                        strptime(gpsdate, "%d%m%y%H%M%S", &tm);
                        interval = difftime(mktime(&tm),newtime);
                        newtime = mktime(&tm);
                        if (difftime(newtime,oldtime) > 60) {
                            oldtime = mktime(&tm);
                            // close old file
                            fclose(fp);
                            free(filepath);
                            // open new file
                            filenum++;
                            filepath = (char *) malloc(25);
                            sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, filenum);
                            fp = fopen(filepath, "a");
                        }
                    }
                    /* speed (knots to mph) */
                    double speed = 0;
                    sscanf(gpstok[7], "%lf", &speed);
                    speed = speed*6076.0/5280.0;
                    snprintf(gpsstr+strlen(gpsstr),MAX_LEN-strlen(gpsstr),"%.1lf",speed);
                    /* write to file */
                    fprintf(fp, "%s", gpsstr);
                    // read counts from USBDaq
                    if (fabs(interval) > 10e-4) {
                        for (i = 0; i < 8; i++) {
                            fprintf(fp, ",%lf", counts[i]/interval);
                            counts[i] = 0; // reset counts
                        }
                    }
                    fprintf(fp, "\n");
                    /* free allocated memory */
                    free(gpsdate);
                    free(gpstime);
                    free(latitude);
                    free(longitude);
                    free(gpsstr);
                    free(csum_str);
                } else {
                    // invalid checksum
                    // TODO: handle invalid checksum 
                    free(rawgps);
                    free(gpstok);
                    free(csum_str);
                }
            } else {
                // format we don't care about
                ;
            }
        } else { // invalid serial string 
            free(rawgps);
        }
        // for testing
        sleep(1);
    }
}
