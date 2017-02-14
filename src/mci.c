/*
    Author: Ryan Carl
    Project: ONU Baja DAQ

    mci.c - gather and send metrics from MC interface 
*/
#include <stdio.h>              /* printf */

void *initUSBDaq(void *array) {    
    int *counts = array;
    int i;
    for (i = 0; i < 8; i++) {
        counts[i] = i*2;
    }
}
