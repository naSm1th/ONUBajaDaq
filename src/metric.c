/*
    Author: Ryan Carl
    Project: ONU Baja DAQ

    metric.c - aggregate and store metrics to csv file

    format: metric_name metric_value timestamp
*/

main(int argc, char *argv[]) {
    FILE *infile = NULL;
    FILE *outfile = NULL;

    initio(argc, argv, &infile, &outfile);
}
