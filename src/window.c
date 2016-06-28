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

double zero_order_modified_bessel(double n) {
    double result = 1;
    double factorial = 1;
    double half_n = n/2;
    double numerator = 1;
    for(int i = 1; i < 20; i++) {
        numerator *= half_n * half_n;
        factorial *= i;
        result += numerator / (factorial * factorial);
    }
    return result;
}

window_t make_window_kaiser(
        uint32_t n, uint32_t ripple, uint32_t transition_width, uint32_t samp_freq) {
    double *coeffs = (double*) malloc(n * sizeof(double));

    double a = -20 * log10(ripple);
    double tw = 2 * M_PI * (transition_width / samp_freq);
    int m = a > 21 ? ceil((a - 7.95) / (2.285 * tw)) : ceil(5.79/tw);

    double beta;
    if (a <= 21) {
        beta = 0.0;
    } else if (a <= 50 && a > 21) {
        beta = 0.5842 * pow(a - 21, 0.4) + 0.07886 * (a - 21);
    } else if (a > 50) {
        beta = 0.1102 * (a - 8.7);
    }

    double denominator = zero_order_modified_bessel(beta);
    for (uint32_t k = 0; k < n; k++) {
        double inner_val = ((2*n)/m) - 1;
        coeffs[k] = zero_order_modified_bessel(
                beta * sqrt((1 - (inner_val * inner_val)))) / denominator;
    }

    window_t win;
    win.size = n;
    win.coeffs = coeffs;
    return win;
}

window_t make_window_parzen(uint32_t n) {
    double *coeffs = (double*) malloc(n * sizeof(double));

    for (uint32_t k = 0; k < n; k++) {
        double val = 0;
        if (k <= (n/4)) {
            double inner = k/(n/2);
            val = 1 - 6 * (inner * inner) * (1 - inner);
        } else if (k > (n/4) && k <= (n/2)) {
            double inner = 1 - (k/(n/2));
            val = 2 * inner * inner * inner;
        }
        coeffs[k] = val;
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
