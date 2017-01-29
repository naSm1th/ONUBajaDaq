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

#include <libusb/pmd.h>
#include <libusb/usb-20X.h>

#include "usbdaq.h"

/* the pointer to the shared int for DIO */
int *counts;
/* the handle to the udev device */
libusb_device_handle *udev = NULL;
/* tracks if we are currently handling a signal */
volatile sig_atomic_t sigInProgress = 0;

/* initializes and runs the process for the USB DAQ interface */
void initUSBDaq(int *inCounts) {
    /* save reference to our array of counts */
    counts = inCounts;
    /* take care of all pipes */

    /* initialize libusb (required for MCC-USB library */
    int ret = libusb_init(NULL);
    if (ret < 0) {
        perror("usb_device_find_USB_MCC: Failed to initialize libusb");
        exit(1);
    }
    /* initialize MCC-USB library */
    udev = NULL;
    if (!(udev = usb_device_find_USB_MCC(USB201_PID, NULL))) {
        /* cannot find device; exit */
        exit(1);
    }

    /* handle SIGTERM */
    signal(SIGTERM, exitHandler);

    /* start reading from DIO */
    scanDIO();
}

/* handles interrupts (SIGTERM), allowing program to exit cleanly */
/* much code borrowed from: */
/* ftp://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_chapter/libc_24.html */
void exitHandler(int sig) {
    /* don't act on more than one signal at a time */
    if (sigInProgress) {
        raise(sig);
    }
    sigInProgress = 1;

    /* clean up device driver */
    cleanup_USB20X(udev);

    /* reraise signal and die */
    signal(sig, SIG_DFL);
    raise(sig);
}

/* reads from DIO, updates counts */
void scanDIO() {
    int portVal = 0;
    int i;

    /* configure tristate registers (set all DIO pins to inputs) */
    usbDTristateW_USB20X(udev, 8);

    /* loop until we get the proper signal */
    while(1) {
        /* read digital inputs */
        portVal = usbDLatchR_USB20X(udev);

        for (i = 1; i<7; i++) {
            counts[i-1] += (portVal&i) >> (i-1);
        }
    }
}
