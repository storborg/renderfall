#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <png.h>
#include <fftw3.h>


void render_fft_value(png_byte *ptr, double val) {
    // Make a monochromatic purple output for now.
    ptr[0] = (uint8_t) (val * 4);
    ptr[1] = 0;
    ptr[2] = (uint8_t) (val * 4);
}

void waterfall(png_structp png_ptr, FILE* fp, int w, int h) {
    png_bytep row = (png_bytep) malloc(3 * w * sizeof(png_byte));

    fftw_complex *in, *out;
    fftw_plan p;

    double val;
    uint32_t offset;

    // Allocate a buffer for a row of raw samples: 4 bytes per IQ pair.
    uint16_t *raw = (uint16_t*) malloc(w * 2 * 2);

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * w);
    p = fftw_plan_dft_1d(w, in, out, FFTW_FORWARD, FFTW_MEASURE);

    for (uint32_t y = 0; y < h; y++) {

        // read an FFT-worth of complex samples
        fread(raw, 4, w, fp);

        // convert input from uint16_t pair to double pair
        for (uint32_t i = 0; i < w; i++) {
            in[i][0] = ((float) raw[i] / UINT16_MAX);
            in[i][1] = ((float) raw[i + 1] / UINT16_MAX);
        }

        fftw_execute(p);

        // convert output from doubles to colors
        for (uint32_t x = 0; x < w; x++) {
            val = hypot(out[x][0], out[x][1]);
            render_fft_value(&(row[x * 3]), val);
        }

        png_write_row(png_ptr, row);
    }


    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
}

int main(int argc, char* argv[]) {
    char filename[255];
    int bit_depth = 8;
    int color_type = PNG_COLOR_TYPE_RGB;

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strcpy(filename, argv[1]);

    FILE *readfp = fopen(filename, "rb");

    fseek(readfp, 0, SEEK_END);
    int size = ftell(readfp);
    fseek(readfp, 0, SEEK_SET);

    // Assuming 16-bit complex integer samples.
    int nsamples = size / 4;

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


    waterfall(png_ptr, readfp, width, height);

    png_write_end(png_ptr, NULL);

    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);

    fclose(writefp);
    fclose(readfp);

    return EXIT_SUCCESS;
}
