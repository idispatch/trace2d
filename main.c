#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

typedef int cell_t;

struct pattern_t {
    int w;
    int h;
    cell_t * data;
};

typedef struct pattern_t pattern;

struct coord_t {
    int column;
    int row;
};

typedef struct coord_t coord;

pattern * alloc_pattern(int w, int h) {
    size_t nbytes = sizeof(pattern) + w * h * sizeof(cell_t);
    pattern * p = (pattern*)calloc(nbytes, 1);
    if(!p) {
        return p;
    }
    p->w = w;
    p->h = h;
    p->data = (cell_t*)(p + 1);
    return p;
}

void free_pattern(pattern * p) {
    free(p);
}

pattern * pattern_copy(pattern * in) {
    size_t nbytes = sizeof(pattern) - sizeof(cell_t*) + in->w * in->h * sizeof(cell_t);
    pattern * p = (pattern*)malloc(nbytes);
    if(!p) {
        return p;
    }
    p->w = in->w;
    p->h = in->h;
    p->data = (cell_t*)(p + 1);
    memcpy(p->data, in->data, in->w * in->h * sizeof(cell_t));
    return p;
}

cell_t pattern_get(pattern * p, int x, int y) {
    if(!p) {
        return 0;
    }
    if(x < 0 || x >= p->w) {
        return 0;
    }
    if(y < 0 || y >= p->h) {
        return 0;
    }
    return p->data[x + y*p->w];
}

void pattern_set(pattern * p, int x, int y, cell_t v) {
    if(!p) {
        return;
    }
    if(x < 0 || x >= p->w) {
        return;
    }
    if(y < 0 || y >= p->h) {
        return;
    }
    p->data[x + y*p->w] = v;
}

void pattern_print(pattern * p) {
    if(!p) {
        return;
    }
    int row, column;
    for(row = 0; row < p->h; row++) {
        for(column = 0; column < p->w; column++) {
            cell_t v = pattern_get(p, column, row);
            fprintf(stdout, "%d ", (int)v);
        }
        fprintf(stdout, "\n");
    }
    fflush(stdout);
}

int read_pattern(const char * fname, pattern ** p) {
    int rc;
    int w, h;
    int row, column;
    *p = NULL;

    FILE * fp = fopen(fname, "rb");
    if(!fp) {
        return -1;
    }

    do {
        if(fscanf(fp, "%d %d", &w, &h) != 2) {
            rc = -1;
            break;
        }

        if(w <= 0 || w > 100 || h <= 0 || h > 100) {
            rc = -1;
            break;
        }

        *p = alloc_pattern(w, h);
        if(!*p) {
            rc = -1;
        }

        rc = 0;
        for(row = 0; row < h && rc == 0; ++row) {
            for(column = 0; column < w && rc == 0; ++column) {
                cell_t c;
                if(1 != fscanf(fp, "%d", (int*)&c)) {
                    rc = -1;
                } else {
                    pattern_set(*p, column, row, c);
                }
            }
        }
    } while(0);

    if(rc != 0) {
        free_pattern(*p);
        *p = NULL;
    }

    fclose(fp);
    return rc;
}

void fill_pattern_do(pattern * p, pattern * r, int column, int row, int group) {
    pattern_set(r, column, row, group);

    if(column + 1 < p->w &&
       pattern_get(p, column + 1, row) != 0 &&
       pattern_get(r, column + 1, row) == 0) {
        fill_pattern_do(p, r, column + 1, row, group);
    }
    if(column - 1 >= 0 &&
       pattern_get(p, column - 1, row) != 0 &&
       pattern_get(r, column - 1, row) == 0) {
        fill_pattern_do(p, r, column - 1, row, group);
    }
    if(row + 1 < p->h &&
       pattern_get(p, column, row + 1) != 0 &&
       pattern_get(r, column, row + 1) == 0) {
        fill_pattern_do(p, r, column, row + 1, group);
    }
    if(row - 1 >= 0 &&
       pattern_get(p, column, row - 1) != 0 &&
       pattern_get(r, column, row - 1) == 0) {
        fill_pattern_do(p, r, column, row - 1, group);
    }
}

pattern * fill_pattern(pattern * p) {
    int row, col, group = 0;
    pattern * r = alloc_pattern(p->w, p->h);
    if(r!=NULL) {
        for(row = 0; row < p->h; ++row) {
            for(col = 0; col < p->w; ++col) {
                cell_t v = pattern_get(p, col, row);
                if(!v)
                    continue;
                v = pattern_get(r, col, row);
                if(v)
                    continue;
                fill_pattern_do(p, r, col, row, ++group);
            }
        }
    }
    return r;
}

void read_and_print(const char * fname) {
    pattern * p;
    if(read_pattern(fname, &p) == 0) {
        pattern_print(p);
        free_pattern(p);
    }
}

void read_and_fill(const char * fname) {
    pattern *p, *r;
    if(read_pattern(fname, &p) == 0) {
        r = fill_pattern(p);
        pattern_print(r);
        free_pattern(r);
        free_pattern(p);
    }
}

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Invalid arguments\n");
        return EXIT_FAILURE;
    }
    int n;
    for(n = 1; n < argc; ++n) {
        //read_and_print(argv[1]);
        read_and_fill(argv[n]);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    return 0;
}

