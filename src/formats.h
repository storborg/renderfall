#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <fftw3.h>

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

void read_samples_int8(FILE *fp, fftw_complex *buf, uint32_t n);
void read_samples_int16(FILE *fp, fftw_complex *buf, uint32_t n);
void read_samples_uint16(FILE *fp, fftw_complex *buf, uint32_t n);
void read_samples_float32(FILE *fp, fftw_complex *buf, uint32_t n);
void read_samples_float64(FILE *fp, fftw_complex *buf, uint32_t n);
