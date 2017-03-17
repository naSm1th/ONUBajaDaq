/*****************************************************************************
 * usbdaq.c - methods to interface with the MCC USB-201 interface            *
 *                                                                           *
 * Author: Nathanael A. Smith - nasmith@olivet.edu                           *
 *                                                                           *
 * This file is part of the Baja DAQ System for Olivet Nazarene University   *
 * Copyright 2017 Nathanael A. Smith & Ryan Carl                             *
 * License: MIT License (see LICENSE for more details)                       *
 *                                                                           *
 *                                                                           *
 * This file contains code copied from the MCC Linux Drivers library         *
 * example files by Warren J. Jasper (released under the GNU LGPL License).  *
 * Copyright 2014 Warren J. Jasper <wjasper@tx.ncsu.edu>                     *
 * This library can be found at https://github.com/wjasper/Linux_Drivers     *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <libusb/pmd.h>
#include <libusb/usb-20X.h>

#include "usbdaq.h"

/* the pointer to the shared int array for DIO */
//int *counts;
/* the pointer to the shared flag for looping */
//int *run;
/* the handle to the udev device */
libusb_device_handle *udev = NULL;
/* tracks if we are currently handling a signal */
volatile sig_atomic_t sigInProgress = 0;

/* initializes and runs the process for the USB DAQ interface */
void *initUSBDaq(void *in) {
    int *counts, *run;
    struct usbParams *params = in;
    /* save reference to our array of counts */
    counts = params->counts;
 
    /* save reference to our run flag */
    run = params->run;

    /* block SIGINT in this thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL)) {
        perror("Error in pthread_sigmask()\n");
    }

    /* initialize libusb (required for MCC-USB library */
    int ret = libusb_init(NULL);
    if (ret < 0) {
        perror("usb_device_find_USB_MCC: Failed to initialize libusb");
        pthread_exit(NULL);
    }
    /* initialize MCC-USB library */
    udev = NULL;
    if (!(udev = usb_device_find_USB_MCC(USB201_PID, NULL))) {
        /* cannot find device; exit */
        pthread_exit(NULL);
    }

    /* start reading from DIO */
    scanDIO(counts, run);
    fflush(stdout);
    return 0;
}

/* reads from DIO, updates counts */
void scanDIO(int *counts, int *run) {
    int portVal = 0;
    int prevPortVal = 0;
    int i;

    /* configure tristate registers (set all DIO pins to inputs) */
    usbDTristateW_USB20X(udev, 8);

    /* loop until we get the proper signal */
    while(*run) {
        printf("usbdaq: scanning...");
        /* store previous reading */
        prevPortVal = portVal;
        /* read digital inputs */
        portVal = usbDLatchR_USB20X(udev);
        
        int temp = 0;
        for (i = 1; i<=8; i++) {
            /* add one to the count if the particular bit is 1 */
            /* and the previous value is 0 */
            /*counts[i-1]*/temp += (((portVal&(1<<(i-1))) >> (i-1)) && 
                    !((prevPortVal&(1<<(i-1))) >> (i-1)));
        }
        struct timespec time;
        time.tv_sec = 0;
        time.tv_nsec = 100000000L;
        nanosleep(&time, &time);
        fflush(stdout);
    }
    fflush(stdout);
    /* we are done */
    /* clean up device driver */
    cleanup_USB20X(udev);
}
