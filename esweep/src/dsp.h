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

#ifndef DSP_H

#define DSP_H

#include "fft.h"

/* 
 * convolution and deconvolution
 * both arrays, signal and kernel, must be of the same size 2^ceil(ld(N+M-1))
 */

void dsp_convolve(Complex *signal, Complex *kernel, Complex *table, int size);
void dsp_deconvolve(Complex *signal, Complex *kernel, Complex *table, int size);
void dsp_hilbert(Complex *signal, Complex *table, int size);
void dsp_wrapPhase(Polar *polar, int size);
void dsp_unwrapPhase(Polar *polar, int size);

#endif /* DSP_H */
 
