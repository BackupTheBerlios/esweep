/*
 * Copyright (c) 2010 Jochen Fabricius <jfab@berlios.de>
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

/* esweep_priv.c */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep_priv.h"

char errmsg[256];


int __esweep_assert2(const char *file, int line, const char *func, const char *assertion) {
	snprintf(errmsg, 256, "%s:%i: %s: Assertion %s failed\n", file, line, func, assertion);
	fprintf(stderr, errmsg);
	return 1;
}

int __esweep_assert(const char *file, int line, const char *assertion) {
	snprintf(errmsg, 256, "%s:%i: Assertion %s failed\n", file, line, assertion);
	fprintf(stderr, errmsg);
	return 1;
}


/********************************
 * Private conversion functions *
 ********************************/

__EXTERN_FUNC__ void inline c2r(Real* output, Complex* input, int input_size) { /* complex 2 real */
	int i;
	
	for (i=0;i<input_size;i++) {
		output[i]=input[i].real; 
	}
}

__EXTERN_FUNC__ void inline r2c(Complex* output, Real* input, int input_size) { /* real 2 complex */
	int i;
	
	for (i=0;i<input_size;i++) {
		output[i].real=input[i];
		output[i].imag=0.0;
	}
}

__EXTERN_FUNC__ void inline p2r(Real* output, Polar* input, int input_size) { /* polar 2 real */
	int i;
	
	for (i=0;i<input_size;i++) {
		output[i]=input[i].abs*cos(input[i].arg); 
	}
}

__EXTERN_FUNC__ void inline r2p(Polar* output, Real* input, int input_size) { /* real 2 polar */
	int i;
	
	for (i=0;i<input_size;i++) {
		output[i].abs=input[i];
		output[i].abs=0.0;
	}
}

__EXTERN_FUNC__ void inline abs2r(Real* output, Polar* input, int input_size) { /* abs 2 real */
	int i;
	
	for (i=0;i<input_size;i++) {
		output[i]=input[i].abs; 
	}
}

__EXTERN_FUNC__ void inline p2c(Complex *output, Polar *input, int input_size) { /* Polar 2 Complex */
	int i;
	Real abs, arg;
	
	for (i=0;i<input_size;i++) {
		abs=input[i].abs;
		arg=input[i].arg;	
		output[i].real=abs*cos(arg);
		output[i].imag=abs*sin(arg);
	}
}

__EXTERN_FUNC__ void inline c2p(Polar *output, Complex *input, int input_size) { /* Complex 2 Polar */
	int i;
	Real real;
	Real imag;
	
	for (i=0;i<input_size;i++) {
		real=input[i].real; /* this hack allows us to use a cast from Complex to Polar as output */
		imag=input[i].imag;
		output[i].abs=hypot(real, imag);
		output[i].arg=atan2(imag, real);
	}
}

#ifdef REAL32
	#define MAXREAL FLT_MAX
	#define MINREAL FLT_MIN
#else
	#define MAXREAL DBL_MAX
	#define MINREAL DBL_MIN
#endif


__EXTERN_FUNC__ int inline correctFpException(esweep_object* obj) {
	int i; 

	Wave *wave; 
	Polar *polar; 
	Complex *cpx; 
	Surface *surf; 

	if (fetestexcept(FE_INVALID)) {
		fprintf(stderr, "Invalid floating point operation occured.\n");
		feclearexcept(FE_INVALID); 
		return 0;
	}

	if (fetestexcept(FE_OVERFLOW | FE_DIVBYZERO)) {
		// find the errors and correct them by rounding to neares representable
		switch (obj->type) {
			case WAVE:
				wave=(Wave*) obj->data; 
				for (i=0; i < obj->size; i++) {
					if (isinf(wave[i])) {
						if (signbit(wave[i])) {
							wave[i]=MINREAL; 
						} else {
							wave[i]=MAXREAL; 
						}
					}
				}
				break; 
			case POLAR:
				polar=(Polar*) obj->data; 
				for (i=0; i < obj->size; i++) {
					if (isinf(polar[i].abs)) {
						if (signbit(polar[i].abs)) {
							polar[i].abs=MINREAL; 
						} else {
							polar[i].abs=MAXREAL; 
						}
					}
					if (isinf(polar[i].arg)) {
						if (signbit(polar[i].arg)) {
							polar[i].arg=MINREAL; 
						} else {
							polar[i].arg=MAXREAL; 
						}
					}
				}
				break; 
			case COMPLEX:
				cpx=(Complex*) obj->data; 
				for (i=0; i < obj->size; i++) {
					if (isinf(cpx[i].real)) {
						if (signbit(cpx[i].real)) {
							cpx[i].real=MINREAL; 
						} else {
							cpx[i].real=MAXREAL; 
						}
					}
					if (isinf(cpx[i].imag)) {
						if (signbit(cpx[i].imag)) {
							cpx[i].imag=MINREAL; 
						} else {
							cpx[i].imag=MAXREAL; 
						}
					}
				}
				break; 
			case SURFACE: 
			default: 
				break; 
		}
		feclearexcept(FE_OVERFLOW | FE_DIVBYZERO); 
	}
	return 1; 
}


