#include <stdint.h>

#include "formats.h"

void read_samples_int8(FILE *fp, fftw_complex *buf, uint32_t n) {
    int8_t *raw = (int8_t*) malloc(n * 2 * sizeof(int8_t));
    fread(raw, 2, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((double) raw[i] / INT8_MAX);
        buf[i][1] = ((double) raw[i + 1] / INT8_MAX);
    }
    free(raw);
}

void read_samples_int16(FILE *fp, fftw_complex *buf, uint32_t n) {
    int16_t *raw = (int16_t*) malloc(n * 2 * sizeof(int16_t));
    fread(raw, 4, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((double) raw[i * 2] / INT16_MAX);
        buf[i][1] = ((double) raw[(i * 2) + 1] / INT16_MAX);
    }
    free(raw);
}

void read_samples_uint16(FILE *fp, fftw_complex *buf, uint32_t n) {
    uint16_t *raw = (uint16_t*) malloc(n * 2 * sizeof(uint16_t));
    fread(raw, 4, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = ((double) raw[i * 2] / UINT16_MAX);
        buf[i][1] = ((double) raw[(i * 2) + 1] / UINT16_MAX);
    }
}

void read_samples_float32(FILE *fp, fftw_complex *buf, uint32_t n) {
    size_t sample_size = 2 * sizeof(float);
    float *raw = (float*) malloc(n * sample_size);
    fread(raw, sample_size, n, fp);
    for (uint32_t i = 0; i < n; i++) {
        buf[i][0] = (double) raw[i * 2];
        buf[i][1] = (double) raw[(i * 2) + 1];
    }
}
