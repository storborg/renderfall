#pragma once

#include <stdint.h>

#include <png.h>

#include "window.h"
#include "formats.h"

typedef struct {
    window_t win;
    uint32_t overlap;
    uint32_t fftsize;
    uint32_t frames;
    read_samples_fn reader;
} waterfall_params_t;

void waterfall(png_structp png_ptr, FILE* fp, waterfall_params_t params);
