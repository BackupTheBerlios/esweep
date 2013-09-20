/*
 * Copyright (c) 2011 Jochen Fabricius <jfab@berlios.de>
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

#include "esweep_priv.h"

#include "dsp.h"
#include "fft.h"

/*
 * src/esweep_filter.c:
 * Creating and applying filters
 * 12.11.2011, jfab: initial creation
*/

/* check for a valid filter structure */
#define ESWEEP_FILTER_CHECK(filter, ret_empty, ret_map, ret_size, ret_type) \
		ESWEEP_ASSERT(filter != NULL, ret_empty); \
		ESWEEP_OBJ_NOTEMPTY(filter[0], ret_empty); \
		ESWEEP_OBJ_NOTEMPTY(filter[2], ret_empty); \
		ESWEEP_SAME_MAPPING(filter[0], filter[2], ret_map);  \
		ESWEEP_ASSERT(filter[0]->size == filter[2]->size, ret_size);  \
		ESWEEP_ASSERT(filter[0]->type == WAVE, ret_type); \
		ESWEEP_ASSERT(filter[2]->type == COMPLEX, ret_type); \
		if (filter[1] != NULL) { \
			ESWEEP_ASSERT(filter[0]->size % 3 == 0, ret_size); \
			ESWEEP_OBJ_NOTEMPTY(filter[1], ret_empty); \
			ESWEEP_SAME_MAPPING(filter[0], filter[1], ret_map);  \
			ESWEEP_ASSERT(filter[0]->size == filter[1]->size, ret_size);  \
			ESWEEP_ASSERT(filter[1]->type == WAVE, ret_type); \
		}


/* simple filter definitions */
#define ESWEEP_FILTER_IIR 0
#define ESWEEP_FILTER_FIR 1

/*
 * With IIR filters, use a cascaded TF2 scheme. The parameter filter must hold 3 objects:
 * numerator. denominator, and filter state. All must have the same size.
 * If the denominator is NULL than it is a FIR filter .
 * The input object can be WAVE or COMPLEX, the filter objects must be WAVE.
 * FIR filters use the normal form 1. This has two advantages over the above mentioned biquad structure:
 * 	- you can simply use an existing impulse response without doing a complicated transform
 * 	- some calculations may be avoided during interpolation and decimation (only a buffer shift is necessary)
 * Be sure not to convert an FIR filter into an IIR filter without transformation to a TF2 biquad!
 */


static inline void __esweep_filter_fir(esweep_object *in, esweep_object *filter[]);
static inline void __esweep_filter_iir(esweep_object *in, esweep_object *filter[]);

int esweep_filter(esweep_object *obj, esweep_object *filter[]) {
	int filter_type=ESWEEP_FILTER_IIR;

	ESWEEP_OBJ_NOTEMPTY(obj, ERR_EMPTY_OBJECT);
	ESWEEP_FILTER_CHECK(filter, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);

	if (filter[1]==NULL) filter_type=ESWEEP_FILTER_FIR;

	ESWEEP_SAME_MAPPING(obj, filter[0], ERR_DIFF_MAPPING);

	switch (obj->type) {
		case WAVE:
		case COMPLEX:
			break;
		case POLAR: /* Fallthrough */
		case SURFACE: /* Fallthrough */
		default:
			ESWEEP_NOT_THIS_TYPE(obj->type, ERR_NOT_ON_THIS_TYPE);
	}

	if (filter_type == ESWEEP_FILTER_FIR) {
		__esweep_filter_fir(obj, filter);
	} else {
		__esweep_filter_iir(obj, filter);
	}

	return ERR_OK;
}

static inline void interpolate(esweep_object *out, esweep_object *in, Complex *carry);
static inline void decimate(esweep_object *out, esweep_object *in);

int esweep_resample(esweep_object *out, esweep_object *in, esweep_object *filter[], Complex *carry) {
	int filter_type=ESWEEP_FILTER_IIR;

	int samplesize;

	ESWEEP_OBJ_NOTEMPTY(out, ERR_EMPTY_OBJECT);
	ESWEEP_OBJ_NOTEMPTY(in, ERR_EMPTY_OBJECT);
	ESWEEP_FILTER_CHECK(filter, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);
	if (filter[1]==NULL) filter_type=ESWEEP_FILTER_FIR;

	/* both objects must have the same type */
	ESWEEP_ASSERT(out->type == in->type, ERR_DIFF_TYPES);
	switch (out->type) {
		case WAVE:
			samplesize=sizeof(Real);
			break;
		case COMPLEX:
			samplesize=sizeof(Complex);
			break;
		case POLAR: /* Fallthrough */
		case SURFACE: /* Fallthrough */
		default:
			ESWEEP_NOT_THIS_TYPE(out->type, ERR_NOT_ON_THIS_TYPE);
	}

	/* samplerates are identical, simply copy in to out */
	if (out->samplerate == in->samplerate) {
		ESWEEP_ASSERT(out->size == in->size, ERR_SIZE_MISMATCH);
		memcpy(out->data, in->data, samplesize*in->size);
		return ERR_OK;
	}

	/* the samplerates and sizes of each object must be integer multiples of each other */
	ESWEEP_ASSERT(in->samplerate % in->size == 0 || in->size % in->samplerate == 0, ERR_SIZE_MISMATCH);
	ESWEEP_ASSERT(out->samplerate % out->size == 0 || out->size % out->samplerate == 0, ERR_SIZE_MISMATCH);

	/* Of course, the ratio between samplerate and size must be equal */
	if (in->samplerate >= in->size) {
		if (out->samplerate >= out->size) {
			ESWEEP_ASSERT(in->samplerate/in->size == out->samplerate/out->size, ERR_SIZE_MISMATCH);
		} else {
			ESWEEP_ASSERT(in->size/in->samplerate == out->size/out->samplerate, ERR_SIZE_MISMATCH);
		}
	} else {
		if (out->samplerate >= out->size) {
			ESWEEP_ASSERT(in->samplerate/in->size == out->samplerate/out->size, ERR_SIZE_MISMATCH);
		} else {
			ESWEEP_ASSERT(in->size/in->samplerate == out->size/out->samplerate, ERR_SIZE_MISMATCH);
		}
	}

	if (out->samplerate > in->samplerate) {
		/* On upsampling, we first linear interpolate in to out
		 * and then lowpass filter the output */
		interpolate(out, in, carry);
		if (filter_type == ESWEEP_FILTER_FIR) {
			__esweep_filter_fir(out, filter);
		} else {
			__esweep_filter_iir(out, filter);
		}
	} else {
		/* On downsampling, we must first filter the input and then decimate.
		 * Note that the input signal is modified by filtering it. This is definitely a drawback
		 * and will hopefully be avoided in the future. */
		if (filter_type == ESWEEP_FILTER_FIR) {
			__esweep_filter_fir(in, filter);
		} else {
			__esweep_filter_iir(in, filter);
		}
		decimate(out, in);
	}

	return ERR_OK;
}

/* copy, zero-stuff and filter in to out */
static inline void interpolate(esweep_object *out, esweep_object *in, Complex *carry) {
	Real Tin=1.0/in->samplerate, Tout=1.0/out->samplerate; // sampling periods
	Real tin, tout; // time running
	Complex *cpx_in, *cpx_out;
	Real *real_in, *real_out;
	Real eta; // interpolation factor

	int i, j;

	switch (in->type) {
		case WAVE:
			real_in=(Real*) in->data;
			real_out=(Real*) out->data;
			/* interpolate the first samples with the carry */
			for (i=0, tout=-Tin; tout < 0.0; i++, tout+=Tout) {
				eta=(tout+Tin)/Tin;
				real_out[i]=carry->real+eta*(real_in[0]-carry->real);
			}
			for (j=1, tin=Tin; j < in->size; j++, tin+=Tin) {
				for (; tin > tout; i++, tout+=Tout) {
					eta=(tout-(tin-Tin))/Tin;
					real_out[i]=real_in[j-1]+eta*(real_in[j]-real_in[j-1]);
				}
			}
			/* reset the carry */
			carry->real=real_in[in->size-1];
			break;
		case COMPLEX:
			cpx_in=(Complex*) in->data;
			cpx_out=(Complex*) out->data;
			/* interpolate the first samples with the carry */
			for (i=0, tout=-Tin; tout < 0.0; i++, tout+=Tout) {
				eta=(tout+Tin)/Tin;
				cpx_out[i].real=carry->real+eta*(cpx_in[0].imag-carry->real);
				cpx_out[i].imag=carry->imag+eta*(cpx_in[0].imag-carry->imag);
			}
			for (j=1, tin=Tin; j < in->size; j++, tin+=Tin) {
				for (; tin > tout; i++, tout+=Tout) {
					eta=(tout-(tin-Tin))/Tin;
					cpx_out[i].real=cpx_in[j-1].real+eta*(cpx_in[j].real-cpx_in[j-1].real);
					cpx_out[i].imag=cpx_in[j-1].imag+eta*(cpx_in[j].imag-cpx_in[j-1].imag);
				}
			}
			/* reset the carry */
			carry->real=cpx_in[in->size-1].real;
			carry->imag=cpx_in[in->size-1].imag;
			break;
		default:
			break;
	}
}

/* copy, decimate and filter in to out */
static inline void decimate(esweep_object *out, esweep_object *in) {
}

static inline void __esweep_filter_iir(esweep_object *in, esweep_object *filter[]) {
	int i, j;
	Wave *wave, tmp;
	Real *num=(filter[0])->data, *denom=(filter[1])->data;
	Complex *state=(filter[2])->data;
	Complex *cpx;
	Real t_real, t_imag;

	switch (in->type) {
		case WAVE:
			wave=in->data;
			for (j=0; j < filter[0]->size; j+=3) {
				for (i=0; i < in->size; i++) {
					tmp=wave[i]*num[j]+state[j].real;

					state[j].real=num[j+1]*wave[i]-denom[j+1]*tmp+state[j+1].real;
					state[j+1].real=num[j+2]*wave[i]-denom[j+2]*tmp;

					wave[i]=tmp;
				}
			}
			break;
		case COMPLEX:
			cpx=in->data;
			for (j=0; j < filter[0]->size; j+=3) {
				for (i=0; i < in->size; i++) {
					t_real=cpx[i].real*num[j]+state[j].real;
					t_imag=cpx[i].imag*num[j]+state[j].imag;

					state[j].real=num[j+1]*cpx[i].real-denom[j+1]*t_real+state[j+1].real;
					state[j].imag=num[j+1]*cpx[i].imag-denom[j+1]*t_imag+state[j+1].imag;
					state[j+1].real=num[j+2]*cpx[i].real-denom[j+2]*t_real;
					state[j+1].imag=num[j+2]*cpx[i].imag-denom[j+2]*t_imag;
					cpx[i].real=t_real;
					cpx[i].imag=t_imag;
				}
			}
			break;
		default:
			break;
	}
}

static inline void __esweep_filter_fir(esweep_object *in, esweep_object *filter[]) {
	int i, j;
	Wave *wave;
	Real *num=filter[0]->data;
	Complex *state=filter[2]->data;
	Complex *cpx;
	Complex tmp[2];

	switch (in->type) {
		case WAVE:
			wave=in->data;
			for (i=0; i < in->size; i++) {
				state[0].real=wave[i];
				wave[i]*=num[0];
				tmp[0].real=state[0].real;
				for (j=1; j < filter[0]->size; j++) {
					wave[i]+=state[j].real*num[j];
					tmp[j % 2].real=state[j].real;
					state[j].real=tmp[(j-1) % 2].real;
				}
			}
			break;
		case COMPLEX:
			cpx=in->data;
			for (i=0; i < in->size; i++) {
				state[0].real=cpx[i].real;
				state[0].imag=cpx[i].imag;
				cpx[i].real*=num[0];
				cpx[i].imag*=num[0];
				tmp[0].real=state[0].real;
				tmp[0].imag=state[0].imag;
				for (j=1; j < filter[0]->size; j++) {
					cpx[i].real+=state[j].real*num[j];
					cpx[i].imag+=state[j].imag*num[j];
					tmp[j % 2].real=state[j].real;
					tmp[j % 2].imag=state[j].imag;
					state[j].real=tmp[(j-1) % 2].real;
					state[j].imag=tmp[(j-1) % 2].imag;
				}
			}
			break;
		default:
			break;
	}
}

/*
 * create a single 2nd order IIR filter from a standard analog filter definition.
 * The result is in the esweep_object array filter, the numerator in filter[0], the denominator in filter[1].
 *
 * The filter structure is as follows:
 * the first sample of the numerator is always the total gain. It will be recomputed every time a
 * further filter is appended. The first sample of the denominator is always 1, and thus ignored in the
 * filter algorithm. The second and third sample are the usual coefficients.
 *
 * It is important to use esweep_appendFilter() instead of esweep_concat() to retain these structure.
 * Otherwise it may lead to unexpected results.
 *
 * If other filters are necessary, use esweep_createFilterPZ(), where you can define single poles and zeroes (still to come).
 */

#define ESWEEP_FILTER_UNKNOWN -1
#define ESWEEP_FILTER_GAIN 0
#define ESWEEP_FILTER_LOWPASS 1
#define ESWEEP_FILTER_HIGHPASS 2
#define ESWEEP_FILTER_BANDPASS 3
#define ESWEEP_FILTER_BANDSTOP 4
#define ESWEEP_FILTER_ALLPASS 5
#define ESWEEP_FILTER_NOTCH 6
#define ESWEEP_FILTER_SHELVE 7
#define ESWEEP_FILTER_LINKWITZ 8
#define ESWEEP_FILTER_INTEGRATOR 9
#define ESWEEP_FILTER_DIFFERENTIATOR 10
#define ESWEEP_FILTER_CUSTOM 128

/*
 * Check the filter parameters.
 * For safety reasons, the frequency must be greater DC and smaller than Nyquist/2.
 * This prevents instabilities of the filter (not all).
 * This filter generator only produces "stable" filter, i. e. no positive poles.
 */

#define ESWEEP_CHECK_Q(Q, f, samplerate) \
	ESWEEP_ASSERT(Q > 0, NULL); \
	ESWEEP_ASSERT(f > 0, NULL); \
	ESWEEP_ASSERT(f < samplerate/2, NULL);

esweep_object** esweep_createFilterFromCoeff(const char *type, Real gain, Real Qp, Real Fp, Real Qz, Real Fz, int samplerate) {
	Real *numZ, *denomZ; // the coefficients in the z-domain
	Real num[3], denom[3]; // in the s-domain
	Real Kp, Kz; // bilinear transformation factors, including frequency pre-warp
	int filter_type=ESWEEP_FILTER_UNKNOWN;

	esweep_object **filter;

	ESWEEP_ASSERT(type != NULL, NULL);
	ESWEEP_ASSERT(strlen(type) > 0, NULL);
	ESWEEP_ASSERT(samplerate > 0, NULL);

	/* Get the filter type */
	if (strcmp(type, "lowpass") == 0) filter_type=ESWEEP_FILTER_LOWPASS;
	else if (strcmp(type, "highpass") == 0) filter_type=ESWEEP_FILTER_HIGHPASS;
	else if (strcmp(type, "bandpass") == 0) filter_type=ESWEEP_FILTER_BANDPASS;
	else if (strcmp(type, "bandstop") == 0) filter_type=ESWEEP_FILTER_BANDSTOP;
	else if (strcmp(type, "allpass") == 0) filter_type=ESWEEP_FILTER_ALLPASS;
	else if (strcmp(type, "notch") == 0) filter_type=ESWEEP_FILTER_NOTCH;
	else if (strcmp(type, "shelve") == 0) filter_type=ESWEEP_FILTER_SHELVE;
	else if (strcmp(type, "linkwitz") == 0) filter_type=ESWEEP_FILTER_LINKWITZ;
	else if (strcmp(type, "integrator") == 0) filter_type=ESWEEP_FILTER_INTEGRATOR;
	else if (strcmp(type, "differentiator") == 0) filter_type=ESWEEP_FILTER_DIFFERENTIATOR;
	else if (strcmp(type, "gain") == 0) filter_type=ESWEEP_FILTER_GAIN;

	ESWEEP_ASSERT(filter_type >= 0, NULL);

	/* frequency warping factors for the bilinear transform */
	Kp=tan(M_PI*Fp/samplerate);
	Kz=Kp; // default, same frequency for numerator and denominator

	/* Create the filter coefficients in the analog domain */
	switch (filter_type) {
		case ESWEEP_FILTER_LOWPASS:
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			num[0]=1.0;
			num[1]=0.0;
			num[2]=0.0;

			denom[0]=1.0;
			denom[1]= 1.0/Qp;
			denom[2]= 1.0;
			break;
		case ESWEEP_FILTER_HIGHPASS:
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			num[0]=0.0;
			num[1]=0.0;
			num[2]=1.0;

			denom[0]=1.0;
			denom[1]=1.0/Qp;
			denom[2]=1.0;
			break;
		case ESWEEP_FILTER_BANDPASS:
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			num[0]=0.0;
			num[1]=1.0;
			num[2]=0.0;

			denom[0]=1.0;
			denom[1]=1.0/Qp;
			denom[2]=1.0;
			break;
		case ESWEEP_FILTER_BANDSTOP:
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			num[0]=1.0;
			num[1]=0.0;
			num[2]=1.0;

			denom[0]=1.0;
			denom[1]=1.0/Qp;
			denom[2]=1.0;
			break;
		case ESWEEP_FILTER_ALLPASS:
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			num[0]=denom[0]=1.0;

			denom[1]=1.0/Qp;
			num[1]=-denom[1];

			num[2]=denom[2]=1.0;
			break;
		case ESWEEP_FILTER_NOTCH: // a notch filter is the same as a bandstop, but with a complex zero at Fz
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			ESWEEP_ASSERT(Qz > 0, NULL);
			num[0]=1.0;
			num[1]=1.0/Qz;
			num[2]=1.0;

			denom[0]=1.0;
			denom[1]=1.0/Qp;
			denom[2]=1.0;
			break;
		case ESWEEP_FILTER_SHELVE: // a single pole shelving filter; if Fp > Fz then highpass, else lowpass
			ESWEEP_CHECK_Q(1, Fp, samplerate); // the Q is not necessary here
			ESWEEP_CHECK_Q(1, Fz, samplerate); // the Q is not necessary here

			num[0]=1.0;
			num[1]=1.0;
			num[2]=0.0;

			denom[0]=1.0;
			denom[1]=1.0;
			denom[2]=0.0;
			Kz=tan(M_PI*Fz/samplerate);

      // the following calculations result in a total gain of Fz/Fp
      // compensate for it
      gain*=Fp/Fz;

			break;
		case ESWEEP_FILTER_LINKWITZ: // Linkwitz transform; Fz > Fp then lowpass, else highpass
			ESWEEP_CHECK_Q(Qp, Fp, samplerate);
			ESWEEP_CHECK_Q(Qz, Fz, samplerate);

			num[0]=1.0;
			num[1]=1.0/Qz;
			num[2]=1.0;

			denom[0]=1.0;
			denom[1]=1.0/Qp;
			denom[2]=1.0;
			Kz=tan(M_PI*Fz/samplerate);
			break;
		case ESWEEP_FILTER_INTEGRATOR: // integrator filter, but also usable as a single order lowpass
			ESWEEP_CHECK_Q(1, Fp, samplerate);
			num[0]=1.0;
			num[1]=0.0;
			num[2]=0.0;

			denom[0]=1.0;
			denom[1]= 1.0;
			denom[2]= 0.0;
			break;
		case ESWEEP_FILTER_DIFFERENTIATOR: // differentiator filter, but also usable as a single order highpass
			ESWEEP_CHECK_Q(1, Fp, samplerate);
			num[0]=0.0;
			num[1]=1.0;
			num[2]=0.0;

			denom[0]=1.0;
			denom[1]= 1.0;
			denom[2]= 0.0;
			break;
		case ESWEEP_FILTER_GAIN:
			num[0]=1.0; // apply gain below
			denom[0]=1.0;
			num[1]=denom[1]=0.0;
			num[2]=denom[2]=0.0;
			break;
	}
	/* Apply gain */
	num[0]*=gain;
	num[1]*=gain;
	num[2]*=gain;

	/* cleanup and create new filter objects */
	ESWEEP_MALLOC(filter, 3, sizeof(filter), NULL);
	ESWEEP_MALLOC(filter[0], 1, sizeof(esweep_object), NULL);
	ESWEEP_MALLOC(filter[1], 1, sizeof(esweep_object), NULL);
	ESWEEP_MALLOC(filter[2], 1, sizeof(esweep_object), NULL);
	filter[0]->samplerate=samplerate;
	filter[0]->type=WAVE;
	filter[0]->size=3;
	ESWEEP_MALLOC(filter[0]->data, 3, sizeof(Wave), NULL);
	filter[1]->samplerate=samplerate;
	filter[1]->type=WAVE;
	filter[1]->size=3;
	ESWEEP_MALLOC(filter[1]->data, 3, sizeof(Wave), NULL);
	filter[2]->samplerate=samplerate;
	filter[2]->type=COMPLEX;
	filter[2]->size=3;
	ESWEEP_MALLOC(filter[2]->data, 3, sizeof(Complex), NULL);

	numZ=filter[0]->data;
	denomZ=filter[1]->data;
	/*
	 * Make a bilinear transform to get the coefficients in the z-plane.
	 * By dividing through denomZ[0] the filter is normalized
	 */

	denomZ[0]=denom[0]*Kp*Kp+denom[1]*Kp+denom[2];
	denomZ[1]=(2*Kp*Kp*denom[0]-2*denom[2]);
	denomZ[2]=(denom[0]*Kp*Kp-denom[1]*Kp+denom[2]);

	numZ[0]=(num[0]*Kz*Kz+num[1]*Kz+num[2]);
	numZ[1]=(2*Kz*Kz*num[0]-2*num[2]);
	numZ[2]=(num[0]*Kz*Kz-num[1]*Kz+num[2]);

	if (denomZ[0] != 0.0) {
		denomZ[1]/=denomZ[0];
		denomZ[2]/=denomZ[0];
		numZ[0]/=denomZ[0];
		numZ[1]/=denomZ[0];
		numZ[2]/=denomZ[0];
		denomZ[0]=1.0;
	}

	return filter;
}

esweep_object **esweep_createFilterFromArray(Real *num, Real *denom, int size, int samplerate) {
	esweep_object **filter;

	ESWEEP_ASSERT(samplerate > 0, NULL);
	ESWEEP_ASSERT(size > 0, NULL);
	ESWEEP_ASSERT(num != NULL, NULL);

	ESWEEP_MALLOC(filter, 3, sizeof(filter), NULL);

	ESWEEP_MALLOC(filter[0], 1, sizeof(esweep_object), NULL);
	filter[0]->samplerate=samplerate;
	filter[0]->type=WAVE;
	filter[0]->size=size;
	ESWEEP_MALLOC(filter[0]->data, size, sizeof(Real), NULL);
	memcpy(filter[0]->data, num, size*sizeof(Real));

	if (denom != NULL) {
		ESWEEP_MALLOC(filter[1], 1, sizeof(esweep_object), NULL);
		filter[1]->samplerate=samplerate;
		filter[1]->type=WAVE;
		filter[1]->size=size;
		ESWEEP_MALLOC(filter[1]->data, size, sizeof(Wave), NULL);
		memcpy(filter[1]->data, denom, size*sizeof(Real));
	} else {
    filter[1]=NULL;
  }

	ESWEEP_MALLOC(filter[2], 1, sizeof(esweep_object), NULL);
	filter[2]->samplerate=samplerate;
	filter[2]->type=COMPLEX;
	filter[2]->size=size;
	ESWEEP_MALLOC(filter[2]->data, size, sizeof(Complex), NULL);

	return filter;
}

esweep_object **esweep_cloneFilter(esweep_object *src[]) {
	esweep_object **filter;
	int i;
	ESWEEP_FILTER_CHECK(src, NULL, NULL, NULL, NULL);

	ESWEEP_MALLOC(filter, 3, sizeof(filter), NULL);

	for (i=0; i<3; i++) {
		filter[i]=esweep_clone(src[i]);
		ESWEEP_ASSERT(filter != NULL, NULL);
	}
	return filter;
}

int esweep_appendFilter(esweep_object *dst[], esweep_object *src[]) {
	esweep_object *tmp;
	int dst_size, src_size;
	ESWEEP_FILTER_CHECK(src, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);
	ESWEEP_FILTER_CHECK(dst, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);

	ESWEEP_SAME_MAPPING(dst[0], src[0], ERR_DIFF_MAPPING);
	dst_size=(dst[0])->size;
	src_size=(src[0])->size;
	// copy numerator
	tmp=esweep_create("wave", (dst[0])->samplerate, dst_size+src_size);
	esweep_copy(tmp, dst[0], 0, 0, &dst_size);
	esweep_copy(tmp, src[0], dst_size, 0, &src_size);
	esweep_free(dst[0]);
	dst[0]=tmp;

	// copy denominator when necessary
	if (dst[1] != NULL) {
		tmp=esweep_create("wave", (dst[1])->samplerate, dst_size+src_size);
		esweep_copy(tmp, dst[1], 0, 0, &dst_size);
		if (src[1] != NULL) {
			esweep_copy(tmp, src[1], dst_size, 0, &src_size);
		}
		esweep_free(dst[1]);
		dst[1]=tmp;
	}
	else if (src[1] != NULL) {
		tmp=esweep_create("wave", (src[1])->samplerate, dst_size+src_size);
		esweep_copy(tmp, src[1], dst_size, 0, &src_size);
		dst[1]=tmp;
	} else {
		dst[1]=NULL;
	}

	// copy filter state

	tmp=esweep_create("complex", (dst[2])->samplerate, dst_size+src_size);
	esweep_copy(tmp, dst[2], 0, 0, &dst_size);
	esweep_copy(tmp, src[2], dst_size, 0, &src_size);
	esweep_free(dst[2]);
	dst[2]=tmp;

	return ERR_OK;
}

int esweep_resetFilter(esweep_object *filter[]) {
	ESWEEP_FILTER_CHECK(filter, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);
	memset(filter[2]->data, 0, filter[2]->size*sizeof(Complex));

	return ERR_OK;
}

int esweep_freeFilter(esweep_object *filter[]) {
	ESWEEP_ASSERT(filter != NULL, ERR_EMPTY_OBJECT);
	esweep_free(filter[0]);
	esweep_free(filter[1]);
	esweep_free(filter[2]);

	return ERR_OK;
}

#define FILE_ID "esweep_filter"

/* helper functions */
static int close_on_error(FILE *fp) {
	fclose(fp);
	return ERR_UNKNOWN;
}

static void *load_close_on_error(FILE *fp, esweep_object **filter) {
	fclose(fp);
	if (filter != NULL) {
		esweep_free(filter[0]);
		esweep_free(filter[1]);
		esweep_free(filter[2]);
	}
	return NULL;
}

int esweep_saveFilter(const char *filename, esweep_object *filter[]) {
	FILE *fp;
	const char *id=FILE_ID;
	int filter_type=-1;
	Real *wave;
	Complex *cpx;

	ESWEEP_ASSERT(filename != NULL, ERR_BAD_ARGUMENT);
	ESWEEP_ASSERT(strlen(filename) > 0, ERR_BAD_ARGUMENT);
	ESWEEP_FILTER_CHECK(filter, ERR_EMPTY_OBJECT, ERR_DIFF_MAPPING, ERR_SIZE_MISMATCH, ERR_NOT_ON_THIS_TYPE);

	/* open file */

	ESWEEP_ASSERT((fp = fopen(filename, "wb")) != NULL, ERR_UNKNOWN);

	/* write file id */
	ESWEEP_ASSERT(fwrite(id, strlen(id)*sizeof(char), 1, fp) == 1, close_on_error(fp));

	/* write samplerate */
	ESWEEP_ASSERT(fwrite(&(filter[0]->samplerate), sizeof(filter[0]->samplerate), 1, fp) == 1, close_on_error(fp));

	/* write samples */
	ESWEEP_ASSERT(fwrite(&(filter[0]->size), sizeof(filter[0]->size), 1, fp) == 1, close_on_error(fp));

	/* write filter type */
	if (filter[1]==NULL) filter_type=ESWEEP_FILTER_FIR;
	else filter_type=ESWEEP_FILTER_IIR;
	ESWEEP_ASSERT(fwrite(&filter_type, sizeof(filter_type), 1, fp) == 1, close_on_error(fp));

	/* write data */
	wave=filter[0]->data;
	ESWEEP_ASSERT(fwrite(wave, sizeof(Real), filter[0]->size, fp) == filter[0]->size, close_on_error(fp));
	if (filter_type == ESWEEP_FILTER_IIR) {
		wave=filter[1]->data;
		ESWEEP_ASSERT(fwrite(wave, sizeof(Real), filter[1]->size, fp) == filter[1]->size, close_on_error(fp));
	}
	cpx=filter[2]->data;
	ESWEEP_ASSERT(fwrite(cpx, sizeof(Complex), filter[2]->size, fp) == filter[2]->size, close_on_error(fp));

	fclose(fp);
	return ERR_OK;
}

esweep_object **esweep_loadFilter(const char *filename) {
	FILE *fp;
	int id_length=strlen(FILE_ID);
	char *id=(char*) calloc(id_length+1, sizeof(char));
	esweep_object **filter;
	int filter_type=-1;
	Real *wave;
	Complex *cpx;

	ESWEEP_ASSERT(filename != NULL, NULL);
	ESWEEP_ASSERT(strlen(filename) > 0, NULL);

	/* open input file */

	ESWEEP_ASSERT((fp = fopen(filename, "rb")) != NULL, NULL);

	/* read file id */
	ESWEEP_ASSERT(fread(id, sizeof(char), id_length, fp) == id_length, load_close_on_error(fp, NULL));
	ESWEEP_ASSERT(strcmp(id, FILE_ID) == 0, load_close_on_error(fp, NULL));
	free(id);

	/* create a new filter object */
	ESWEEP_MALLOC(filter, 3, sizeof(filter), NULL);
	ESWEEP_MALLOC(filter[0], 1, sizeof(esweep_object), NULL);

	/* read samplerate */
	ESWEEP_ASSERT(fread(&(filter[0]->samplerate), sizeof(filter[0]->samplerate), 1, fp) == 1, load_close_on_error(fp, filter));

	/* read size */
	ESWEEP_ASSERT(fread(&(filter[0]->size), sizeof(filter[0]->size), 1, fp) == 1, load_close_on_error(fp, filter));

	/* read filter type */
	ESWEEP_ASSERT(fread(&filter_type, sizeof(filter_type), 1, fp) == 1, load_close_on_error(fp, filter));
	ESWEEP_ASSERT(filter_type == ESWEEP_FILTER_FIR || filter_type == ESWEEP_FILTER_IIR, NULL);

	/* read data */
	filter[0]->type=WAVE;
	ESWEEP_MALLOC(filter[0]->data, filter[0]->size, sizeof(Wave), NULL);
	wave=filter[0]->data;
	ESWEEP_ASSERT(fread(&wave, sizeof(Real), filter[0]->size, fp) == filter[0]->size, load_close_on_error(fp, filter));

	if (filter_type==ESWEEP_FILTER_IIR) {
		ESWEEP_MALLOC(filter[1], 1, sizeof(esweep_object), NULL);
		filter[1]->samplerate=filter[0]->samplerate;
		filter[1]->type=WAVE;
		filter[1]->size=filter[0]->size;
		ESWEEP_MALLOC(filter[1]->data, filter[1]->size, sizeof(Wave), NULL);
		wave=filter[1]->data;
		ESWEEP_ASSERT(fread(&wave, sizeof(Real), filter[1]->size, fp) == filter[1]->size, load_close_on_error(fp, filter));
	} else {filter[1]=NULL; }

	ESWEEP_MALLOC(filter[2], 1, sizeof(esweep_object), NULL);
	filter[2]->samplerate=filter[0]->samplerate;
	filter[2]->type=COMPLEX;
	filter[2]->size=filter[0]->size;
	ESWEEP_MALLOC(filter[2]->data, filter[2]->size, sizeof(Wave), NULL);
	cpx=filter[2]->data;
	ESWEEP_ASSERT(fread(&cpx, sizeof(Real), filter[2]->size, fp) == filter[2]->size, load_close_on_error(fp, filter));

	fclose(fp);
	return filter;
}

