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
#include <gps.h>                /* gpsd */
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
char *gpsstr;       // string to write to file
double interval;	// interval for usb daq counts
int cw;		        // fprintf return val
struct gps_data_t gpsdata;

/* temporary */
char *waitForSerial() {
    return "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
}

void freeEverything() {
    free(filepath);
    free(dirname);
    free(gpsdate);
    free(gpstime);
    //free(latitude);
    //free(longitude);
    free(gpsstr);
    free(outfp);
}

/* gets input from serial
 * returns number of lines read
 */
int getSerial(int fd, char **lines) {
    char line[256]; 
    int cr, n;

    n = 0;
    
    if (fd == -1) {
        /* exit */
        raise(SIGINT);
        return fd;
    }
    while ((cr = read(fd, (void*)line, sizeof(line)-1)) > 0) {
        if (!strcmp(line,"") || line[0] == '\n') break;
        line[cr] = '\0';
        *lines++ = line;
        n++;
    }
    if (cr < 0) {
        fprintf(stderr, "%s: error in getSerial", LOG_LEVEL);
        /* exit */
        raise(SIGINT);
        return cr;
    }
    return n;
}

/* stops program and wraps up */
static void cleanup(int sig) {
    closeSerial(serfd);
    gps_stream(&gpsdata, WATCH_DISABLE, NULL);
    gps_close(&gpsdata);
    fcloseall();
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
        // failure
        fprintf(stderr, "%s: initSerial failed\n", LOG_LEVEL);
        // check error type
        raise(SIGINT);
    }

    int rc;
    if ((rc = gps_open("localhost", "2947", &gpsdata)) == -1) {
        fprintf(stderr, "%s: code: %d, reason: %s\n", LOG_LEVEL, rc, gps_errstr(rc));
        raise(SIGINT);
    }
    gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
  
    while (1) {
        printf("%s: logging...\n", LOG_LEVEL);
        int fixed = 0;
        /* wait for 2 seconds to receive gps data */
        if (gps_waiting(&gpsdata, 2000000)) {
            while (!fixed) {
                /* read data */
                if ((rc = gps_read(&gpsdata)) < 0) {
                    printf("%s: code: %d, reason: %s\n", LOG_LEVEL, rc, gps_errstr(rc));
                    raise(SIGINT);
                } else {
                    if ((gpsdata.status == STATUS_FIX) && (gpsdata.fix.mode == MODE_2D || gpsdata.fix.mode == MODE_3D) && !isnan(gpsdata.fix.latitude) && !isnan(gpsdata.fix.longitude)) {
                        printf("%s: got the fix\n", LOG_LEVEL);
                        fixed = 1;
                    } else {
                        // no fix
                        printf("%s: waiting for fix...\n", LOG_LEVEL);
                    }
                }
                sleep(1);
            }
        }
        /* checksum calc
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
        */
        gpsstr = (char *) malloc(MAX_GPS_LEN);
        // date and time
        time_t ts = (time_t)gpsdata.fix.time;
        gpsdate = (char *) malloc(13);
        gpstime = (char *) malloc(10);
        strftime(gpsdate, 6, "%d%m%y", localtime(&ts));
        strftime(gpstime, 9, "%H%M%S", localtime(&ts));
        printf("time from gps: %s_%s\n", gpsdate, gpstime);
        // GPS coordinates 
        latitude = (char *) malloc(12);
        longitude = (char *) malloc(13);
        /*
        (!strcmp(gpstok[4],"N")) ? strcpy(latitude, "+") : strcpy(latitude, "-");
        (!strcmp(gpstok[6],"E")) ? strcpy(longitude, "+") : strcpy(longitude, "-");
        strncat(latitude, gpstok[3], 2);
        strncat(longitude, gpstok[5], 3);
        strcat(latitude, " ");
        strcat(longitude, " ");
        strncat(latitude, gpstok[3]+2, 7);
        strncat(longitude, gpstok[5]+3, 7);
        */
        //sprintf(gpsstr+strlen(gpsstr),"%s,%s,",latitude,longitude);
        /*
        } else {
            sprintf(gpsstr+strlen(gpsstr),",,");
        }
        */
        if (first) {
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
            sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, filenum);
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
        //sscanf(gpsdata.fix.speed, "%lf", &speed);
        //speed = speed*6076.0/5280.0;
        //snprintf(gpsstr+strlen(gpsstr),MAX_GPS_LEN-strlen(gpsstr),"%.1lf",speed);
        /* write to file */
        //cw = fprintf(outfp, "%s", gpsstr);
        if (cw < 0) { // problem writing to flash drive
            /* free allocated memory */
            freeEverything();
            /* exit */
            raise(SIGINT);
        }
        // read counts from USBDaq
        cw = 1;
        if (interval > 10e-4) {
            int i;
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
    }
}
