#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <fftw3.h>

#include "window.h"

window_t make_window_hann(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));
    printf("Using a Hann window of size %d.\n", n);
    for (uint32_t k = 0; k < n; k++) {
        coeffs[k] = (1 - cos((2.0f * M_PI * k) / (n - 1))) / 2.0f;
    }

    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

window_t make_window_square(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));
    printf("Using a square window.\n");
    for (uint32_t k = 0; k < n; k++) {
        coeffs[k] = 1.0f;
    }
    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

window_t make_window_gaussian(uint32_t n, double beta) {
    double *coeffs = (double*) malloc(n * sizeof(double));
    printf("Using a Gaussian window of size %d, beta %0.3f\n", n, beta);

    double arg;
    for (uint32_t k = 0; k < n; k++) {
        arg = (beta * (1.0 - ((double) k / (double) n) * 2.0));
        coeffs[k] = exp(-0.5 * (arg * arg));
    }

    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

void destroy_window(window_t win) {
    free(win.coeffs);
}

void apply_window(window_t win, fftw_complex *in, fftw_complex *out) {
    double *coeffs = win.coeffs;
    for (uint32_t i = 0; i < win.size; i++) {
        out[i][0] = coeffs[i] * in[i][0];
        out[i][1] = coeffs[i] * in[i][1];
    }
}
