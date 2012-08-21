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

#ifndef M_PI
	#define M_PI 3.14159265358979323846264338327	
#endif

#ifndef M_PI_2
	#define M_PI_2 6.28318530717958647692528676654
#endif
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
 * in-place FFT
*/
void fft(Complex *input, Complex *table, u_int input_size, int dir); 

/* Complex-to-Complex FFT */
void fft_cc(Complex *output, Complex *input, Complex *table, u_int input_size, u_int fft_size, int dir); 

/* Polar-to-Complex FFT */
void fft_pc(Complex *output, Polar *input, Complex *table, u_int input_size, u_int fft_size, int dir); 

/* Real-to-Complex FFT */
void fft_rc(Complex *output, Wave *input, Complex *table, u_int input_size, u_int fft_size, int dir); 

/* create a coefficient lookup table for fft() */
Complex *fft_create_table(int input_size);

void smooth(Polar *polar, Real factor, int size); /* polar smoothing */

/*

windowing functions

start: start sample
stop: stop sample
dir: left or rigth window

*/

void window(Real *data, int start, int stop, int dir, int type);
void window_complex(Complex *data, int start, int stop, int dir, int type);

#endif /* FFT_H */
