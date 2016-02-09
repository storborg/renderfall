#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#include <getopt.h>
#include <png.h>
#include <fftw3.h>

#include "formats.h"
#include "window.h"

double maxdb = 0;
double mindb = FLT_MAX;

int32_t maxval = 0;
int32_t minval = INT32_MAX;

void scale_log(png_byte *ptr, double val) {
    // Make a monochromatic purple output for now.
    double db = log10(val);
    if (db > maxdb) maxdb = db;
    if (db < mindb) mindb = db;

    // Kind of arbitrarily picked.
    int32_t v = (int32_t) ((db * 85.0f) + 50.0f);

    if (v > maxval) maxval = v;
    if (v < minval) minval = v;

    v = 255 - v;

    if (v > 255) v = 255;
    if (v < 0) v = 0;

    ptr[0] = (uint8_t) v;
    ptr[1] = (uint8_t) v;
    ptr[2] = (uint8_t) v;
}

void scale_linear(png_byte *ptr, double val) {
    int32_t v = (int32_t) (val * 100.0f);
    if (v > maxval) maxval = v;
    if (v < minval) minval = v;

    if (v > 255) v = 255;
    if (v < 0) v = 0;

    // Make a monochromatic purple output for now.
    ptr[0] = (uint8_t) v;
    ptr[1] = 0;
    ptr[2] = (uint8_t) v;
}

void render_complex(png_byte *ptr, fftw_complex val) {
    double mag = hypot(val[0], val[1]);
    scale_log(ptr, mag);
}

void waterfall(png_structp png_ptr, FILE* fp,
               uint32_t w, uint32_t h, read_samples_fn reader) {
    png_bytep row = (png_bytep) malloc(3 * w * sizeof(png_byte));

    fftw_complex *in, *out, *inter;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    inter = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    p = fftw_plan_dft_1d(w, inter, out, FFTW_FORWARD, FFTW_PATIENT);

    window_t win = make_window_hann(w);
    uint32_t half = w / 2;
    uint32_t x, y;

    for (y = 0; y < h; y++) {
        // read an FFT-worth of complex samples and convert them to doubles
        reader(fp, in, w);

        // apply a window we created earlier
        apply_window(win, in, inter);

        fftw_execute(p);

        // convert output from doubles to colors

        // first half (negative frequencies)
        for (x = 0; x < half; x++) {
            render_complex(&(row[x * 3]), out[x + half]);
        }

        // second half (positive frequencies)
        for (x = half; x < w; x++) {
            render_complex(&(row[x * 3]), out[x - half]);
        }

        png_write_row(png_ptr, row);
    }

    destroy_window(win);

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(inter);
    fftw_free(out);
}

void usage(char *arg) {
    fprintf(stderr,
            "Usage: %s [-n fftsize] [-f format] [-v] <filename>\n",
            arg);
}

int parse_format(format_t *result, char *arg) {
    if (strcmp(arg, "uint8")) {
        *result = FORMAT_UINT8;
    } else if (strcmp(arg, "int8")) {
        *result = FORMAT_INT8;
    } else if (strcmp(arg, "uint16")) {
        *result = FORMAT_UINT16;
    } else if (strcmp(arg, "int16")) {
        *result = FORMAT_INT16;
    } else if (strcmp(arg, "uint32")) {
        *result = FORMAT_UINT32;
    } else if (strcmp(arg, "int32")) {
        *result = FORMAT_INT32;
    } else if (strcmp(arg, "float32")) {
        *result = FORMAT_FLOAT32;
    } else if (strcmp(arg, "float64")) {
        *result = FORMAT_FLOAT64;
    } else {
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char filename[255];

    // Default args.
    format_t fmt = FORMAT_FLOAT32;
    uint32_t fftsize = 2048;
    bool verbose = false;

    int c;

    while ((c = getopt(argc, argv, "hvf:n:")) != -1) {
        switch (c) {
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                verbose = true;
                break;
            case 'n':
                fftsize = atoi(optarg);
                break;
            case 'f':
                if (parse_format(&fmt, optarg) < 0) {
                    fprintf(stderr, "Unknown format: %s", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                usage(argv[0]);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "Error parsing arguments.\n");
                return EXIT_FAILURE;
        }
    }

    if ((argc - optind) > 1) {
        fprintf(stderr, "Too many arguments.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    } else if ((argc - optind) < 1) {
        fprintf(stderr, "Must supply an input filename.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    strcpy(filename, argv[optind]);

    FILE *readfp = fopen(filename, "rb");

    fseek(readfp, 0, SEEK_END);
    int size = ftell(readfp);
    fseek(readfp, 0, SEEK_SET);

    size_t sample_size;
    read_samples_fn reader;
    switch(fmt) {
        case FORMAT_INT8:
            sample_size = sizeof(int8_t) * 2;
            reader = read_samples_int8;
            break;
        case FORMAT_UINT8:
            sample_size = sizeof(uint8_t) * 2;
            reader = read_samples_uint8;
            break;
        case FORMAT_INT16:
            sample_size = sizeof(int16_t) * 2;
            reader = read_samples_int16;
            break;
        case FORMAT_UINT16:
            sample_size = sizeof(uint16_t) * 2;
            reader = read_samples_uint16;
            break;
        case FORMAT_INT32:
            sample_size = sizeof(int32_t) * 2;
            reader = read_samples_int32;
            break;
        case FORMAT_UINT32:
            sample_size = sizeof(uint32_t) * 2;
            reader = read_samples_uint32;
            break;
        case FORMAT_FLOAT32:
            sample_size = sizeof(float) * 2;
            reader = read_samples_float32;
            break;
        case FORMAT_FLOAT64:
            sample_size = sizeof(double) * 2;
            reader = read_samples_float64;
            break;
    }

    int nsamples = size / sample_size;

    uint32_t width = fftsize;
    uint32_t height = nsamples / width;

    strcat(filename, ".png");

    printf("Writing %d x %d output to %s...\n", width, height, filename);

    FILE *writefp = fopen(filename, "wb");
    if (!writefp) {
        fprintf(stderr, "Error: failed to write to %s.\n", filename);
        return EXIT_FAILURE;
    }

    png_structp png_ptr = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error: could not initialize write struct.\n");
        return EXIT_FAILURE;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Error: could not initialize info struct.\n");
        png_destroy_write_struct(&png_ptr, NULL);
        return EXIT_FAILURE;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: libpng error.\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(writefp);
        return EXIT_FAILURE;
    }

    png_init_io(png_ptr, writefp);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    waterfall(png_ptr, readfp, width, height, reader);

    png_write_end(png_ptr, NULL);

    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);

    fclose(writefp);
    fclose(readfp);

    printf("Max dB: %f\n", maxdb);
    printf("Min dB: %f\n", mindb);

    printf("Max value: %d\n", maxval);
    printf("Min value: %d\n", minval);

    return EXIT_SUCCESS;
}
