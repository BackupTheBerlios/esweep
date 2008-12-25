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
 
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dsp.h"
#include "fft.h"

void fft(Complex *input, Complex *table, long input_size, long dir) {
	
	long step=1;
	long i, j, k, l;
	long runs=(int) (log(input_size)/log(2)+0.5);
	long groups=input_size >> 1; /* input_size/2 */
	long stop;
	long idx;
	long table_size=groups;
	
	double t_real, t_imag; /* temporary */
	double w_real, w_imag; 
	
	/* bit-reverse shuffle */

	for (i=1;i<input_size;i++) {
		
		idx=0;
		for (j=0;j<runs;j++) { /* is this part architecture dependent? */
			idx<<=1;
			idx|=((i >> j) & 0x01);
		}
		
		if (idx > i) {
			t_real=input[i].real;
			t_imag=input[i].imag;
			
			input[i].real=input[idx].real;
			input[i].imag=input[idx].imag;
			
			input[idx].real=t_real;
			input[idx].imag=t_imag;
		}
	}

	/* fft */
	
	/* we have ld(N) runs */
	
	for (i=0;i<runs;i++) {
		
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
				
				w_real=table[l%table_size].real;
				w_imag=dir*table[l%table_size].imag;
				
				/* multiplicate with w^n */
				
				t_real=input[k+step].real*w_real-input[k+step].imag*w_imag;
				t_imag=input[k+step].imag*w_real+input[k+step].real*w_imag;
				
				/* complex substraction of samples */
				
				input[k+step].real=input[k].real-t_real;
				input[k+step].imag=input[k].imag-t_imag;
				
				/* complex addition of samples */
				
				input[k].real+=t_real;
				input[k].imag+=t_imag;
			}
			
			k+=step;
			
		}
		
		groups >>=1; /* groups/=2 */
		step <<= 1; /* step*=2 */
	}
}

Complex *fft_create_table(long input_size) {
	long size=input_size/2;
	long i;
	double angle, delta_a;
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

void c2r(double* output, Complex* input, long input_size) { /* complex 2 real */
	long i;
	
	for (i=0;i<input_size;i++) {
		output[i]=hypot(input[i].real, input[i].imag);
	}
}

void r2c(Complex* output, double* input, long input_size) { /* real 2 complex */
	long i;
	
	for (i=0;i<input_size;i++) {
		output[i].real=input[i];
		output[i].imag=0.0;
	}
}

void p2c(Complex *output, Polar *input, long input_size) { /* Polar 2 Complex */
	long i;
	double abs, arg;
	
	for (i=0;i<input_size;i++) {
		abs=input[i].abs;
		arg=input[i].arg;	
		output[i].real=abs*cos(arg);
		output[i].imag=abs*sin(arg);
	}
}

void c2p(Polar *output, Complex *input, long input_size) { /* Complex 2 Polar */
	long i;
	double real;
	double imag;
	
	for (i=0;i<input_size;i++) {
		real=input[i].real; /* this hack allows us to use a cast from Complex to Polar as output */
		imag=input[i].imag;
		output[i].abs=hypot(real, imag);
		output[i].arg=atan2(imag, real);
	}
}

void smooth(Polar *polar, double factor, long size) {
	Polar *tmp;
	long i, j, k;
	long range;
	long herm_size=size/2;
	
	/* unwrap phase */
	/* do not touch the nyquist frequency */
	dsp_unwrapPhase(polar, herm_size);
	tmp=(Polar*) calloc(herm_size, sizeof(Polar));
	
	for (i=1;i<herm_size;i++) {
		range=(long) (i/(2*factor)+0.5);
		tmp[i].abs=tmp[i].arg=0.0;
		for (j=i-range, k=0;j<=i+range;j++) {
			if ((j>0) && (j<=herm_size)) {
				tmp[i].abs+=polar[j].abs;
				tmp[i].arg+=polar[j].arg;
				k++;
			}
		}
		tmp[i].abs/=k;
		tmp[i].arg/=k;
	}
	
	/* copy back */
	for (i=1;i<herm_size;i++) {
		polar[i].abs=tmp[i].abs;
		polar[i].arg=tmp[i].arg;
	}
	
	/* wrap phase */
	dsp_wrapPhase(polar, herm_size);
	
	for (i=1;i<herm_size;i++) {
		polar[size-i].abs=polar[i].abs;
		polar[size-i].arg=-polar[i].arg;
	}
	
	free(tmp);
}


/* see literature */

void window(double *data, long start, long stop, long dir, long type) {
	double alpha;
	double beta;
	double mu;
	double eta;
	long i;
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

void window_complex(Complex *data, long start, long stop, long dir, long type) {
	double alpha;
	double beta;
	double mu;
	double eta;
	long i;
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
