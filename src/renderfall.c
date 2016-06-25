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
#include "colormap.h"

void waterfall(png_structp png_ptr, FILE* fp, window_t win,
               uint32_t w, uint32_t h, read_samples_fn reader) {
    png_bytep row = (png_bytep) malloc(3 * w * sizeof(png_byte));

    fftw_complex *in, *out, *inter;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    inter = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    p = fftw_plan_dft_1d(w, inter, out, FFTW_FORWARD, FFTW_PATIENT);

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

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(inter);
    fftw_free(out);
}

void usage(char *arg) {
    fprintf(stderr, "Usage: %s [OPTIONS] <in>\n", arg);
    fprintf(stderr, "Render a waterfall spectrum from raw IQ samples.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n <fftsize>\tFFT size (power of 2)\n");
    fprintf(stderr, "  -f <format>\tInput format: uint8, int16, float64, etc.\n");
    fprintf(stderr, "  -w <window>\tWindowing function: hann, gaussian, square\n");
    fprintf(stderr, "  -o <outfile>\tOutput file path (defaults to <infile>.png)\n");
    fprintf(stderr, "  -s <offset>\tStart at specified byte offset\n");
    fprintf(stderr, "  -v \t\tPrint verbose debugging output\n");
}

int parse_format(format_t *result, char *arg) {
    if (!strcmp(arg, "uint8")) {
        *result = FORMAT_UINT8;
    } else if (!strcmp(arg, "int8")) {
        *result = FORMAT_INT8;
    } else if (!strcmp(arg, "uint16")) {
        *result = FORMAT_UINT16;
    } else if (!strcmp(arg, "int16")) {
        *result = FORMAT_INT16;
    } else if (!strcmp(arg, "uint32")) {
        *result = FORMAT_UINT32;
    } else if (!strcmp(arg, "int32")) {
        *result = FORMAT_INT32;
    } else if (!strcmp(arg, "float32")) {
        *result = FORMAT_FLOAT32;
    } else if (!strcmp(arg, "float64")) {
        *result = FORMAT_FLOAT64;
    } else {
        return -1;
    }
    return 0;
}

int prepare_window(window_t *win, char *arg, uint32_t w, bool verbose) {
    if (!strcmp(arg, "hann")) {
        if (verbose) printf("Using a Hann window of size %d.\n", w);
        *win = make_window_hann(w);
    } else if (!strcmp(arg, "gaussian")) {
        double beta = 8.00;
        if (verbose) printf("Using a Gaussian window of size %d, beta %0.3f\n", w, beta);
        *win = make_window_gaussian(w, beta);
    } else if (!strcmp(arg, "square")) {
        if (verbose) printf("Using a square window of size %d.\n", w);
        *win = make_window_square(w);
    } else {
        return -1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    char infile[255];
    char outfile[255] = "";
    char fmt_s[255];
    char window_s[255] = "hann";

    // Default args.
    format_t fmt = FORMAT_FLOAT32;
    uint32_t fftsize = 2048;
    bool verbose = false;
    uint64_t skip = 0;

    int c;

    while ((c = getopt(argc, argv, "hvf:n:o:s:")) != -1) {
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
                strcpy(fmt_s, optarg);
                if (parse_format(&fmt, fmt_s) < 0) {
                    fprintf(stderr, "Unknown format: %s", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'w':
                strcpy(window_s, optarg);
                break;
            case 'o':
                strcpy(outfile, optarg);
                break;
            case 's':
                {
                    char *end = NULL;
                    int64_t tempskip;
                    if (optarg == NULL
                            || ((tempskip = strtol(optarg, &end, 0 )),
                                (end && *end ))
                            || tempskip <= 0) {
                        //error
                        fprintf(stderr, "Invalid value for byte offset\n");
                        return EXIT_FAILURE;
                    } else {
                        skip = (uint64_t)tempskip;
                    }
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
        fprintf(stderr, "Excess arguments.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    } else if ((argc - optind) < 1) {
        fprintf(stderr, "Must supply an input filename.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    window_t win;
    if (prepare_window(&win, window_s, fftsize, verbose) < 0) {
        fprintf(stderr, "Unknown window function: %s", window_s);
        return EXIT_FAILURE;
    }

    if (verbose) printf("Opening input file...\n");

    strcpy(infile, argv[optind]);

    FILE *readfp = fopen(infile, "rb");
    if (readfp == NULL) {
        fprintf(stderr, "Failed to open input file: %s\n", infile);
        return EXIT_FAILURE;
    }

    fseek(readfp, 0, SEEK_END);
    int size = ftell(readfp);
    fseek(readfp, skip, SEEK_SET);

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

    if (!strcmp(outfile, "")) {
        strcpy(outfile, infile);
        strcat(outfile, ".png");
    }

    if (verbose) {
        printf("Reading %s samples from %s...\n", fmt_s, infile);
        printf("Writing %d x %d output to %s...\n", width, height, outfile);
    }

    FILE *writefp = fopen(outfile, "wb");
    if (!writefp) {
        fprintf(stderr, "Error: failed to write to %s.\n", outfile);
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

    if (verbose) printf("Writing PNG header..\n");
    png_init_io(png_ptr, writefp);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    if (verbose) printf("Rendering (this may take a while)...\n");
    waterfall(png_ptr, readfp, win, width, height, reader);

    if (verbose) printf("Writing PNG footer...\n");
    png_write_end(png_ptr, NULL);

    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);

    if (verbose) printf("Cleaning up...\n");
    fclose(writefp);
    fclose(readfp);

    destroy_window(win);

    print_scale_stats();

    return EXIT_SUCCESS;
}
