#include "csv_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define READ_ERROR (-2)
#define BASE 1000000000LL

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

typedef struct {
    char *name;
    long long min, max;
    long long even, positive, negative;
    long long high, low;
} Column;

static int read_field(FILE *file, Buffer *buf) {
    int ch;

    buf->length = 0;
    while (1) {
        ch = fgetc(file);
        if (ch == '\r') continue;
        if (ch == ',' || ch == '\n' || ch == EOF) break;
        if (buf->length + 1 >= buf->capacity) {
            char *bigger = realloc(buf->data, buf->capacity * 2);
            if (bigger == NULL) return READ_ERROR;
            buf->data = bigger;
            buf->capacity = buf->capacity * 2;
        }
        buf->data[buf->length] = (char)ch;
        buf->length++;
    }
    buf->data[buf->length] = '\0';
    return ch;
}

static int read_header(FILE *file, Buffer *buf, Column **cols, int *ncols) {
    Column *bigger;
    Column *col;
    int delimiter;

    while (1) {
        delimiter = read_field(file, buf);
        if (delimiter == READ_ERROR || buf->length == 0) return 0;

        bigger = realloc(*cols, (*ncols + 1) * sizeof(Column));
        if (bigger == NULL) return 0;
        *cols = bigger;
        col = &bigger[*ncols];
        (*ncols)++;

        memset(col, 0, sizeof(Column));
        col->min = LLONG_MAX;
        col->max = LLONG_MIN;
        col->name = malloc(buf->length + 1);
        if (col->name == NULL) return 0;
        memcpy(col->name, buf->data, buf->length + 1);

        if (delimiter != ',') return 1;
    }
}

static int add_value(Column *col, const char *text) {
    char *end;
    long long value = strtoll(text, &end, 10);

    if (end == text || *end != '\0') return 0;

    if (value < col->min) col->min = value;
    if (value > col->max) col->max = value;
    if (value % 2 == 0) col->even++;
    if (value > 0) col->positive++;
    if (value < 0) col->negative++;

    col->high = col->high + value / BASE;
    col->low = col->low + value % BASE;
    if (col->low >= BASE) { col->low -= BASE; col->high++; }
    if (col->low <= -BASE) { col->low += BASE; col->high--; }
    return 1;
}

static int read_rows(FILE *file, Column *cols, int ncols, Buffer *buf, long long *rows) {
    int delimiter;
    int col = 0;

    while (1) {
        delimiter = read_field(file, buf);
        if (delimiter == READ_ERROR) return 0;
        if (delimiter == EOF && col == 0 && buf->length == 0) return 1;
        if (col >= ncols) return 0;
        if (add_value(&cols[col], buf->data) == 0) return 0;
        col++;

        if (delimiter == ',') continue;
        if (col != ncols) return 0;
        (*rows)++;
        col = 0;
        if (delimiter == EOF) return 1;
    }
}

static void print_mean(const Column *col, long long rows) {
    long long high = col->high;
    long long low = col->low;
    long long rest;
    long long fraction;
    unsigned long long whole;
    int negative = 0;

    if (high > 0 && low < 0) { high--; low += BASE; }
    if (high < 0 && low > 0) { high++; low -= BASE; }
    if (high < 0 || low < 0) { negative = 1; high = -high; low = -low; }

    rest = (high % rows) * BASE + low;
    whole = (unsigned long long)(high / rows) * BASE + (unsigned long long)(rest / rows);
    rest = (rest % rows) * 1000000;
    fraction = rest / rows;
    if ((rest % rows) * 2 >= rows) fraction++;
    if (fraction == 1000000) { fraction = 0; whole++; }

    if (negative && (whole != 0 || fraction != 0)) printf("-");
    printf("%llu.%06lld", whole, fraction);
}

static void print_result(const Column *cols, int ncols, long long rows, int mode) {
    int i;

    printf("rows %lld\ncolumns %d\n", rows, ncols);
    for (i = 0; i < ncols; i++) {
        if (mode == MODE_SCHEMA) {
            printf("%d \"%s\" INT\n", i, cols[i].name);
            continue;
        }
        if (i > 0) printf("\n");
        printf("column %d\nname \"%s\"\ntype INT\nnulls 0\n", i, cols[i].name);
        printf("min %lld\nmax %lld\nmean ", cols[i].min, cols[i].max);
        print_mean(&cols[i], rows);
        printf("\npositive %lld\nnegative %lld\neven %lld\n",
               cols[i].positive, cols[i].negative, cols[i].even);
    }
}

int process_csv(const char *filename, int mode) {
    FILE *file = fopen(filename, "r");
    Buffer buf = {NULL, 0, 64};
    Column *cols = NULL;
    int ncols = 0;
    int ok = 0;
    int i;
    long long rows = 0;

    if (file == NULL) {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return 1;
    }

    buf.data = malloc(buf.capacity);
    if (buf.data != NULL && read_header(file, &buf, &cols, &ncols)
        && read_rows(file, cols, ncols, &buf, &rows) && rows > 0) {
        ok = 1;
    }

    if (ok) {
        print_result(cols, ncols, rows, mode);
    } else {
        fprintf(stderr, "Error: invalid CSV file\n");
    }

    for (i = 0; i < ncols; i++) free(cols[i].name);
    free(cols);
    free(buf.data);
    fclose(file);

    if (ok) return 0;
    return 1;
}
