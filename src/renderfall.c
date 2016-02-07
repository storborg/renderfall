#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include <png.h>
#include <fftw3.h>

float maxdb = 0;
float mindb = FLT_MAX;

int32_t maxval = 0;
int32_t minval = UINT32_MAX;

typedef enum {
    FORMAT_INT8 = 0,
    FORMAT_UINT8 = 1,
    FORMAT_INT16 = 2,
    FORMAT_UINT16 = 3,
    FORMAT_INT32 = 4,
    FORMAT_UINT32 = 5,
    FORMAT_FLOAT32 = 6,
    FORMAT_FLOAT64 = 7,
} format_t;

void scale_log(png_byte *ptr, float val) {
    // Make a monochromatic purple output for now.
    float db = log10f(val);
    if (db > maxdb) maxdb = db;
    if (db < mindb) mindb = db;

    int32_t v = (int32_t) ((db * 80.0f) + 40.0f);
    if (v > maxval) maxval = v;
    if (v < minval) minval = v;

    v = 255 - v;

    if (v > 255) v = 255;
    if (v < 0) v = 0;

    ptr[0] = (uint8_t) v;
    ptr[1] = (uint8_t) v;
    ptr[2] = (uint8_t) v;
}

void scale_linear(png_byte *ptr, float val) {
    // Make a monochromatic purple output for now.
    ptr[0] = (uint8_t) (val * 4);
    ptr[1] = 0;
    ptr[2] = (uint8_t) (val * 4);
}

void read_samples_int8(FILE *fp, fftwf_complex *buf, uint32_t n) {
    int8_t *raw = (int8_t*) malloc(n * 2 * sizeof(int8_t));
    fread(raw, 2, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((float) raw[i] / INT8_MAX);
        buf[i][1] = ((float) raw[i + 1] / INT8_MAX);
    }
    free(raw);
}

void read_samples_int16(FILE *fp, fftwf_complex *buf, uint32_t n) {
    int16_t *raw = (int16_t*) malloc(n * 2 * sizeof(int16_t));
    fread(raw, 4, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((float) raw[i * 2] / INT16_MAX);
        buf[i][1] = ((float) raw[(i * 2) + 1] / INT16_MAX);
    }
    free(raw);
}

void read_samples_uint16(FILE *fp, fftwf_complex *buf, uint32_t n) {
    uint16_t *raw = (uint16_t*) malloc(n * 2 * sizeof(uint16_t));
    fread(raw, 4, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((float) raw[i * 2] / UINT16_MAX);
        buf[i][1] = ((float) raw[(i * 2) + 1] / UINT16_MAX);
    }
}

void read_samples_float32_bad(FILE *fp, fftwf_complex *buf, uint32_t n) {
    size_t sample_size = 2 * sizeof(float);
    float *raw = (float*) malloc(n * sample_size);
    fread(raw, sample_size, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = raw[i * 2];
        buf[i][1] = raw[(i * 2) + 1];
    }
    free(raw);
}

void read_samples_float32(FILE *fp, fftwf_complex *buf, uint32_t n) {
    size_t sample_size = 2 * sizeof(float);
    fread(buf, sample_size, n, fp);
}

void render_complex(png_byte *ptr, fftwf_complex val) {
    float mag = hypot(val[0], val[1]);
    scale_log(ptr, mag);
}

void waterfall(png_structp png_ptr, FILE* fp, int w, int h, format_t fmt) {
    png_bytep row = (png_bytep) malloc(3 * w * sizeof(png_byte));

    fftwf_complex *in, *out;
    fftwf_plan p;

    float val;

    in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * w);
    out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * w);
    p = fftwf_plan_dft_1d(w, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (uint32_t y = 0; y < h; y++) {
        // read an FFT-worth of complex samples and convert them to floats
        if (fmt == FORMAT_INT8) {
            read_samples_int8(fp, in, w);
        } else if (fmt == FORMAT_INT16) {
            read_samples_int16(fp, in, w);
        } else if (fmt == FORMAT_UINT16) {
            read_samples_uint16(fp, in, w);
        } else if (fmt == FORMAT_FLOAT32) {
            read_samples_float32(fp, in, w);
        }

        fftwf_execute(p);

        // convert output from floats to colors

        uint32_t half = w / 2;
        uint32_t x;

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

    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);
}

int main(int argc, char* argv[]) {
    char filename[255];
    int bit_depth = 8;
    int color_type = PNG_COLOR_TYPE_RGB;
    format_t fmt = FORMAT_FLOAT32;

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strcpy(filename, argv[1]);

    FILE *readfp = fopen(filename, "rb");

    fseek(readfp, 0, SEEK_END);
    int size = ftell(readfp);
    fseek(readfp, 0, SEEK_SET);

    size_t sample_size;
    switch(fmt) {
        case FORMAT_INT8:
            sample_size = sizeof(int8_t) * 2;
            break;
        case FORMAT_UINT8:
            sample_size = sizeof(uint8_t) * 2;
            break;
        case FORMAT_INT16:
            sample_size = sizeof(int16_t) * 2;
            break;
        case FORMAT_UINT16:
            sample_size = sizeof(uint16_t) * 2;
            break;
        case FORMAT_INT32:
            sample_size = sizeof(int32_t) * 2;
            break;
        case FORMAT_UINT32:
            sample_size = sizeof(uint32_t) * 2;
            break;
        case FORMAT_FLOAT32:
            sample_size = sizeof(float) * 2;
            break;
        case FORMAT_FLOAT64:
            sample_size = sizeof(double) * 2;
            break;
    }

    int nsamples = size / sample_size;

    uint32_t width = 2048;
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
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    waterfall(png_ptr, readfp, width, height, FORMAT_FLOAT32);

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
