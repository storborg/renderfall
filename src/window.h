#pragma once

#include <stdint.h>

#include <fftw3.h>

typedef struct {
    uint32_t size;
    double *coeffs;
} window_t;

window_t make_window_hann(uint32_t n);
window_t make_window_square(uint32_t n);
window_t make_window_gaussian(uint32_t n, double alpha);
window_t make_window_blackman(uint32_t n);
window_t make_window_blackman_harris(uint32_t n);
window_t make_window_hamming(uint32_t n);
window_t make_window_kaiser(uint32_t n, double beta);
window_t make_window_parzen(uint32_t n);

void destroy_window(window_t win);

void apply_window(window_t win, fftw_complex *in, fftw_complex *out);
