#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <fftw3.h>

#include "window.h"

window_t make_window_hann(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));
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

window_t make_window_blackman(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));

    double alpha = 0.16;
    double a0, a1, a2;

    a0 = (1.0 - alpha) / 2.0;
    a1 = 1.0 / 2.0;
    a2 = alpha / 2.0;

    for (uint32_t k = 0; k < n; k++) {
        coeffs[k] = (a0 -
                     (a1 * cos((2 * M_PI * k) / (n - 1))) +
                     (a2 * cos((4 * M_PI * k) / (n - 1))));
    }

    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

window_t make_window_hamming(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));
    double alpha = 0.54;
    double beta = 0.46;
    double constant = alpha - beta;

    for (uint32_t k = 0; k < n; k++) {
        coeffs[k] = constant * cos((2* M_PI * k) / (n - 1));
    }

    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

window_t make_window_blackman_harris(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));
    double a0 = 0.35875;
    double a1 = 0.48829;
    double a2 = 0.14128;
    double a3 = 0.01168;

    for (uint32_t k = 0; k < n; k++) {
        coeffs[k] = (a0-a1) * cos((2 * M_PI * k) / (n -1)) +
                    a2 * cos((4 * M_PI * k) / (n -1)) +
                    a3 * cos((6 * M_PI * k) / (n -1));

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
