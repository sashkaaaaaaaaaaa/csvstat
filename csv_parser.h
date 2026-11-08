#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#define MODE_SCHEMA 0
#define MODE_STATS  1

int process_csv(const char *filename, int mode);

#endif
