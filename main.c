#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#define CHECK_BOUNDS 1

typedef int cell_t;

struct pattern_t {
    int w;
    int h;
    cell_t * data;
};

typedef struct pattern_t pattern;

struct coord_t {
    int col;
    int row;
};

typedef struct coord_t coord;

struct rect_t {
    coord pos;
    int w;
    int h;
};

typedef struct rect_t rect;

struct rectset_t {
    rect r;
    struct rectset_t * next;
};

typedef struct rectset_t * rectset;

static rectset rectset_add(rectset r, int col, int row, int w, int h) {
    rectset p = (rectset)calloc(sizeof(struct rectset_t), 1);
    p->next = r;
    p->r.pos.col = col;
    p->r.pos.row = row;
    p->r.w = w;
    p->r.h = h;
    return p;
}

static void rectset_free(rectset r) {
    while(r) {
        rectset t = r->next;
        free(r);
        r = t;
    }
}

static void rectset_print(rectset r) {
    fprintf(stdout, "(");
    while(r) {
        fprintf(stdout, "[x=%d,y=%d,w=%d,h=%d]", r->r.pos.col, r->r.pos.row, r->r.w, r->r.h);
        if(r->next) {
            fprintf(stdout, ",\n ");
        }
        r = r->next;
    }
    fprintf(stdout, ")\n");
    fflush(stdout);
}

static pattern * pattern_alloc(int w, int h) {
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

static void pattern_free(pattern * p) {
    free(p);
}

static cell_t pattern_get(pattern * p, int x, int y) {
#if CHECK_BOUNDS
    if(!p) {
        return 0;
    }
    if(x < 0 || x >= p->w || y < 0 || y >= p->h) {
        fprintf(stderr, "Out of bounds: %d, %d (%d, %d)\n", x, y, p->w, p->h);
        abort();
    }
#endif
    return p->data[x + y * p->w];
}

static void pattern_set(pattern * p, int x, int y, cell_t v) {
#if CHECK_BOUNDS
    if(!p) {
        return;
    }
    if(x < 0 || x >= p->w || y < 0 || y >= p->h) {
        fprintf(stderr, "Out of bounds: %d, %d (%d, %d)\n", x, y, p->w, p->h);
        abort();
    }
#endif
    p->data[x + y * p->w] = v;
}

static unsigned int pattern_hash(pattern * p) {
    int row, col;
    int min_row = p->h, min_col = p->w;
    int max_row = -1, max_col = -1;
    for(row = 0; row < p->h; ++row) {
        for(col = 0; col < p->w; ++col) {
            if(pattern_get(p, col, row) == 0)
                continue;
            if(row < min_row)
                min_row = row;
            if(row > max_row)
                max_row = row;
            if(col < min_col)
                min_col = col;
            if(col > max_col)
                max_col = col;
        }
    }
#if 0
    fprintf(stdout, "min_col=%d,min_row=%d,max_col=%d,max_row=%d\n", min_col, min_row, max_col, max_row);
#endif
    max_col = max_col - min_col + 1;
    max_row = max_row - min_row + 1;
    const int size = max_col > max_row ? max_col : max_row;
    unsigned result = 0;
    for(row = min_row; row < min_row + size; ++row) {
        for(col = min_col; col < min_col + size; ++col) {
            if(col < p->w && row < p->h && pattern_get(p, col, row) != 0) {
                result |= 1;
            }
            result <<= 1;
        }
    }
    result >>= 1;
    return result;
}

static void pattern_print(pattern * p) {
#if CHECK_BOUNDS
    if(!p) {
        return;
    }
#endif
    int row, col;
    for(row = 0; row < p->h; row++) {
        for(col = 0; col < p->w; col++) {
            cell_t v = pattern_get(p, col, row);
            fprintf(stdout, "%d ", (int)v);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "hash=%x\n", pattern_hash(p));
    fflush(stdout);
}

static int pattern_read(const char * fname, pattern ** p) {
    int rc;
    int w, h;
    int row, col;
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

        *p = pattern_alloc(w, h);
        if(!*p) {
            rc = -1;
        }

        rc = 0;
        for(row = 0; row < h && rc == 0; ++row) {
            for(col = 0; col < w && rc == 0; ++col) {
                cell_t c;
                if(1 != fscanf(fp, "%d", (int*)&c)) {
                    rc = -1;
                } else {
                    pattern_set(*p, col, row, c);
                }
            }
        }
    } while(0);

    if(rc != 0) {
        pattern_free(*p);
        *p = NULL;
    }

    fclose(fp);
    return rc;
}

static void pattern_fill_do(pattern * p, pattern * r, int col, int row, int group) {
    pattern_set(r, col, row, group);

    if(col + 1 < p->w &&
       pattern_get(p, col + 1, row) != 0 &&
       pattern_get(r, col + 1, row) == 0) {
        pattern_fill_do(p, r, col + 1, row, group);
    }
    if(col - 1 >= 0 &&
       pattern_get(p, col - 1, row) != 0 &&
       pattern_get(r, col - 1, row) == 0) {
        pattern_fill_do(p, r, col - 1, row, group);
    }
    if(row + 1 < p->h &&
       pattern_get(p, col, row + 1) != 0 &&
       pattern_get(r, col, row + 1) == 0) {
        pattern_fill_do(p, r, col, row + 1, group);
    }
    if(row - 1 >= 0 &&
       pattern_get(p, col, row - 1) != 0 &&
       pattern_get(r, col, row - 1) == 0) {
        pattern_fill_do(p, r, col, row - 1, group);
    }
}

static pattern * pattern_fill(pattern * p) {
    int row, col, group = 0;
    pattern * r = pattern_alloc(p->w, p->h);
    if(r!=NULL) {
        for(row = 0; row < p->h; ++row) {
            for(col = 0; col < p->w; ++col) {
                cell_t v = pattern_get(p, col, row);
                if(!v)
                    continue;
                v = pattern_get(r, col, row);
                if(v)
                    continue;
                pattern_fill_do(p, r, col, row, ++group);
            }
        }
    }
    return r;
}

static rectset rectset_calc(pattern * p) {
    int row, col, group = 0;
    rectset result = NULL;

    pattern * r = pattern_alloc(p->w, p->h);
    if(!r)
        return result;

    for(row = 0; row < p->h; ++row) {
        for(col = 0; col < p->w; ++col) {
            cell_t v = pattern_get(p, col, row);
            if(!v)
                continue;
            v = pattern_get(r, col, row);
            if(v)
                continue;
            pattern_fill_do(p, r, col, row, ++group);
        }
    }

    /* pattern r contains 4-connected components and group is the count of them */
    for(row = 0; row < p->h; ++row) {
        for(col = 0; col < p->w; ++col) {
            cell_t group = pattern_get(r, col, row);
            if(!group)
                continue;
            int i = col + 1, j;
            int w, h = p->h;
            while(i < p->w && pattern_get(r, i, row) == group) {
                i++;
            }
            i--;
            w = i - col + 1;

            for(i = col; i < col + w; ++i) {
                for(j = row + 1; j < p->h && pattern_get(r, i, j) == group; j++);
                j--;
                if(h > j) {
                    h = j;
                }
            }
            h = h - row + 1;

            /* rectangle is at (col,row) and size is w x h */
            for(j = row; j < row + h; ++j)
                for(i = col; i < col + w; ++i)
                    pattern_set(r, i, j, 0);

            result = rectset_add(result, col, row, w, h);
        }
    }

    return result;
}

static void read_and_print(const char * fname) {
    pattern * p;
    if(pattern_read(fname, &p) == 0) {
        pattern_print(p);
        pattern_free(p);
    }
}

static void read_and_fill(const char * fname) {
    pattern *p, *r;
    if(pattern_read(fname, &p) == 0) {
        r = pattern_fill(p);
        pattern_print(r);
        pattern_free(r);
        pattern_free(p);
    }
}

static void read_and_rectset_calc(const char * fname) {
    pattern *p;
    if(pattern_read(fname, &p) == 0) {
        rectset r = rectset_calc(p);
        pattern_print(p);
        rectset_print(r);
        rectset_free(r);
        pattern_free(p);
    }
}

static void pattern_generate_do(pattern * p, int col, int row, int step, int * counter) {
    if(step <= 0) {
        (*counter)++;
        fprintf(stdout, "%d)\n", *counter);
        pattern_print(p);
        fprintf(stdout, "\n");
    } else {
        step--;
        if(pattern_get(p, col - 1, row) == 0) {
            pattern_set(p, col - 1, row, 1);
            pattern_generate_do(p, col - 1, row, step, counter);
            pattern_set(p, col - 1, row, 0);
        }
        if(pattern_get(p, col + 1, row) == 0) {
            pattern_set(p, col + 1, row, 1);
            pattern_generate_do(p, col + 1, row, step, counter);
            pattern_set(p, col + 1, row, 0);
        }
        if(pattern_get(p, col, row - 1) == 0) {
            pattern_set(p, col, row - 1, 1);
            pattern_generate_do(p, col, row - 1, step, counter);
            pattern_set(p, col, row - 1, 0);
        }
        if(pattern_get(p, col, row + 1) == 0) {
            pattern_set(p, col, row + 1, 1);
            pattern_generate_do(p, col, row + 1, step, counter);
            pattern_set(p, col, row + 1, 0);
        }
    }
}

static void pattern_trace_do(pattern * p, int step, int * counter) {
    if(step <= 0) {
        (*counter)++;
        fprintf(stdout, "%d)\n", *counter);
        pattern_print(p);
        fprintf(stdout, "\n");
    } else {
        int col, row;
        for(row = 0; row < p->h; ++row) {
            for(col = 0; col < p->w; ++col) {
                if(pattern_get(p, col, row) != 0)
                    continue;
                if(col > 0 && pattern_get(p, col - 1, row) != 0) {
                    pattern_set(p, col, row, step);
                    pattern_trace_do(p, step - 1, counter);
                    pattern_set(p, col, row, 0);
                    continue;
                }
                if(col < p->w - 1 && pattern_get(p, col + 1, row) != 0) {
                    pattern_set(p, col, row, step);
                    pattern_trace_do(p, step - 1, counter);
                    pattern_set(p, col, row, 0);
                    continue;
                }
                if(row > 0 && pattern_get(p, col, row - 1) != 0) {
                    pattern_set(p, col, row, step);
                    pattern_trace_do(p, step - 1, counter);
                    pattern_set(p, col, row, 0);
                    continue;
                }
                if(row < p->h - 1 && pattern_get(p, col, row + 1) != 0) {
                    pattern_set(p, col, row, step);
                    pattern_trace_do(p, step - 1, counter);
                    pattern_set(p, col, row, 0);
                    continue;
                }
            }
        }
    }
}

static void pattern_generate(int step) {
    const int size = step * 2 - 1;
    pattern * p = pattern_alloc(size, size);
    if(p) {
        int col = step - 1;
        int row = step - 1;
        int counter = 0;
        pattern_set(p, col, row, 1);
        pattern_generate_do(p, col, row, step - 1, &counter);
        pattern_free(p);
    }
}

static void pattern_trace(int step) {
    const int size = step * 2 - 1;
    pattern * p = pattern_alloc(size, size);
    if(p) {
        int col = step - 1;
        int row = step - 1;
        int counter = 0;
        pattern_set(p, col, row, step);
        pattern_trace_do(p, step - 1, &counter);
        pattern_free(p);
    }
}

int main(int argc, char **argv) {
    int ch;
    int mode = 'p';
    int steps;
    while((ch = getopt(argc, argv, "pfrg:t:")) != -1) {
        switch(ch) {
        case 'g':
        case 't':
            steps = atoi(optarg);
            mode = ch;
            break;
        case 'p':
            mode = ch;
            break;
        case 'f':
            mode = ch;
            break;
        case 'r':
            mode = ch;
            break;
        case '?':
        default:
            fprintf(stderr, "Invalid argument\n");
            exit(EXIT_FAILURE);
            break;
        }
    }

    argc -= optind;
    argv += optind;

    for(;mode == 'g' || mode == 't' || argc > 0; --argc, ++argv) {
        switch(mode) {
        case 'g':
            if(steps > 0) {
                pattern_generate(steps);
            }
            return EXIT_SUCCESS;
        case 't':
            if(steps > 0) {
                pattern_trace(steps);
            }
            return EXIT_SUCCESS;
        case 'p':
            read_and_print(*argv);
            fprintf(stdout, "\n");
            break;
        case 'f':
            read_and_fill(*argv);
            fprintf(stdout, "\n");
            break;
        case 'r':
            read_and_rectset_calc(*argv);
            fprintf(stdout, "\n");
            break;
        }
    }

    return 0;
}

