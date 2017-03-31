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
#include "usbdaq.h"
#include "serial.h"

#define MAX_GPS_LEN     100             /* max GPS string length */
#define MAX_TOKENS      13              /* max num tokens in GPS string */
#define LOG_DIR         "/mnt/bajadaq"  /* directory for logs (flash drive) */
#define LOG_LEVEL       "logger"      /* name of file for logging */

int initSerial(int *);

int run;            // flag to run loop in usbdaq.c
int counts[8];      // usbdaq shared array
char *dirname;      // directory for session
pthread_t thread;   // thread for usbdaq
int serfd;          // serial file
FILE *outfp;        // output file pointer
int filenum = 1;    // current file
char *filepath;     // full file path
int first = 1;      // boolean for first time
struct tm tm;       // time for str to time 
time_t newtime, oldtime;    // times for comparison
char *gpstime, *gpsdate;  // date and time
char *latitude, *longitude; // coordinates
char *rawgps;       // unparsed string
char **gpstok;      // parsed string
char **serin;       // lines from serial input
char *csum_str;     // checksum check
char *gpsstr;       // string to write to file
double interval;	// interval for usb daq counts
int cw;		        // fprintf return val

/* temporary */
char *waitForSerial() {
    return "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
}

void freeEverything() {
    free(filepath);
    free(dirname);
    free(gpsdate);
    free(gpstime);
    free(latitude);
    free(longitude);
    free(gpsstr);
    free(gpstok);
    free(csum_str);
    free(rawgps);
    free(serin);
    free(outfp);
}

/* gets input from serial
 * returns number of lines read
 */
int getSerial(int fd, char **lines) {
    char line[256]; 
    int cr, n;

    n = 0;
    
    if (fd == -1)
        /* free allocated memory */
        free(rawgps);
        free(gpstok);
        free(serin);
        /* exit */
        raise(SIGINT);
        return fd;
    while ((cr = read(fd, (void*)line, sizeof(line)-1)) > 0) {
        line[cr] = '\0';
        printf("%s", line);
        *lines++ = line;
        n++;
    }
    if (cr < 0) {
        fprintf(stderr, "%s: error in getSerial", LOG_LEVEL);
        /* free allocated memory */
        free(rawgps);
        free(gpstok);
        free(serin);
        /* exit */
        raise(SIGINT);
        return cr;
    }
    return n;
}

/* stops program and wraps up */
static void cleanup(int sig) {
    closeSerial(serfd);
    fcloseall();
    freeEverything();
    /* set flag to exit */
    run = 0;
    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "%s: error in pthread_join", LOG_LEVEL);
    }
    fflush(stdout);
    exit(0);
}


int main(int argc, char *argv[]) {
    run = 1;        // flag to continue logging USBdaq data

    // catch ctrl-C 
    struct sigaction sa;
    sa.sa_handler = cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGINT, &sa, NULL) == -1)
        fprintf(stderr, "%s: problem with SIGINT catch", LOG_LEVEL);

    /* thread USB daq */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* create parameter struct */
    struct usbParams params;
    params.counts = counts;
    params.run = &run;

    if (pthread_create(&thread, &attr, initUSBDaq, &params)) {
        fprintf(stderr, "%s: error in pthread_create", LOG_LEVEL);
        exit(1);
    }   
    /* destroy thread */
    pthread_attr_destroy(&attr);

    /* initialize serial */
    if (initSerial(&serfd)) {
        // success
        printf("%s: initSerial\n", LOG_LEVEL);
    } else {
        // failure
        printf("%s: initSerial failed\n", LOG_LEVEL);
        // check error type
        raise(SIGINT);
    }
  
    while (1) {
        printf("%s: logging...\n", LOG_LEVEL);
        rawgps = (char *) malloc(MAX_GPS_LEN);
        gpstok = (char **) malloc(MAX_TOKENS*sizeof(char *)); 
        serin = (char **) malloc(10*sizeof(char *));
        /* wait for input from GPS */
        int n = getSerial(serfd, serin);
        //serin[0] = (char *) malloc(MAX_GPS_LEN);
        //strcpy(serin[0], waitForSerial());
        //int n = 1;
        int i = 0;
        /* loop through strings from serial */
        while (i < n) {
            strcpy(rawgps, serin[i++]);
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
                        gpsstr = (char *) malloc(MAX_GPS_LEN);
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
                            first = 0; 
                            // set time for update
                            oldtime = mktime(&tm);
                            newtime = mktime(&tm);
                            // open new file
                            filepath = (char *) malloc(strlen(LOG_DIR)+strlen(dirname)+10);
                            //sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, filenum);
                            sprintf(filepath, "%s/%s/", LOG_DIR, dirname);
                            // make new directory
                            if (mkdir(filepath,0700))
                                fprintf(stderr, "%s: log directory could not be created", LOG_LEVEL);
                            sprintf(filepath+strlen(filepath),"%03d.csv", filenum);
                            outfp = fopen(filepath, "a");
                            // insert file header
                            cw = fprintf(outfp, "%s Logging Session\nStart time:,%s\n", gpsdate, gpstime);
                            if (cw < 0) { // problem writing to flash drive
                                /* free allocated memory */
                                freeEverything();
                                /* exit */
                                raise(SIGINT);
                            }
                            // format for google maps: +40  42.6142', -74  00.4168'
                            cw = fprintf(outfp, "Copy and paste lat and long cells separated by commas into a Google search bar to verify starting coordinates\n");
                            if (cw < 0) { // problem writing to flash drive
                                /* free allocated memory */
                                freeEverything();
                                /* exit */
                                raise(SIGINT);
                            }
                        } else {
                            strcat(gpsdate,gpstime);
                            strptime(gpsdate, "%d%m%y%H%M%S", &tm);
                            interval = difftime(mktime(&tm),newtime);
                            newtime = mktime(&tm);
                            if (difftime(newtime,oldtime) > 60) {
                                oldtime = mktime(&tm);
                                // close old file
                                fclose(outfp);
                                free(filepath);
                                // open new file
                                filenum++;
                                filepath = (char *) malloc(strlen(LOG_DIR)+strlen(dirname)+10);
                                sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, filenum);
                                outfp = fopen(filepath, "a");
                            }
                        }
                        /* speed (knots to mph) */
                        double speed = 0;
                        sscanf(gpstok[7], "%lf", &speed);
                        speed = speed*6076.0/5280.0;
                        snprintf(gpsstr+strlen(gpsstr),MAX_GPS_LEN-strlen(gpsstr),"%.1lf",speed);
                        /* write to file */
                        cw = fprintf(outfp, "%s", gpsstr);
                        if (cw < 0) { // problem writing to flash drive
                            /* free allocated memory */
                            freeEverything();
                            /* exit */
                            raise(SIGINT);
                        }
                        // read counts from USBDaq
                        if (interval > 10e-4) {
                            for (i = 0; i < 8; i++) {
                                cw = fprintf(outfp, ",%lf", counts[i]/interval);
                                if (cw < 0) { // problem writing to flash drive
                                    /* free allocated memory */
                                    freeEverything();
                                    /* exit */
                                    raise(SIGINT);
                                }
                            }
                            memset(counts, 0, sizeof(int)*8);
                        }
                        cw = fprintf(outfp, "\n");
                        if (cw < 0) { // problem writing to flash drive
                            /* free allocated memory */
                            freeEverything();
                            /* exit */
                            raise(SIGINT);
                        }
                        /* free allocated memory */
                        free(gpsdate);
                        free(gpstime);
                        free(latitude);
                        free(longitude);
                        free(gpsstr);
                        free(csum_str);
                    } else {
                        // invalid checksum
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
                free(serin);
            }
        }
        // for testing
        sleep(1);
    }
}
