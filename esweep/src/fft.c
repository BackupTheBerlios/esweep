/*
 * Copyright (c) 2007, 2009 Jochen Fabricius <jfab@berlios.de>
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

 /* FFT and utility functions, like converters from Complex to Polar and windowing*/

/* 
 * src/fft.c:
 * 22.11.2009, jfab: added Complex2Complex, Polar2Complex, Real2Complex FFTs
 * 17.12.2009, jfab: moved the FFT kernel into an inline function
 * 		     added an alternative FFT kernel, which is significantly faster with small FFT lengths
 * */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esweep.h"
#include "dsp.h"
#include "fft.h"

static __inline void fft_kernel(Complex *data, Complex *table, u_int length, u_int ld_length, int dir); 

void fft(Complex *input, Complex *table, u_int input_size, int dir) {
	
	int i, j;
	int runs=(int) (log(input_size)/log(2)+0.5);
	int idx;
	
	Complex t; /* temporary */

	/* bit-reverse shuffle */

	for (i=1;i<input_size;i++) {
		
		idx=0;
		for (j=0;j<runs;j++) { 
			idx<<=1;
			idx|=((i >> j) & 0x01);
		}
		
		if (idx > i) {
			t.real=input[i].real;
			t.imag=input[i].imag;
			
			input[i].real=input[idx].real;
			input[i].imag=input[idx].imag;
			
			input[idx].real=t.real;
			input[idx].imag=t.imag;
		}
	}

	/* fft */
	fft_kernel(input, table, input_size, runs, dir); 
}
	
/* Complex-to-Complex FFT */
void fft_cc(Complex *output, Complex *input, Complex *table, u_int input_size, u_int fft_size, int dir) {
	int i, j;
	int runs=(int) (log(fft_size)/log(2)+0.5);
	int idx;
	
	/* bit-reverse shuffle */

	for (i=0;i<input_size;i++) {
		idx=0;
		for (j=0;j<runs;j++) { 
			idx<<=1;
			idx|=((i >> j) & 0x01);
		}

		output[idx].real=input[i].real;
		output[idx].imag=input[i].imag;
	}

	/* fft */
	fft_kernel(output, table, fft_size, runs, dir); 
}

/* Polar-to-Complex FFT */
void fft_pc(Complex *output, Polar *input, Complex *table, u_int input_size, u_int fft_size, int dir) {
	int i, j;
	int runs=(int) (log(fft_size)/log(2)+0.5);
	int idx;
	
	/* bit-reverse shuffle */

	for (i=0;i<input_size;i++) {
		idx=0;
		for (j=0;j<runs;j++) { 
			idx<<=1;
			idx|=((i >> j) & 0x01);
		}

		output[idx].real=input[i].abs*cos(input[i].arg);
		output[idx].imag=input[i].abs*sin(input[i].arg);
	}

	/* fft */
	fft_kernel(output, table, fft_size, runs, dir); 
	
}

/* Real-to-Complex FFT */
void fft_rc(Complex *output, Wave *input, Complex *table, u_int input_size, u_int fft_size, int dir) {
	int i, j;
	int runs=(int) (log(fft_size)/log(2)+0.5);
	int idx;
	
	/* bit-reverse shuffle */

	for (i=0;i<input_size;i++) {
		idx=0;
		for (j=0;j<runs;j++) { 
			idx<<=1;
			idx|=((i >> j) & 0x01);
		}
		output[idx].real=input[i];
		output[idx].imag=0.0;
	}

	/* fft */
	
	/* TODO: exploit the hermitian redundancy */
	fft_kernel(output, table, fft_size, runs, dir); 
	
}
#ifndef ALTFFT
static __inline void fft_kernel(Complex *data, Complex *table, u_int length, u_int ld_length, int dir) {
	u_int groups=length >> 1; /* input_size/2 */
	u_int table_size=groups;
	u_int stop;
	u_int i, j, k, l;
	u_int step=1;
	
	Complex t; /* temporary */
	Complex w; 
	for (i=0;i<ld_length;i++) {
		
		k=0;
		
		/* 
		in each run we have less groups, 
		reducing by factor 2, starting with N/2
		*/
		
		for (j=0;j<groups;j++) {
			
			/* 
			each group consists of N/groups members
			but only the first half is directly accessed
			the second half only through k+step
			*/
			
			stop=k+step;
			
			for (l=0;k<stop;k++, l+=groups) {
				w.real=table[l%table_size].real;
				w.imag=dir*table[l%table_size].imag;
				
				
				t.real=data[k+step].real*w.real-data[k+step].imag*w.imag;
				t.imag=data[k+step].imag*w.real+data[k+step].real*w.imag;
				
				/* complex substraction of samples */
				
				data[k+step].real=data[k].real-t.real;
				data[k+step].imag=data[k].imag-t.imag;
				
				/* complex addition of samples */
				
				data[k].real+=t.real;
				data[k].imag+=t.imag;
			}
			
			k+=step;
			
		}
		
		groups >>=1; /* groups/=2 */
		step <<= 1; /* step*=2 */
	}
}

#else

/* The alternative kernel. For small FFT sizes, this kernel is significant faster than the standard kernel above. 
 * But, unfortunately, when the FFT size (plus the table size) exceeds the processor cache, you will get a huge slowdown */

static __inline void fft_kernel(Complex *data, Complex *table, u_int length, u_int ld_length, int dir) {
	u_int groups=length >> 1; /* input_size/2 */
	u_int table_size=groups;
	u_int i, j, k, l;
	u_int step=1;
	
	Complex t; /* temporary */
	Complex w; 
	
	for (i=0;i<ld_length;i++) {
		
		groups=1 << i;
		step=2<<i; 
		
		for (j=0, l=0;j<groups;j++, l+=(table_size/groups)) {

			w.real=table[l].real;
			w.imag=dir*table[l].imag;

			for (k=groups+j;k<length;k+= step) {
		
				/* multiplicate with w^n */
				
				t.real=data[k].real*w.real-data[k].imag*w.imag;
				t.imag=data[k].imag*w.real+data[k].real*w.imag;
				
				/* complex substraction of samples */
				
				data[k].real=data[k-groups].real-t.real;
				data[k].imag=data[k-groups].imag-t.imag;
				
				/* complex addition of samples */
				
				data[k-groups].real+=t.real;
				data[k-groups].imag+=t.imag;
			}
		}
	}
}
#endif

Complex *fft_create_table(int input_size) {
	int size=input_size/2;
	int i;
	Real angle, delta_a;
	Complex *table=(Complex*) calloc(size, sizeof(Complex));
	
	delta_a=2*M_PI/input_size;
	
	angle=0.0;
	for (i=0;i<size;i++) {
		table[i].real=cos(angle);
		table[i].imag=sin(angle);
		angle+=delta_a;
	}
	
	return table;
}

/* see literature */

void window(Real *data, int start, int stop, int dir, int type) {
	Real alpha;
	Real beta;
	Real mu;
	Real eta;
	int i;
	switch (type) {
		case WIN_BART:
			alpha=1.0;
			beta=-1.0;
			mu=0.0;
			eta=0.0;
			break;
			
		case WIN_HANN:
			alpha=0.5;
			beta=0.0;
			mu=0.5;
			eta=0.0;
			break;
		
		case WIN_HAMM:
			alpha=0.54;
			beta=0.0;
			mu=0.46;
			eta=0.0;
			break;
			
		case WIN_BLACK:
			alpha=0.42;
			beta=0.0;
			mu=0.5;
			eta=0.08;
			break;
			
			case WIN_NOWIN: /* FALLTHROUGH */
				case WIN_RECT: /* FALLTHROUGH */
		default:
			alpha=1.0;
			beta=0.0;
			mu=0.0;
			eta=0.0;
			break;	
	}
	
	if (dir==WIN_LEFT) {
		for (i=start;i<stop;i++) {
			data[i]*=(alpha+beta*abs(i-stop-1)/(stop-start)+mu*cos(M_PI*(i-stop)/(stop-start))+eta*cos(2*M_PI*(i-stop)/(stop-start)));
		}
	} else {
		for (i=start;i<stop;i++) {
			data[i]*=(alpha+beta*abs(i-start)/(stop-start)+mu*cos(M_PI*(i-start)/((stop-start)))+eta*cos(2*M_PI*(i-start)/(stop-start)));
		}
	}
}

void window_complex(Complex *data, int start, int stop, int dir, int type) {
	Real alpha;
	Real beta;
	Real mu;
	Real eta;
	int i;
	switch (type) {
		case WIN_BART:
			alpha=1.0;
			beta=-1.0;
			mu=0.0;
			eta=0.0;
			break;
			
		case WIN_HANN:
			alpha=0.5;
			beta=0.0;
			mu=0.5;
			eta=0.0;
			break;
		
		case WIN_HAMM:
			alpha=0.54;
			beta=0.0;
			mu=0.46;
			eta=0.0;
			break;
			
		case WIN_BLACK:
			alpha=0.42;
			beta=0.0;
			mu=0.5;
			eta=0.08;
			break;
			
			case WIN_NOWIN: /* FALLTHROUGH */
				case WIN_RECT: /* FALLTHROUGH */
		default:
			alpha=1.0;
			beta=0.0;
			mu=0.0;
			eta=0.0;
			break;	
	}
	
	if (dir==WIN_LEFT) {
		for (i=start;i<stop;i++) {
			data[i].real*=(alpha+beta*abs(i-stop-1)/(stop-start)+mu*cos(M_PI*(i-stop)/(stop-start))+eta*cos(2*M_PI*(i-stop)/(stop-start)));
			data[i].imag*=(alpha+beta*abs(i-stop-1)/(stop-start)+mu*cos(M_PI*(i-stop)/(stop-start))+eta*cos(2*M_PI*(i-stop)/(stop-start)));
		}
	} else {
		for (i=start;i<stop;i++) {
			data[i].real*=(alpha+beta*abs(i-start)/(stop-start)+mu*cos(M_PI*(i-start)/((stop-start)))+eta*cos(2*M_PI*(i-start)/(stop-start)));
			data[i].imag*=(alpha+beta*abs(i-start)/(stop-start)+mu*cos(M_PI*(i-start)/((stop-start)))+eta*cos(2*M_PI*(i-start)/(stop-start)));
		}
	}
}
