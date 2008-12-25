/*
 * Copyright (c) 2007 Jochen Fabricius <jfab@berlios.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef FFT_H

#define FFT_H

#include "esweep.h"

#define FFT_FORWARD -1
#define FFT_BACKWARD 1

#define WIN_LEFT 0
#define WIN_RIGHT 1

/*
the available window functions
*/

enum Window {
	WIN_NOWIN,
	WIN_RECT,
	WIN_BART,
	WIN_HANN,
	WIN_HAMM,
	WIN_BLACK
};

/*
 * standard in-place transform, either fft (dir=FFT_FORWARD) or ifft (dir=FFT_BACKWARD)
*/

void fft(Complex *input, Complex *table, long input_size, long dir);

/* create a coefficient lookup table for fft() */
Complex *fft_create_table(long input_size);

void c2r(double* output, Complex* input, long input_size); /* Complex to Real */
void r2c(Complex* output, double* input, long input_size); /* Real to Complex */

void p2c(Complex *output, Polar *input, long input_size); /* Polar 2 Complex */
void c2p(Polar *output, Complex *input, long input_size); /* Complex 2 Polar */

void smooth(Polar *polar, double factor, long size); /* polar smoothing */

/*

windowing functions

start: start sample
stop: stop sample
dir: left or rigth window

*/

void window(double *data, long start, long stop, long dir, long type);
void window_complex(Complex *data, long start, long stop, long dir, long type);

#endif /* FFT_H */
