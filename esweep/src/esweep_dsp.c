/*
 * Copyright (c) 2009, 2010, 2011 Jochen Fabricius <jfab@berlios.de>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esweep.h"

#include "dsp.h"
#include "fft.h"

/*
 * src/esweep_dsp.c:
 * Various DSP functions
 * 25.11.2009, jfab: new FFT interface
 * 10.01.2010, jfab: allow in-place FFT transforms
 * 14.05.2010, jfab: update to new ESWEEP_OBJ_ macros
 * 28.09.2011, jfab: bringing functions to newest style
*/

int esweep_fft(esweep_object *out, esweep_object *in, esweep_object *table) {
	int fft_size;
	Complex *fft_table;
	Complex *cpx;

	ESWEEP_OBJ_NOTEMPTY(in, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_ISVALID(out, ERR_OBJ_NOT_VALID);

	switch (in->type) {
		case WAVE:
		case POLAR: /* Fallthrough */
		case COMPLEX:
			/* set fft size to power of two */
			fft_size=(int) (pow(2, ceil(log(in->size)/log(2)))+0.5);
			break;
		case SURFACE: /* Fallthrough */
		default:
			ESWEEP_NOT_THIS_TYPE(in->type, ERR_NOT_ON_THIS_TYPE);
	}

	if (in == out) { /* in-place FFT */
		if (in->type != COMPLEX || in->size != fft_size) {
			ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
			switch (in->type) {
				case WAVE:
					r2c(cpx, in->data, in->size);
					break;
				case POLAR:
					p2c(cpx, in->data, in->size);
					break;
				case COMPLEX:
					memcpy(cpx, in->data, in->size*sizeof(Complex));
					break;
				default: /* any other case should have been handled by the switch-statement above */
					break;
			}
			free(in->data);
			in->size=fft_size; 
			in->data=cpx;
			in->type=COMPLEX;
		}
	} else {
		switch (out->type) {
			case SURFACE: /* Fallthrough */
			case WAVE:
				free(out->data);
				out->size=fft_size;
				out->type=COMPLEX;
				ESWEEP_MALLOC(out->data, fft_size, sizeof(Complex), ERR_MALLOC);
				break;
			case POLAR:
				out->type=COMPLEX;
				/* Fallthrough, still need to check the size */
			case COMPLEX:
				/*
				 * Make sure that out->data has at least size fft_size.
				 * out->size may be greater than fft_size. This allows some elegant
				 * possibilities, e. g. during linear convolution.
				 */
				if (out->size < fft_size) {
					free(out->data);
					out->size=fft_size;
					ESWEEP_MALLOC(out->data, fft_size, sizeof(Complex), ERR_MALLOC);
				} else {
					/* set the output array to zero */
					memset(out->data, 0, out->size*sizeof(Complex));
				}
				break;
			default:
				return ERR_UNKNOWN; /* should never happen */
		}
	}

	/* create FFT table if not given or has the wrong size or the wrong type */

	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) table->data;
	/* do FFT */

	switch (in->type) {
		case WAVE:
			fft_rc((Complex*) (out->data), (Wave*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
			break;
		case POLAR:
			fft_pc((Complex*) (out->data), (Polar*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
			break;
		case COMPLEX:
			if (in == out) {
				/* in-place */
				fft((Complex*) out->data, fft_table, fft_size, FFT_FORWARD);
			} else {
				fft_cc((Complex*) (out->data), (Complex*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			return ERR_UNKNOWN;  /* should never happen */
	}

	/* if we created the fft table inside this function free it */
	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);


	return ERR_OK;
}

/* inverse FFT; see above */
int esweep_ifft(esweep_object *out, esweep_object *in, esweep_object *table) {
	int fft_size;
	Complex *fft_table;
	Complex *cpx;

	ESWEEP_OBJ_NOTEMPTY(in, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_ISVALID(out, ERR_OBJ_NOT_VALID);

	switch (in->type) {
		case WAVE: /* Fallthrough */
		case POLAR: /* Fallthrough */
		case COMPLEX:
			/* set fft size to power of two */
			fft_size=(int) (pow(2, ceil(log(in->size)/log(2)))+0.5);
			break;
		case SURFACE: /* Fallthrough */
		default:
			ESWEEP_NOT_THIS_TYPE(in->type, ERR_NOT_ON_THIS_TYPE);
	}

	if (in == out) { /* in-place FFT */
		if (in->type != COMPLEX || in->size != fft_size) {
			ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
			switch (in->type) {
				case WAVE:
					r2c(cpx, in->data, in->size);
					break;
				case POLAR:
					p2c(cpx, in->data, in->size);
					break;
				case COMPLEX:
					memcpy(cpx, in->data, in->size*sizeof(Complex));
					break;
				default: /* any other case should be handled by the switch-statement above */
					break;
			}
			free(in->data);
			in->size=fft_size; 
			in->data=cpx;
			in->type=COMPLEX;
		}
	} else {
		switch (out->type) {
			case SURFACE: /* Fallthrough */
			case WAVE:
				free(out->data);
				out->size=fft_size;
				out->type=COMPLEX;
				ESWEEP_MALLOC(out->data, fft_size, sizeof(Complex), ERR_MALLOC);
				break;
			case POLAR:
				out->type=COMPLEX;
				/* Fallthrough, still need to check the size */
			case COMPLEX:
				/*
				 * Make sure that out->data has at least of size fft_size.
				 * out->size may be greater than fft_size. This allows some elegant
				 * possibilities, e. g. during linear convolution.
				 */
				if (out->size < fft_size) {
					free(out->data);
					out->size=fft_size;
					ESWEEP_MALLOC(out->data, fft_size, sizeof(Complex), ERR_MALLOC);
				} else {
					/* set the output array to zero */
					memset(out->data, 0, fft_size*sizeof(Complex));
				}
				break;
			default:
				return ERR_UNKNOWN; /* should never happen */
		}
	}

	/* create FFT table if not given or has the wrong size or the wrong type */

	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) table->data;

	/* do FFT */

	switch (in->type) {
		case WAVE:
			fft_rc((Complex*) (out->data), (Wave*) (in->data), fft_table, in->size, fft_size, FFT_BACKWARD);
			break;
		case POLAR:
			fft_pc((Complex*) (out->data), (Polar*) (in->data), fft_table, in->size, fft_size, FFT_BACKWARD);
			break;
		case COMPLEX:
			if (in == out) {
				fft((Complex*) out->data, fft_table, fft_size, FFT_BACKWARD);
			} else {
				fft_cc((Complex*) (out->data), (Complex*) (in->data), fft_table, in->size, fft_size, FFT_BACKWARD);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			return ERR_UNKNOWN;  /* should never happen */
	}

	/* if we created the fft table inside this function free it */
	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);

	return ERR_OK;
}

/*
creates an FFT table for FFT/IFFT/(De)Convolution
output is always COMPLEX, size is automatically adjusted to a power of 2
*/

int esweep_createFFTTable(esweep_object *table, int fft_size) {
	int size;

	ESWEEP_OBJ_ISVALID(table, ERR_OBJ_NOT_VALID);
	if (table->data!=NULL) free(table->data);

	if (fft_size <= 0) size = table->size;
	else size=fft_size; 

	size=(int) (pow(2, ceil(log(size)/log(2)))+0.5);

	table->data=(void*) fft_create_table(size);
	table->size=size/2;
	table->type=COMPLEX;

	return ERR_OK;
}

/*
compute a fast convolution of in with filter (in*filter)
output type is type of in.
see esweep_fft for table
both "a" and "filter" may be of types WAVE and COMPLEX
if "filter" is COMPLEX it is assumed as pre-transformed
this saves 1 FFT hence gives a speed-up
*/

int esweep_convolve(esweep_object *in, esweep_object *filter, esweep_object *table) {
	Complex *cpx, *complex_filter=NULL, *fft_table;
	int fft_size;
	int i;
	Real t_real, t_imag; /* temp variables */

	ESWEEP_OBJ_NOTEMPTY(in, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(filter, ERR_EMPTY_OBJECT);
	ESWEEP_SAME_MAPPING(in, filter, ERR_DIFF_MAPPING);

	/* set fft size to power of two */
	fft_size=(int) (pow(2, ceil(log(in->size)/log(2)))+0.5);

	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) table->data;

	switch (filter->type) {
		case WAVE:
			/* transform the filter kernel */
			ESWEEP_MALLOC(complex_filter, fft_size, sizeof(Complex), ERR_MALLOC);
			fft_rc(complex_filter, (Wave*) (filter->data), fft_table, filter->size, fft_size, FFT_FORWARD);
			break;
		case COMPLEX: /* pre-transformed filter kernel */
			if (fft_size!=filter->size) { /* The pre-transformed filter must have the size of the FFT */
				/* if we created the fft table inside this function free it */
				if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
				/* produce a correct error message */
				ESWEEP_ASSERT(filter->size == fft_size, ERR_SIZE_MISMATCH); 
				break; 
			} else {
				complex_filter=(Complex*) (filter->data);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			return ERR_NOT_ON_THIS_TYPE;
	}

	switch (in->type) {
		case WAVE:
			/* The input must be transformed */
			ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
			fft_rc(cpx, (Wave*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
			free(in->data);
			in->data=cpx;
			in->type=COMPLEX;
			in->size=fft_size;
			break;
		case COMPLEX:
			/* The input must be transformed */
			if (fft_size != in->size) {
				ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
				fft_cc(cpx, (Complex*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
				free(in->data);
				in->data=cpx;
				in->size=fft_size;
			} else {
				cpx=(Complex*) (in->data);
				/* In-place transform */
				fft(cpx, fft_table, fft_size, FFT_FORWARD);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			if (filter->type!=COMPLEX) free(complex_filter);
			return ERR_NOT_ON_THIS_TYPE;
	}

	/* Complex multiplication of in and filter. */
	for (i=0;i<fft_size;i++) {
		t_real=cpx[i].real/fft_size; /* correct for fft scaling */
		t_imag=cpx[i].imag/fft_size;

		cpx[i].real=t_real*complex_filter[i].real-t_imag*complex_filter[i].imag;
		cpx[i].imag=t_real*complex_filter[i].imag+t_imag*complex_filter[i].real;
	}

	/* in-place backward transform */
	fft(cpx, fft_table, fft_size, FFT_BACKWARD);

	/* clean up */
	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
	if (filter->type!=COMPLEX) free(complex_filter);

	/* make complex the new data for a */

	return ERR_OK;
}

/*
fast _linear_ deconvolution
see esweep_convolve
*/
int esweep_deconvolve(esweep_object *in, esweep_object *filter, esweep_object *table) {
	Complex *cpx, *complex_filter=NULL, *fft_table;
	int fft_size;
	int i;
	Real t_real, t_imag, t_abs; /* temp variables */

	ESWEEP_OBJ_NOTEMPTY(in, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(filter, ERR_EMPTY_OBJECT);
	ESWEEP_SAME_MAPPING(in, filter, ERR_DIFF_MAPPING);

	/* set fft size to power of two */
	fft_size=(int) (pow(2, ceil(log(in->size)/log(2)))+0.5);

	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) fft_table=fft_create_table(fft_size);
	else fft_table=(Complex*) table->data;

	switch (filter->type) {
		case WAVE:
			/* transform the filter kernel */
			ESWEEP_MALLOC(complex_filter, fft_size, sizeof(Complex), ERR_MALLOC);
			fft_rc(complex_filter, (Wave*) (filter->data), fft_table, filter->size, fft_size, FFT_FORWARD);
			break;
		case COMPLEX: /* pre-transformed filter kernel */
			if (fft_size!=filter->size) { /* The pre-transformed filter must have the size of the FFT */
				/* if we created the fft table inside this function free it */
				if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
				/* produce a correct error message */
				ESWEEP_ASSERT(filter->size == fft_size, ERR_SIZE_MISMATCH); 
				break; 
			} else {
				complex_filter=(Complex*) (filter->data);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			return ERR_NOT_ON_THIS_TYPE;
	}

	switch (in->type) {
		case WAVE:
			/* The input must be transformed */
			ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
			fft_rc(cpx, (Wave*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
			free(in->data);
			in->data=cpx;
			in->type=COMPLEX;
			in->size=fft_size;
			break;
		case COMPLEX:
			/* The input must be transformed */
			if (fft_size != in->size) {
				ESWEEP_MALLOC(cpx, fft_size, sizeof(Complex), ERR_MALLOC);
				fft_cc(cpx, (Complex*) (in->data), fft_table, in->size, fft_size, FFT_FORWARD);
				free(in->data);
				in->data=cpx;
				in->size=fft_size;
			} else {
				cpx=(Complex*) (in->data);
				/* In-place transform */
				fft(cpx, fft_table, fft_size, FFT_FORWARD);
			}
			break;
		default:
			/* if we created the fft table inside this function free it */
			if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
			if (filter->type!=COMPLEX) free(complex_filter);
			return ERR_NOT_ON_THIS_TYPE;
	}

	for (i=0;i<fft_size;i++) {
		t_real=cpx[i].real;
		t_imag=cpx[i].imag;

		t_abs=complex_filter[i].real*complex_filter[i].real+complex_filter[i].imag*complex_filter[i].imag;
		t_abs*=fft_size; /* correct for fft scaling */

		cpx[i].real=(t_real*complex_filter[i].real+t_imag*complex_filter[i].imag)/t_abs;
		cpx[i].imag=(t_imag*complex_filter[i].real-t_real*complex_filter[i].imag)/t_abs;
	}

	/* in-place backward transform */
	fft(cpx, fft_table, fft_size, FFT_BACKWARD);

	/* clean up */
	if (table==NULL || table->size!=fft_size/2 || table->data==NULL || table->type!=COMPLEX) free(fft_table);
	if (filter->type!=COMPLEX) free(complex_filter);

	return ERR_OK;
}

/* delay line with a ringbuffer scheme */
int esweep_delay(esweep_object *signal, esweep_object *line, int *offset) {
	Complex *cpx_sig, *cpx_line; 
	Real *real_sig, *real_line; 
	Real t_real, t_imag; 
	int i, j; 

	ESWEEP_OBJ_NOTEMPTY(signal, ERR_EMPTY_OBJECT);

	ESWEEP_OBJ_ISVALID(line, ERR_EMPTY_OBJECT);
	/* ignore delay lines of size 0 */
	if (line->size == 0) return ERR_OK; 

	ESWEEP_SAME_MAPPING(signal, line, ERR_DIFF_MAPPING);
	ESWEEP_ASSERT(*offset >= 0, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(*offset < line->size, ERR_BAD_ARGUMENT); 

	ESWEEP_ASSERT(signal->type == line->type, ERR_DIFF_TYPES); 


	switch (signal->type) {
		case WAVE: 
			real_sig=(Real*) signal->data; 
			real_line=(Real*) line->data; 
			for (i=0; i < signal->size; i++) {
				t_real=real_sig[i]; 
				j=(i+(*offset)) % line->size; 
				real_sig[i]=real_line[j]; 
				real_line[j]=t_real; 
			}
			break; 
		case COMPLEX: 	
			cpx_sig=(Complex*) signal->data; 
			cpx_line=(Complex*) line->data; 
			for (i=0; i < signal->size; i++) {
				t_real=cpx_sig[i].real; 
				t_imag=cpx_sig[i].imag; 
				j=(i+(*offset)) % line->size; 
				cpx_sig[i].real=cpx_line[j].real; 
				cpx_sig[i].imag=cpx_line[j].imag; 
				cpx_line[j].real=t_real; 
				cpx_line[j].imag=t_imag; 
			}
			break; 
		default: 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
			
	}		
	*offset=(i+(*offset)) % line->size; 

	return ERR_OK; 
}	

#if 0
/*
Hilbert transformation of "a"
WAVE: zero-padded to power of 2
COMPLEX: simple transform
POLAR/SURFACE: not available
*/

int esweep_hilbert(esweep_object *obj, esweep_object *table) {
	Complex *complex, *fft_table;
	int fft_size;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	fft_size=(int) (pow(2, ceil(log(obj->size)/log(2)))+0.5);

	switch (obj->type) {
		case WAVE:
			ESWEEP_MALLOC(complex, fft_size, sizeof(Complex), ERR_MALLOC);
			r2c(complex, (Wave*) obj->data, obj->size);
			free(obj->data);
			obj->size=fft_size;
			obj->type=COMPLEX;
			obj->data=complex;
			break;
		case COMPLEX:
			complex=(Complex*) obj->data;
			fft_size=obj->size;
			break;
		case POLAR:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	if ((table==NULL) || (table->size!=fft_size/2) || (table->data==NULL) || (table->type!=COMPLEX)) fft_table=fft_create_table(obj->size);
	else fft_table=(Complex*) table->data;

	dsp_hilbert(complex, fft_table, obj->size);

	if ((table==NULL) || (table->size!=fft_size/2) || (table->data==NULL) || (table->type!=COMPLEX)) free(fft_table);

	return ERR_OK;
}

int esweep_analytic(esweep_object *obj, esweep_object *table) {
	Complex *complex, *fft_table, *analytic;
	int fft_size, i;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case WAVE:
			fft_size=(int) (pow(2, ceil(log(obj->size)/log(2)))+0.5);
			ESWEEP_MALLOC(analytic, fft_size, sizeof(Complex), ERR_MALLOC);
			r2c(analytic, (Wave*) obj->data, obj->size);
			free(obj->data);
			obj->size=fft_size;
			obj->type=COMPLEX;
			obj->data=analytic;
			break;
		case COMPLEX:
		case POLAR:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	if ((table==NULL) || (table->size!=fft_size/2) || (table->data==NULL) || (table->type!=COMPLEX)) fft_table=fft_create_table(obj->size);
	else fft_table=(Complex*) table->data;

	ESWEEP_MALLOC(complex, fft_size, sizeof(Complex), ERR_MALLOC);
	memcpy(complex, analytic, fft_size);

	dsp_hilbert(complex, fft_table, fft_size);

	/* the real part of complex is the imaginary part of the analytic signal */
	for (i=0;i<fft_size;i++) {
		analytic[i].imag=complex[i].real;
	}
	free(complex);

	if ((table==NULL) || (table->size!=fft_size/2) || (table->data==NULL) || (table->type!=COMPLEX)) free(fft_table);

	return ERR_OK;
}
#endif

/* time integration implemented as an IIR-Filter

WAVE: simple accumulation
POLAR/COMPLEX: division by 1/(2*pi*f); DC is set to the value at df
SURFACE: not available

This integrator has a gain of 1/Fs
*/

int esweep_integrate(esweep_object *obj) {
	Wave *wave;
	Complex *complex;
	Polar *polar;
	int i;
	int size;
	Real dw;
	Real temp=0.0;
	Real pi_2=M_PI/2;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	dw=2*M_PI*obj->samplerate/obj->size; /* angular frequency */

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			wave[0]/=obj->samplerate;
			for (i=1;i<obj->size;i++) wave[i]=wave[i]/obj->samplerate+wave[i-1];
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			size=obj->size/2+1; /* hermitian redundancy */
			polar[0].abs/=dw;
			polar[0].arg+=pi_2;
			for (i=1;i<size;i++) {
				polar[i].abs/=(i*dw);
				temp=polar[i].arg+=pi_2; /* +90° */
				/* restore redundancy */
				polar[obj->size-i].abs=polar[i].abs;
				polar[obj->size-i].arg=-polar[i].arg;
			}
			polar[size-1].arg=temp;
			break;
		case COMPLEX:
			complex=(Complex*) obj->data;
			size=obj->size/2; /* hermitian redundancy */
			temp=complex[0].real/dw;
			complex[0].real=-complex[0].imag/dw;
			complex[0].imag=temp;
			temp=complex[size].real/dw;
			complex[size].real=-complex[size].imag/dw;
			complex[size].imag=temp;
			for (i=1;i<size;i++) {
				temp=complex[i].real/(i*dw);
				complex[i].real=-complex[i].imag/(i*dw);
				complex[i].imag=temp;
				complex[obj->size-i].real=complex[i].real;
				complex[obj->size-i].imag=-complex[i].imag;
			}
			break;
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}

/* time differentiation

WAVE: simple differentiator; first sample is extrapolated from the slope at the second sample
POLAR/COMPLEX: multiplication with 2*pi*f
SURFACE: not available

This differentiator has a gain of Fs
*/

int esweep_differentiate(esweep_object *obj) { /* d/dt */
	Wave *wave;
	Complex *complex;
	Polar *polar;
	int i;
	int size;
	Real dw;
	Real temp=0.0;
	Real pi_2=M_PI/2;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	dw=2*M_PI*obj->samplerate/obj->size; /* angular frequency */

	switch (obj->type) {
		case WAVE:
			wave=(Wave*) obj->data;
			for (i=obj->size-1; i > 0; i--) {
				wave[i]=obj->samplerate*(wave[i]-wave[i-1]);
			}
			wave[0]*=obj->samplerate; 	
			break;
		case POLAR:
			polar=(Polar*) obj->data;
			size=obj->size/2+1; /* hermitian redundancy */

			polar[0].abs=0;
			polar[0].arg=0;
			for (i=1;i<size;i++) {
				polar[i].abs*=i*dw;
				temp=polar[i].arg-=pi_2;
				/* restore redundancy */
				polar[obj->size-i].abs=polar[i].abs;
				polar[obj->size-i].arg=-polar[i].arg;
			}
			polar[size-1].arg=temp;
			break;
		case COMPLEX:
			complex=(Complex*) obj->data;
			size=obj->size/2; /* hermitian redundancy */
			complex[0].real=0;
			complex[0].imag=0;
			temp=-complex[size].real*size*dw;
			complex[size].real=complex[size].imag*size*dw;
			complex[size].imag=temp;
			for (i=1;i<size;i++) {
				temp=-complex[i].real*i*dw;
				complex[i].real=complex[i].imag*i*dw;
				complex[i].imag=temp;
				/* restore redundancy */
				complex[obj->size-i].real=complex[i].real;
				complex[obj->size-i].imag=-complex[i].imag;
			}
			break;
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}

/*
wraps the argument of "a" so it is in the range [-pi:+pi]
only POLAR
*/

int esweep_wrapPhase(esweep_object *obj) {
	Polar *polar;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			dsp_wrapPhase(polar, obj->size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}

/*
unwraps the phase of "a"
only POLAR
*/

int esweep_unwrapPhase(esweep_object *obj) {
	Polar *polar;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			dsp_unwrapPhase(polar, obj->size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}
#if 0
/*
calculate the group delay of "a"
only POLAR
*/

int esweep_groupDelay(esweep_object *obj) {
	Polar *polar;
	Wave *wave;
	Complex *complex, *fft_table, *wave_diff;
	int i, size;
	Real dw;
	Real tmp;
	Real t_real, t_imag, t_abs;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			/* differentiate phase response */
			dw=2*M_PI*obj->samplerate/obj->size;
			/* the group delay at Fs */
			tmp=polar[0].arg-polar[obj->size-1].arg;
			if (polar[obj->size-1].arg>M_PI) tmp-=2*M_PI;
			else if (polar[obj->size-1].arg<-M_PI) tmp+=2*M_PI;
			tmp/=-dw;
			/* unwrap phase */
			dsp_unwrapPhase(polar, obj->size);
			for (i=0;i<obj->size-1;i++) {
				polar[i].arg=-(polar[i+1].arg-polar[i].arg)/dw;
			}
			polar[obj->size-1].arg=tmp;
			break;
		case WAVE:
			wave=(Wave*) obj->data;
			size=(int) (pow(2, ceil(log(obj->size)/log(2)))+0.5);

			ESWEEP_MALLOC(complex, size, sizeof(Complex), ERR_MALLOC);
			ESWEEP_MALLOC(wave_diff, size, sizeof(Complex), ERR_MALLOC);

			r2c(complex, wave, obj->size);
			for (i=0;i<obj->size;i++) wave_diff[i].real=i*wave[i]/obj->samplerate;

			fft_table=fft_create_table(size);
			fft(complex, fft_table, size, FFT_FORWARD);
			fft(wave_diff, fft_table, size, FFT_FORWARD);
			free(fft_table);

			/* complex division */
			for (i=0;i<size;i++) {
				t_real=wave_diff[i].real;
				t_imag=wave_diff[i].imag;

				t_abs=complex[i].real*complex[i].real+complex[i].imag*complex[i].imag;

				wave_diff[i].real=(t_real*complex[i].real+t_imag*complex[i].imag)/t_abs;
				wave_diff[i].imag=(t_imag*complex[i].real-t_real*complex[i].imag)/t_abs;
			}
			/* the group delay is the real part of wave_diff, but we need the amplitude calculated from complex */
			polar=(Polar*) complex;
			c2p(polar, complex, size);
			/* copy the real part of wave_diff into the argument of polar */
			for (i=0;i<size; i++) polar[i].arg=wave_diff[i].real;

			free(wave_diff);
			free(wave);

			obj->data=polar;
			obj->size=size;
			obj->type=POLAR;
			break;
		case COMPLEX:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}

/*
convert group delay to phase
only POLAR
*/

int esweep_gDelayToPhase(esweep_object *obj) {
	Polar *polar;
	int i;
	Real dw;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			/* integrate phase response */
			dw=2*M_PI*obj->samplerate/obj->size;
			polar[0].arg*=dw;
			for (i=1;i<obj->size;i++) {
				polar[i].arg=polar[i-1].arg-polar[i].arg*dw;
			}
			/* wrap phase */
			dsp_wrapPhase(polar, obj->size);
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			return ERR_NOT_ON_THIS_TYPE;
	}

	return ERR_OK;
}

#endif
/*
smooth magnitude and argument of "a" in the range Octave/factor
only POLAR
*/

int esweep_smooth(esweep_object *obj, Real factor) {
	Polar *polar, *tmp;
	int herm_size; 
	int range; 
	int i, j, k; 

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(factor >= 1, ERR_BAD_ARGUMENT);

	switch (obj->type) {
		case POLAR:
			polar=(Polar*) obj->data;
			herm_size=obj->size/2+1; 
			/* unwrap phase */
			dsp_unwrapPhase(polar, obj->size);
			tmp=(Polar*) calloc(obj->size, sizeof(Polar));
			/* do not touch the nyquist frequency */
			for (i=1;i<herm_size;i++) {
				j=(int) (i-i/(4*factor)+0.5);
				j=j < 0 ? 0 : j; 
				range=(int) (i+i/(2*factor)+0.5); 
				range=range >= herm_size ? herm_size-1 : range; 
				tmp[i].abs=tmp[i].arg=0.0;
				for (k=0;j <= range;j++, k++) {
					tmp[i].abs+=polar[j].abs;
					tmp[i].arg+=polar[j].arg;
				}
				tmp[obj->size-i].abs=tmp[i].abs/=k;
				tmp[obj->size-i].arg=-(tmp[i].arg/=k);
			}
			/* wrap phase */
			dsp_wrapPhase(tmp, obj->size);
			
			free(obj->data);
			obj->data=(void*) tmp; 
			break;
		case COMPLEX:
		case WAVE:
		case SURFACE:
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
	}

	return ERR_OK;
}

/*
applies a window to a WAVE or a COMPLEX
possible window types are:

- Rectangle
- von Hann ("hanning")
- Hamming
- Bartlett (triangle)
- Blackman

both left and right may be different, width in percent
*/


int esweep_window(esweep_object *obj, const char *left_win, Real left_width, const char *right_win, Real right_width) {
	int win_type;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	ESWEEP_ASSERT(left_width >= 0.0 && left_width <= 100.0, ERR_BAD_ARGUMENT);
        ESWEEP_ASSERT(right_width >= 0.0 && right_width <= 100.0, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(left_width + right_width <= 100.0, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(left_win != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(right_win != NULL, ERR_BAD_ARGUMENT); 

	/* left window */

	if (strlen(left_win)>0) {
		/* set window type */

		win_type=WIN_NOWIN;
		if (strcmp("rect", left_win)==0) { win_type=WIN_RECT; }
		if (strcmp("bartlett", left_win)==0) { win_type=WIN_BART; }
		if (strcmp("hann", left_win)==0) { win_type=WIN_HANN; }
		if (strcmp("hamming", left_win)==0) { win_type=WIN_HAMM; }
		if (strcmp("blackman", left_win)==0) { win_type=WIN_BLACK; }

		/* apply window */

		switch (obj->type) {
			case WAVE: 
				window((Wave*) obj->data, 0, (int) (obj->size*left_width/100+0.5), WIN_LEFT, win_type);
				break; 
			case COMPLEX: 
				window_complex((Complex*) obj->data, 0, (int) (obj->size*left_width/100+0.5), WIN_LEFT, win_type);
				break; 
			default: 
				ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
		}
	}

	/* right window */

	if (strlen(right_win)>0) {
		/* set window type */

		win_type=WIN_NOWIN;
		if (strcmp("rect", right_win)==0) { win_type=WIN_RECT; }
		if (strcmp("bartlett", right_win)==0) { win_type=WIN_BART; }
		if (strcmp("hann", right_win)==0) { win_type=WIN_HANN; }
		if (strcmp("hamming", right_win)==0) { win_type=WIN_HAMM; }
		if (strcmp("blackman", right_win)==0) { win_type=WIN_BLACK; }

		/* apply window */

		switch (obj->type) {
			case WAVE: 
				window((Wave*) obj->data, (int) (obj->size-obj->size*right_width/100+0.5), obj->size, WIN_RIGHT, win_type);
				break; 
			case COMPLEX: 
				window_complex((Complex*) obj->data, (int) (obj->size-obj->size*right_width/100+0.5), obj->size, WIN_RIGHT, win_type);
				break; 
			default: 
				ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
		}
	}

	return ERR_OK;
}

int esweep_restoreHermitian(esweep_object *obj) {
	Polar *polar; 
	Complex *cpx; 
	int new_size, i; 

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);

	new_size=2*obj->size; 
	switch (obj->type) {
		case POLAR: 
			/* create new object */
			ESWEEP_MALLOC(polar, new_size, sizeof(Polar), ERR_MALLOC);
			/* copy data */
			memcpy(polar, obj->data, obj->size*sizeof(Polar)); 
			/* set Fs/2 to a pure real number */
			polar[obj->size].abs=1e-10; polar[obj->size].arg=0.0; 
			/* restore redundancy */
			for (i=1; i < obj->size; i++) {
				polar[new_size-i].abs=polar[i].abs;
				polar[new_size-i].arg=-polar[i].arg;
			}
			obj->size=new_size; 
			free(obj->data); 
			obj->data=polar; 
			break; 
		case COMPLEX:  
			/* create new object */
			ESWEEP_MALLOC(cpx, new_size, sizeof(Complex), ERR_MALLOC);
			/* copy data */
			memcpy(cpx, obj->data, obj->size*sizeof(Complex)); 
			/* set Fs/2 to a pure real number */
			cpx[obj->size].real=1e-10; cpx[obj->size].imag=0.0; 
			/* restore redundancy */
			for (i=1; i < obj->size; i++) {
				cpx[new_size-i].real=cpx[i].real;
				cpx[new_size-i].imag=-cpx[i].imag;
			}
			obj->size=new_size; 
			free(obj->data); 
			obj->data=cpx; 
			break; 
		default: 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
	}

	return ERR_OK; 
}

int esweep_peakDetect(esweep_object *obj, Real threshold, int *peaks, int *n) {
	Real *real; 
	Polar *polar; 
	int i, k, m; 

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_ASSERT(n != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(*n > 1, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(peaks != NULL, ERR_BAD_ARGUMENT); 
	ESWEEP_ASSERT(threshold != 0, ERR_BAD_ARGUMENT); 

	m=0; 
	switch (obj->type) {
		case WAVE: 
			real=(Real*) obj->data; 
			if (threshold > 0) {
				for (i=1; i < obj->size; i++) {
					/* as long as the function is rising, follow it */
					if (real[i] > real[i-1]) continue; 
					/* if not, check if it drops below the threshold */
					for (k=i; k < obj->size; k++) {
						if (real[k] > real[i-1]) break; // obviously not
						if (real[i-1] - real[k] > threshold) {
							peaks[m]=i-1; 
							m++;
							if (m == *n) return ERR_OK; //array is full
							/* now find the bottom of the hole */
							for (i=k+1; i < obj->size; i++) {
								if (real[i] > real[i-1]) break; 
							}
							break; 
						}
					}
				}
			} else {
				for (i=1; i < obj->size; i++) {
					/* as long as the function is falling, follow it */
					if (real[i] < real[i-1]) continue; 
					/* if not, check if it rises above the threshold */
					for (k=i; k < obj->size; k++) {
						if (real[k] < real[i-1]) break; // obviously not
						if (real[i-1] - real[k] < threshold) {
							peaks[m]=i-1; 
							m++;
							if (m == *n) return ERR_OK; //array is full
							/* now find the next minimum */
							for (i=k+1; i < obj->size; i++) {
								if (real[i] < real[i-1]) break; 
							}
							break; 
						}
					}
				}
			}
			break; 
		case POLAR: 
			polar=(Polar*) obj->data; 
			if (threshold > 0) {
				for (i=1; i < obj->size; i++) {
					/* as long as the function is rising, follow it */
					if (polar[i].abs > polar[i-1].abs) continue; 
					/* if not, check if it drops below the threshold */
					for (k=i; k < obj->size; k++) {
						if (polar[k].abs > polar[i-1].abs) break; // obviously not
						if (polar[i-1].abs - polar[k].abs > threshold) {
							peaks[m]=i-1; 
							m++;
							if (m == *n) return ERR_OK; //array is full
							/* now find the bottom of the hole */
							for (i=k+1; i < obj->size; i++) {
								if (polar[i].abs > polar[i-1].abs) break; 
							}
							break; 
						}
					}
				}
			} else {
				for (i=1; i < obj->size; i++) {
					/* as long as the function is falling, follow it */
					if (polar[i].abs < polar[i-1].abs) continue; 
					/* if not, check if it drops below the threshold */
					for (k=i; k < obj->size; k++) {
						if (polar[k].abs < polar[i-1].abs) break; // obviously not
						if (polar[i-1].abs - polar[k].abs < threshold) {
							peaks[m]=i-1; 
							m++;
							if (m == *n) return ERR_OK; //array is full
							/* now find the next maximum */
							for (i=k+1; i < obj->size; i++) {
								if (polar[i].abs < polar[i-1].abs) break; 
							}
							break; 
						}
					}
				}
			}
			break; 
		default: 
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE); 
	}
	*n=m; 

	return ERR_OK; 
}
