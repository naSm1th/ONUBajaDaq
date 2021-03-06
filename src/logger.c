/*****************************************************************************
 * logger.c - control gathering and storing of metrics to csv file           *
 *                                                                           *
 * Contributors: Ryan Carl, Nathanael Smith                                  *
 *                                                                           *   
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>              /* snprintf */
#include <stdlib.h>             /* exit */
#include <string.h>             /* strcpy, strcat, strlen */
#include <sys/time.h>           /* time() */
#include <pthread.h>            /* threading */
#include <sys/stat.h>           /* stat */
#include <signal.h>             /* SIGTERM */
#include <unistd.h>             /* sleep() */
#include <gps.h>                /* gpsd */
#include <math.h>               /* fabs */
#include "usbdaq.h"
#include "serial.h"

#define MAX_GPS_LEN     100             /* max GPS string length */
#define MAX_TOKENS      13              /* max num tokens in GPS string */
#define LOG_DIR         "/mnt/bajadaq"  /* directory for logs (flash drive) */
#define NAME            "logger"        /* name of file for logging */

/* USB interface */
int run;                    // control flag for loop in usbdaq.c
int counts[8];              // shared array for sensor counts 
pthread_t thread;           // thread for usbdaq.c

struct gps_data_t gpsdata;  // values read from gps

/* string pointers are global so they can be freed before exit */
char *dirname;              // directory for session
FILE *outfp;                // output file pointer
char *filepath;             // full file path
char *gpstime, *gpsdate, *msstr;    // date and time
char *fdate;                // formatted date
char *latitude, *longitude; // coordinates
char *gpsstr;               // string to write to file

/* struct for accelerometer stuff */
struct accelAxes *accel;

/* PID of bash script - used for signaling to change LED color */
int bashPID;

void freeEverything() {
    if (filepath != NULL) free(filepath);
    if (dirname != NULL) free(dirname);
    if (gpsdate != NULL) free(gpsdate);
    if (gpstime != NULL) free(gpstime);
    if (gpsstr != NULL) free(gpsstr);
    if (outfp != NULL) free(outfp);
    if (msstr != NULL) free(msstr);
    if (latitude != NULL) free(latitude);
    if (longitude != NULL) free(latitude);
    if (accel != NULL) free(accel);
}

/* stops program and wraps up */
static void cleanup(int sig) {
    /* free all allocated memory */
    freeEverything();

    /* shutdown gps stream */
    gps_stream(&gpsdata, WATCH_DISABLE, NULL);
    gps_close(&gpsdata);

    /* close open files */
    fcloseall();
   
    /* set flag to stop */
    run = 0;
    
    /* join thread */
    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "%s: error in pthread_join", NAME);
        /* stop */
        raise(SIGINT);
    }

    fflush(stdout);
    exit(EXIT_SUCCESS);
}

void waitForGPS() {
    int rc;
    int fixed = 0;
    int timer = 0;
    if (!gps_waiting(&gpsdata, 5000000)) {
        /* timeout after 5 seconds */
        fprintf(stderr, "%s: gpsd not available\n", NAME);
        /* stop */
        raise(SIGINT);
    } else { 
        // printf("%s: waiting for fix...\n", NAME);
        /* wait for fix, timeout after 10 sec */
        while (!fixed && timer++ < 10) {
            /* read data */
            if ((rc = gps_read(&gpsdata)) < 0) {
                fprintf(stderr, "%s: code: %d, cause: %s\n", NAME, rc, gps_errstr(rc));
                /* stop */
                raise(SIGINT);
            } else {
                if ((gpsdata.status == STATUS_FIX) && gpsdata.fix.mode >= MODE_2D && !isnan(gpsdata.fix.latitude) && !isnan(gpsdata.fix.longitude)) {
                    //printf("%s: got the fix\n", NAME);
                    fixed = 1;
                } else ;
            }
            sleep(1);
        }
        if (!fixed)
            /* stop */
            raise(SIGINT);
    }
}

void processData(double *oldtime, double *newtime, int *fn, int first) {
    double interval;	        // interval for usb daq counts
    int cw;		                // fprintf return val
    /* initialize all pointers */
    gpsstr = (char *) malloc(MAX_GPS_LEN);
    gpsdate = (char *) malloc(7);
    gpstime = (char *) malloc(7);
    msstr = (char *) malloc(5);
    latitude = (char *) malloc(11);
    longitude = (char *) malloc(11);
    accel = malloc(sizeof(float) * 3);

    /* date and time */
    time_t ts = (time_t)gpsdata.fix.time;
    /* get decimal part */
    double ms = (double)gpsdata.fix.time-(int)ts;
    snprintf(msstr, 5, "%.2f", ms); 
    strftime(gpsdate, 7, "%d%m%y", localtime(&ts));
    strftime(gpstime, 7, "%H%M%S", localtime(&ts));
    fdate = ctime(&ts);
    /* GPS coordinates */
    snprintf(latitude, 11, "%s", (gpsdata.fix.latitude < 0) ? "-" : "+");
    snprintf(longitude, 11, "%s", (gpsdata.fix.longitude < 0) ? "-" : "+");
    snprintf(latitude+1, 10, "%02lf", fabs(gpsdata.fix.latitude));
    snprintf(longitude+1, 10, "%03lf", fabs(gpsdata.fix.longitude));
    /* leave off leading 0 of ms */
    sprintf(gpsstr,"%s%s,%s,%s,",gpstime,msstr+1,latitude,longitude);

    if (first) {
        dirname = (char *) malloc(15);
        strcpy(dirname,gpsdate);
        strcat(dirname,"_");
        strcat(dirname,gpstime);
        /* set time for new file check */ 
        *oldtime = (double)gpsdata.fix.time;
        *newtime = *oldtime;
        /* open new file */
        filepath = (char *) malloc(strlen(LOG_DIR)+strlen(dirname)+10);
        sprintf(filepath, "%s/%s/", LOG_DIR, dirname);
        /* make new directory */
        if (mkdir(filepath,0700)) {
            fprintf(stderr, "%s: log directory could not be created (USB might not be mounted)\n", NAME);
            /* stop */
            raise(SIGINT);
        }
        sprintf(filepath+strlen(filepath),"%03d.csv", *fn);
        outfp = fopen(filepath, "a");
        /* insert file header */
        cw = fprintf(outfp, "%s\nMetric:,Time,Latitude,Longitude,Speed,X-Accel,Y-Accel,Z-Accel\nUnits:,HHMMSS,DD.dddddd,DDD.dddddd,m/s,m/s^2,m/s^2\n\n", fdate);
        /* check for problem writing to flash drive */
        if (cw < 0) { 
            /* stop */
            raise(SIGINT);
        }
    } else {
        interval = (double)gpsdata.fix.time-*newtime;
        *newtime = (double)gpsdata.fix.time;
        if (*newtime - *oldtime >= 60.0) {
            *oldtime = *newtime;
            /* close old file */
            fclose(outfp);
            free(filepath);
            /* open new file */
            (*fn)++;
            filepath = (char *) malloc(strlen(LOG_DIR)+strlen(dirname)+10);
            sprintf(filepath, "%s/%s/%03d.csv", LOG_DIR, dirname, *fn);
            outfp = fopen(filepath, "a");
        }
    }

    /* blank first column */
    cw = fprintf(outfp, ",");
    /* check for problem writing to flash drive */
    if (cw < 0) { 
        /* stop */
        raise(SIGINT);
    }

    /* speed (knots to m/s) */
    double speed = gpsdata.fix.speed;
    speed = speed/1.9438445;
    snprintf(gpsstr+strlen(gpsstr),MAX_GPS_LEN-strlen(gpsstr),"%.1lf",speed);

    /* write formatted gps data to file */
    cw = fprintf(outfp, "%s", gpsstr);
    /* check for problem writing to flash drive */
    if (cw < 0) { 
        /* stop */
        raise(SIGINT);
    }

    /* accelerometer */
    if (readAccel(accel) != 0) {
        // printf("error in readAccel()\n");
    }
    /* write accel data to file */
    cw = fprintf(outfp, ",%lf,%lf,%lf", accel->x, accel->y, accel->z);
    /* check for problem writing to flash drive */
    if (cw < 0) { 
        /* stop */
        raise(SIGINT);
    }

    /* USB device (sensors) */
    int i;
    for (i = 0; i < 8; i++) {
        if (interval > 10e-4) {
            cw = fprintf(outfp, ",%lf", counts[i]/interval);
        } else {
            cw = fprintf(outfp, ",%lf", 0.0);
        }
        /* check for problem writing to flash drive */
        if (cw < 0) { 
            /* stop */
            raise(SIGINT);
        }
    }
    memset(counts, 0, sizeof(int)*8);

    cw = fprintf(outfp, "\n");
    /* check for problem writing to flash drive */
    if (cw < 0) { 
        /* stop */
        raise(SIGINT);
    }

    /* free allocated memory
       that isn't save for next iteration */
    free(gpsdate);
    free(gpstime);
    free(gpsstr);
    free(msstr);
    free(latitude);
    free(longitude);
    free(accel);
    /* initialize to NULL in case
       program is terminated before 
       initialization */
    gpsdate = NULL;
    gpstime = NULL;
    gpsstr = NULL;
    msstr = NULL;
    latitude = NULL;
    longitude = NULL;
    accel = NULL;
}

int main(int argc, char *argv[]) {
    double lasttime;
    int rc;
    int filenum = 1;            // current file number
    double newtime, oldtime;    // times for comparison
    int first = 1;              // boolean for first time
    
    /* initialize global pointers */
    dirname = NULL;
    outfp = NULL;
    filepath = NULL;
    gpstime = NULL;
    gpsdate = NULL;
    msstr = NULL;
    fdate = NULL;
    latitude = NULL;
    longitude = NULL;
    gpsstr = NULL;
    accel = NULL;

    /* store PID of bash script to signal later */
    if (argc < 2) {
        exit(1);
    }
    bashPID = atoi(argv[1]);

    run = 1;
    /* catch ctrl-C */
    struct sigaction sa;
    sa.sa_handler = cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "%s: problem with SIGINT catch", NAME);
        /* stop */
        raise(SIGINT);
    }

    /* thread USB daq */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* create parameter struct */
    struct usbParams params;
    params.counts = counts;
    params.run = &run;

    if (pthread_create(&thread, &attr, initUSBDaq, &params)) {
        fprintf(stderr, "%s: error in pthread_create", NAME);
        /* stop */
        raise(SIGINT);
    }   
    /* destroy thread */
    pthread_attr_destroy(&attr);

    /* initialize serial */
    if (initSerial()) {
        fprintf(stderr, "%s: initSerial failed\n", NAME);
        /* stop */
        raise(SIGINT);
    }

    if ((rc = gps_open("localhost", "2947", &gpsdata)) == -1) {
        fprintf(stderr, "%s: code: %d, reason: %s\n", NAME, rc, gps_errstr(rc));
        /* stop */
        raise(SIGINT);
    }
    gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
  
    /* wait for gps fix */
    waitForGPS();

    /* tell bash/python we are ready; update LED color */
    kill(bashPID, SIGUSR1);

    while (1) {
        // printf("%s: logging...\n", NAME);
        if (!gps_waiting(&gpsdata, 5000000)) {
            /* timeout after 5 seconds */
            fprintf(stderr, "%s: gpsd not available\n", NAME);
            /* stop */
            raise(SIGINT);
        } else { 
            /* empty buffer on first time */
            while ((rc = gps_read(&gpsdata)) > 0 && first) {
                ;
            }
            if (rc < 0) {
                /* read error */
                fprintf(stderr, "%s: code: %d, cause: %s\n", NAME, rc, gps_errstr(rc));
                /* stop */
                raise(SIGINT);
            } else {
                /* check gps fix */
                if ((gpsdata.status != STATUS_FIX) || gpsdata.fix.mode < MODE_2D || isnan(gpsdata.fix.latitude) || isnan(gpsdata.fix.longitude)) {
                    /* gps is invalid */
                    fprintf(stderr, "%s: code: %d, cause: %s\n", NAME, rc, gps_errstr(rc));
                    waitForGPS();
                }

                /* throw out duplicates */
                if ((double)gpsdata.fix.time != lasttime) {
                    /* process GPS data; write to file */
                    processData(&oldtime, &newtime, &filenum, first);
                    if (first) first = 0;
                }
                /* save time for duplicate GPS data check */
                lasttime = (double)gpsdata.fix.time;
            }
        }
    }
    return EXIT_SUCCESS;
}
