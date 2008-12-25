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

%module esweep

%{
#include "esweep.h"
%}

/* memory functions */

esweep_data *esweep_free(esweep_data *data);
esweep_data *esweep_copy(esweep_data *src); 
esweep_data *esweep_create(const char *type, long samplerate, long size);

/* info functions */

const char *esweep_type(esweep_data *data);
long esweep_samplerate(esweep_data *data);
long esweep_setSamplerate(esweep_data *a, long samplerate);
long esweep_size(esweep_data *data);

long esweep_maxPos(esweep_data *a);

double esweep_getX(esweep_data *a, long i);
double esweep_getY(esweep_data *a, long i);
double esweep_getZ(esweep_data *a, long x, long y);

/* math */

double esweep_max(esweep_data *a);
double esweep_min(esweep_data *a);
double esweep_mean(esweep_data *a);
double esweep_sum(esweep_data *a, long lo, long hi);

long esweep_real(esweep_data *a);
long esweep_imag(esweep_data *a);
long esweep_abs(esweep_data *a);
long esweep_arg(esweep_data *a);

long esweep_add(esweep_data *a, esweep_data *b);
long esweep_sub(esweep_data *a, esweep_data *b);
long esweep_mul(esweep_data *a, esweep_data *b);
long esweep_div(esweep_data *a, esweep_data *b);

long esweep_addScalar(esweep_data *a, double re, double im);
long esweep_subScalar(esweep_data *a, double re, double im);
long esweep_mulScalar(esweep_data *a, double re, double im);
long esweep_divScalar(esweep_data *a, double re, double im);

long esweep_ln(esweep_data *a);
long esweep_lg(esweep_data *a);

long esweep_exp(double x, esweep_data *a);
long esweep_pow(esweep_data *a, double x);

long esweep_schroeder(esweep_data *a);

/* dsp */

long esweep_fft(esweep_data *a, esweep_data *table);
long esweep_ifft(esweep_data *a, esweep_data *table);
long esweep_createFftTable(esweep_data *table, long size);

long esweep_convolve(esweep_data *a, esweep_data *filter, esweep_data *table);
long esweep_deconvolve(esweep_data *a, esweep_data *filter, esweep_data *table);

long esweep_hilbert(esweep_data *a, esweep_data *table);
long esweep_analytic(esweep_data *a, esweep_data *table);

long esweep_integrate(esweep_data *a);
long esweep_differentiate(esweep_data *a);

long esweep_wrapPhase(esweep_data *a);
long esweep_unwrapPhase(esweep_data *a);

long esweep_groupDelay(esweep_data *a);
long esweep_gDelayToPhase(esweep_data *a);

long esweep_smooth(esweep_data *a, double factor);

long esweep_window(esweep_data *a, const char *left_win, double left_width, const char *right_win, double right_width);

/* generate */

long esweep_concat(esweep_data *dst, esweep_data *src, long dst_pos, long src_pos);
esweep_data *esweep_cut(esweep_data *a, long start, long stop);
long esweep_pad(esweep_data *a, long size);

double esweep_genSine(esweep_data *output, double freq, double phase);
double esweep_genLogsweep(esweep_data *output, double f1, double f2, const char *spec);
double esweep_genLinsweep(esweep_data *output, double f1, double f2, const char *spec);
long esweep_genNoise(esweep_data *output, double f1, double f2, const char *spec);

/* convert */

long esweep_toComplex(esweep_data *a);
long esweep_toPolar(esweep_data *a);

/* file handling */

long esweep_toAscii(const char *output, esweep_data *input);
long esweep_save(const char *output, esweep_data *input);
long esweep_load(esweep_data *output, const char *input);

/* audio */

long esweep_audioOpen(const char *device, long samplerate);
long esweep_audioSetData(esweep_data *out, esweep_data *left, esweep_data *right);
long esweep_audioGetData(esweep_data *left, esweep_data *right, esweep_data *in);
long esweep_audioPlay(esweep_data *out, long handle);
long esweep_audioRecord(esweep_data *in, long handle);
long esweep_audioMeasure(esweep_data *in, esweep_data *out, long handle);
long esweep_audioClose(long handle);

/* surface */

long esweep_csd(esweep_data *out, esweep_data *in, double time_step, long steps, double rise_time, double smooth_factor);
long esweep_cbsd(esweep_data *out, esweep_data *in, double f1, double f2, long resolution, long periods, long steps, char time_shift);

long esweep_sparseSurface(esweep_data *a, long xsize, long ysize);
long esweep_addToSurface(esweep_data *a, esweep_data *b, char axis, long index, double dep);