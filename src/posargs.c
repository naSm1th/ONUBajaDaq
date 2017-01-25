/*
    Author: Ryan Carl
    Project: ONU Baja DAQ

    posargs.c - process positional command line arguments
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_ERROR -1

/* handle args */
int initio(int argc, char *argv[], FILE **infile, FILE **outfile) {
    char *prog = argv[0];
    if (argc > 3) {
        fprintf(stderr, "%s: too many args\n", prog);
        *infile = stdin;
        *outfile = stdout;
    } else if (argc == 3) {
        if ((*infile = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "%s: cannot open %s\n", prog, argv[1]);
            /* no interrupt version
            *infile = stdin;
            */
            exit(FILE_ERROR);
        }
        if ((*outfile = fopen(argv[2], "w")) == NULL) {
            fprintf(stderr, "%s: cannot open %s\n", prog, argv[2]);
            /* no interrupt version
            *outfile = stdout;
            */
            exit(FILE_ERROR);
        }
    } else if (argc == 2) {
        if ((*outfile = fopen(argv[1], "w")) == NULL) {
            fprintf(stderr, "%s: cannot open %s\n", prog, argv[1]);
            /* no interrupt version
            *outfile = stdout;
            */
            exit(FILE_ERROR);
        }
        *infile = stdin;
    } else {
        *infile = stdin;
        *outfile = stdout;
    }
    return -1;
}
