#include <stdint.h>

#include <png.h>
#include <fftw3.h>

#include "waterfall.h"
#include "shell.h"
#include "colormap.h"

void shiftleft(fftw_complex arr[], uint32_t size, uint32_t shift) {
    for (uint32_t i = 0; i < (size - shift); i++) {
        arr[i][0] = arr[i + shift][0];
        arr[i][1] = arr[i + shift][1];
    }
}

void waterfall(png_structp png_ptr, FILE* fp, waterfall_params_t params) {
    png_bytep row = (png_bytep) malloc(3 * params.fftsize * sizeof(png_byte));

    fftw_complex *in, *out, *inter;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * params.fftsize);
    inter = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * params.fftsize);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * params.fftsize);
    p = fftw_plan_dft_1d(params.fftsize, inter, out, FFTW_FORWARD, FFTW_PATIENT);

    uint32_t half = params.fftsize / 2;
    uint32_t x, y;

    start_progress();

    for (y = 0; y < params.frames; y++) {
        // read an FFT-worth of complex samples and convert them to doubles

        // if overlap is nonzero, left shift the 'in' array by the overlap
        // amount first
        if (params.overlap > 0) {
            shiftleft(in, params.fftsize, params.overlap);
        }

        // read in fft size minus overlap, starting at overlap offset into
        // array
        params.reader(fp, in + params.overlap, params.fftsize - params.overlap);

        // apply a window we created earlier
        apply_window(params.win, in, inter);

        fftw_execute(p);

        // convert output from doubles to colors

        // first half (negative frequencies)
        for (x = 0; x < half; x++) {
            render_complex(&(row[x * 3]), out[x + half]);
        }

        // second half (positive frequencies)
        for (x = half; x < params.fftsize; x++) {
            render_complex(&(row[x * 3]), out[x - half]);
        }

        png_write_row(png_ptr, row);

        update_progress(y, params.frames);
    }

    end_progress();

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(inter);
    fftw_free(out);
}


