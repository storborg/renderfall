#include <float.h>
#include <math.h>
#include <stdint.h>

#include <fftw3.h>
#include <png.h>

double maxdb = 0;
double mindb = FLT_MAX;

int32_t maxval = 0;
int32_t minval = INT32_MAX;

void hsv_to_rgb(double *r, double *g, double *b, double h, double s, double v) {
    // Note that right now, hue goes from 0 to 1.0 and not 0 to 360..
    // All input and output values are 0 to 1.0.

    // fprintf(stderr, "hsv_to_rgb - hue %f / sat %f / val %f\n\n", h, s, v);
    double c = 0.0, m = 0.0, x = 0.0;

    h *= 360.0;

    c = v * s;
    x = c * (1.0 - fabs(fmod(h / 60.0, 2) - 1.0));
    m = v - c;

    // XXX Refactor this series of if statements into a case statement with a
    // 'sector' int?
    if (h >= 0.0 && h < 60.0) {
        *r = c + m;
        *g = x + m;
        *b = m;
    } else if (h >= 60.0 && h < 120.0) {
        *r = x + m;
        *g = c + m;
        *b = m;
    } else if (h >= 120.0 && h < 180.0) {
        *r = m;
        *g = c + m;
        *b = x + m;
    } else if (h >= 180.0 && h < 240.0) {
        *r = m;
        *g = x + m;
        *b = c + m;
    } else if (h >= 240.0 && h < 300.0) {
        *r = x + m;
        *g = m;
        *b = c + m;
    } else if (h >= 300.0 && h < 360.0) {
        *r = c + m;
        *g = m;
        *b = x + m;
    } else {
        *r = m;
        *g = m;
        *b = m;
    }
}

void scale_log(png_byte *ptr, double val) {
    // Make a monochromatic black-on-white output for now.
    double db = log10(val);
    if (db > maxdb)
        maxdb = db;
    if (db < mindb)
        mindb = db;

    // Kind of arbitrarily picked.
    int32_t v = (int32_t)((db * 85.0f) + 200.0f);

    if (v > maxval)
        maxval = v;
    if (v < minval)
        minval = v;

    v = 255 - v;

    if (v > 255)
        v = 255;
    if (v < 0)
        v = 0;

    ptr[0] = (uint8_t)v;
    ptr[1] = (uint8_t)v;
    ptr[2] = (uint8_t)v;
}

void scale_log_hue(png_byte *ptr, double val) {
    double h = 0.0, s = 0.0, v = 0.0;
    double r, g, b;

    double db = log10(val);
    if (db > maxdb)
        maxdb = db;
    if (db < mindb)
        mindb = db;

    // Kind of arbitrarily picked.
    h = (db + 4.0) / 4.0;
    s = 1.0;
    v = 1.0;

    if (h > maxval)
        maxval = h;
    if (h < minval)
        minval = h;

    if (h > 1.0)
        h = 1.0;
    if (h < 0.0)
        h = 0.0;

    hsv_to_rgb(&r, &g, &b, h, s, v);

    ptr[0] = (uint8_t)(255.0 * r);
    ptr[1] = (uint8_t)(255.0 * g);
    ptr[2] = (uint8_t)(255.0 * b);
}

void scale_linear(png_byte *ptr, double val) {
    int32_t v = (int32_t)(val * 100.0f);
    if (v > maxval)
        maxval = v;
    if (v < minval)
        minval = v;

    if (v > 255)
        v = 255;
    if (v < 0)
        v = 0;

    // Make a monochromatic purple output for now.
    ptr[0] = (uint8_t)v;
    ptr[1] = 0;
    ptr[2] = (uint8_t)v;
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
