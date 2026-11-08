#include <stdio.h>
#include <string.h>
#include "csv_parser.h"

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Usage:\n  csvstat --help\n  csvstat schema <csv-file>\n  csvstat stats <csv-file>\n");
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "schema") == 0) return process_csv(argv[2], MODE_SCHEMA);
    if (argc == 3 && strcmp(argv[1], "stats") == 0) return process_csv(argv[2], MODE_STATS);

    fprintf(stderr, "Usage: csvstat --help | schema <csv-file> | stats <csv-file>\n");
    return 2;
}
