# Makefile for Baja DAQ logger program by Ryan Carl, Nathanael Smith

make: logger.o serial.o usbdaq.o 
	gcc -Wall -pthread -lmccusb -lhidapi-libusb -lusb-1.0 -lm -lgps -lwiringPi -o logger logger.o serial.o usbdaq.o

debug:
	gcc -g -c -Wall logger.c
	gcc -g -c -Wall serial.c
	gcc -g -c -Wall usbdaq.c
	gcc -Wall -pthread -lmccusb -lhidapi-libusb -lusb-1.0 -lm -lgps -lwiringPI -o daq logger.o serial.o usbdaq.o


logger.o: logger.c
	gcc -Wall -lm -lgps -c logger.c

serial.o: serial.c serial.h
	gcc -Wall -c serial.c

usbdaq.o: usbdaq.c usbdaq.h
	gcc -Wall -c usbdaq.c

clean:
	rm *.o logger 

wrapup:
	tar -cvf daq.tar *.c *.h 
