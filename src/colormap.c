#include <stdint.h>
#include <math.h>
#include <float.h>

#include <png.h>
#include <fftw3.h>

double maxdb = 0;
double mindb = FLT_MAX;

int32_t maxval = 0;
int32_t minval = INT32_MAX;

void scale_log(png_byte *ptr, double val) {
    // Make a monochromatic black-on-white output for now.
    double db = log10(val);
    if (db > maxdb) maxdb = db;
    if (db < mindb) mindb = db;

    // Kind of arbitrarily picked.
    int32_t v = (int32_t) ((db * 85.0f) + 200.0f);

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

void print_scale_stats() {
    printf("Max dB: %f\n", maxdb);
    printf("Min dB: %f\n", mindb);

    printf("Max value: %d\n", maxval);
    printf("Min value: %d\n", minval);
}
