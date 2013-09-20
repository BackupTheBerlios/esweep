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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsp.h"
#include "esweep.h"
#include "fft.h"

/*
calculate the classic CSD from WAVE "in"
output in "out" is of type SURFACE
see theory section in doc
*/

int esweep_csd(esweep_object *out, esweep_object *in, Real time_step, int steps, Real rise_time, Real smooth_factor) {
	Complex *complex;
	Polar *polar;
	Wave *wave;
	Surface *surf;
	int fft_size, i, j;
	int rise_samples, step_samples;
	Complex *fft_table;
	Real df;

	if ((in==NULL) || ((*in).data==NULL) || ((*in).size<=0)) return ERR_EMPTY_OBJECT;
	if (out==NULL) return ERR_EMPTY_OBJECT;
	if (((*in).type!=WAVE) || ((*out).type!=SURFACE)) return ERR_NOT_ON_THIS_TYPE;

	fft_size=(int) (pow(2, ceil(log((*in).size)/log(2)))+0.5);
	df=(Real) (*in).samplerate/fft_size;

	/* check arguments */

	if (!(time_step>0.0)) return ERR_BAD_ARGUMENT;
	if (steps<=0) return ERR_BAD_ARGUMENT;
	if (steps*time_step/1000>1/df) return ERR_BAD_ARGUMENT;

	rise_samples=(int) (rise_time/1000*(*in).samplerate+0.5);
	if ((rise_samples<0) || (rise_samples > (*in).size)) return ERR_BAD_ARGUMENT;
	if (smooth_factor<0.1) return ERR_BAD_ARGUMENT;

	/* allocate enough space for the resultant surface */

	surf=(Surface*) (*out).data;
	if ((*out).data!=NULL) {
		free((*surf).x);
		free((*surf).y);
		free((*surf).z);
		free(surf);
	}

	surf=(Surface*) calloc(1, sizeof(Surface));
	(*out).data=surf;
	(*out).samplerate=(*in).samplerate;

	/* the x-vector is the frequency axis, we do not save the hermitian redundancy */

	(*surf).xsize=fft_size/2+1;
	(*surf).x=(Real*) calloc((*surf).xsize, sizeof(Real));

	/* the y-vector is the time axis */

	(*surf).ysize=steps;
	(*surf).y=(Real*) calloc((*surf).ysize, sizeof(Real));

	/* the z vector is the dependent part */
	(*surf).z=(Real*) calloc((*surf).xsize*(*surf).ysize, sizeof(Real));

	/* fill the x vector */

	for (i=0;i<(*surf).xsize;i++) {
		(*surf).x[i]=i*df;
	}

	/* fill the y vector */

	for (i=0;i<(*surf).ysize;i++) {
		(*surf).y[i]=i*time_step/1000;
	}

	/* set up fft table */

	fft_table=fft_create_table(fft_size);

	complex=(Complex*) calloc(fft_size, sizeof(Complex));
	polar=(Polar*) complex;
	wave=(Wave*) (*in).data;

	for (i=0, step_samples=0;i<steps;i++, step_samples+=(int) (time_step/1000*(*in).samplerate+0.5)) {
		/* copy part of input wave into temporary complex array */
		r2c(complex, &(wave[step_samples]), (*in).size-step_samples);

		/* windowing of the rise time, we use a blackman window */
		window_complex(complex, 0, rise_samples, WIN_LEFT, WIN_BLACK);

		/* in-place fft */
		fft(complex, fft_table, fft_size, FFT_FORWARD);

		/* converting to polar */
		c2p(polar, complex, fft_size);

		/* smoothing */
		smooth(polar, smooth_factor, fft_size);

		/* copy to surface structure, no redundancy, no phase*/
		for (j=0;j<(*surf).xsize;j++) {
			(*surf).z[i+j*(*surf).ysize]=polar[j].abs;
		}

		/* zero-out complex array */
		memset(complex, 0, fft_size*sizeof(Complex));
	}
	free(fft_table);
	free(complex);
	return ERR_OK;
}

/*
calculate the burst decay from WAVE "in"
it is a convolution of "in" with Morlet-Wavelets of various center frequencies
*/

int esweep_cbsd(esweep_object *out, esweep_object *in, Real f1, Real f2, int resolution, int periods, int steps, char time_shift) {
	Surface *surf;
	Complex *wave;
	Complex *wave_out;
	Complex *fft_table;
	int fft_size, i, j;
	Real freq, omega, dw;
	Real tau; /* time constant auf gaussian window */
	Real step_width;
	Real re, im; /* temporary variables */

	if ((in==NULL) || ((*in).data==NULL) || ((*in).size<=0)) return ERR_EMPTY_OBJECT;
	if (out==NULL) return ERR_EMPTY_OBJECT;
	if (((*in).type!=WAVE) || ((*out).type!=SURFACE)) return ERR_NOT_ON_THIS_TYPE;

	if ((periods<1) || (steps<1) || (resolution<2)) return ERR_BAD_ARGUMENT;
	if ((f2<f1) || (f2>=(*in).samplerate/2) || (f1<(*in).samplerate/(*in).size)) return ERR_BAD_ARGUMENT;

	/*
	The morlet wavelet is of infinite length,
	but the most significant part is in approximately two times "resolution" periods
	(before and after the amplitude is below 1e-4)
	To avoid smearing due to wrap-around in the convolution
	the total length must be at least "two times 'resolution' periods of the lowest f"+"the
	length of the input array"+1.
	Or, if we need more space to get all periods in the output array, use that size
	*/
	fft_size=(int) ((*in).samplerate*2*resolution/f1+(*in).size+1+0.5);
	if ((int)((*in).samplerate*periods/f1+0.5) > fft_size) fft_size=(int)((*in).samplerate*periods/f1+0.5);

	/* make this a power of 2 */
	fft_size=(int) (pow(2, ceil(log(fft_size)/log(2)))+0.5);

	surf=(Surface*) (*out).data;
	if ((*out).data!=NULL) {
		free((*surf).x);
		free((*surf).y);
		free((*surf).z);
		free(surf);
	}

	surf=(Surface*) calloc(1, sizeof(Surface));
	(*out).data=surf;
	(*out).samplerate=(*in).samplerate;

	/*
	the x-vector is the frequency axis
	the frequencies are spaced so that the spectra of the wavelets cross at -3dB
	*/
	(*surf).xsize=(int) (log(f2/f1)/log(1+0.5/resolution)+0.5);
	(*surf).x=(Real*) calloc((*surf).xsize, sizeof(Real));

	/* the y-vector is the period axis */
	(*surf).ysize=steps*periods;
	(*surf).y=(Real*) calloc((*surf).ysize, sizeof(Real));

	/* fill y array */
	for (i=0;i<(*surf).ysize;i++) {
		(*surf).y[i]=(Real) i/steps;
	}

	/* the z vector is the dependent part */
	(*surf).z=(Real*) calloc((*surf).xsize*(*surf).ysize, sizeof(Real));

	/* allocate temporary arrays */
	wave=(Complex*) calloc(fft_size, sizeof(Complex));
	wave_out=(Complex*) calloc(fft_size, sizeof(Complex));

	/* set up fft table */
	fft_table=fft_create_table(fft_size);

	/* copy input to wave and fft it */
	r2c(wave, (Wave*) (*in).data, (*in).size);
	fft(wave, fft_table, fft_size, FFT_FORWARD);
	dw=2*M_PI*(*in).samplerate/fft_size;
	for (i=0, freq=f1;i<(*surf).xsize;i++, freq=freq*(1+0.5/resolution)) {
		omega=2*M_PI*freq;

		/* calculate the time constant of the gaussian window */
		tau=2.35482*resolution/omega;

		/*
		the convolution is now the simple complex multiplication
		of the FFTed input wave with the spectrum of the morlet wavelet.
		The wavelet is shifted "resolution" periods of the actual frequency
		towards positive times
		*/
		if (time_shift=='y') {
			for (j=0;j<fft_size;j++) {
				re=tau*exp(-((j*dw-omega)*(j*dw-omega))*tau*tau/4); /* magnitude of the wavelet spectrum */
				im=-re*sin(j*dw*resolution/freq);
				re=re*cos(j*dw*resolution/freq);

				wave_out[j].real=re*wave[j].real-im*wave[j].imag;
				wave_out[j].imag=im*wave[j].real+re*wave[j].imag;
			}
		} else {
			for (j=0;j<fft_size;j++) {
				re=tau*exp(-((j*dw-omega)*(j*dw-omega))*tau*tau/4); /* magnitude of the wavelet spectrum */

				wave_out[j].real=re*wave[j].real;
				wave_out[j].imag=re*wave[j].imag;
			}
		}

		/* now the inverse fourier transform */
		fft(wave_out, fft_table, fft_size, FFT_BACKWARD);

		/* the magnitude of wave_out is what we need */
		step_width=(Real) (*in).samplerate/(steps*freq);
		for (j=0;j<(*surf).ysize;j++) {
			(*surf).z[j+i*(*surf).ysize]=2*hypot(wave_out[(int)(j*step_width+0.5)].real, wave_out[(int)(j*step_width+0.5)].imag)/fft_size;
		}
		(*surf).x[i]=freq;
	}

	free(wave);
	free(wave_out);
	free(fft_table);

	return ERR_OK;
}

/*
adds "b" to SURFACE "a"
"b" may be WAVE or POLAR
"axis" denotes the independant variable, either 'x' or 'y'
In case of WAVE, the time axis is used, case of POLAR the frequency axis
"index" is the index where to put the new data on the other axis
"dep" is the value for the other axis
*/

int esweep_addToSurface(esweep_object *a, esweep_object *b, char axis, int index, Real dep) {
	Surface *surf;
	Wave *wave;
	Polar *polar;
	int i;

	if ((a==NULL) || ((*a).size<=0) || ((*a).data==NULL)) return ERR_EMPTY_OBJECT;
	if ((b==NULL) || ((*b).size<=0) || ((*b).data==NULL)) return ERR_EMPTY_OBJECT;

	if ((*a).type!=SURFACE) return ERR_NOT_ON_THIS_TYPE;
	if (((*b).type!=WAVE) && ((*b).type!=POLAR)) return  ERR_NOT_ON_THIS_TYPE;

	surf=(Surface*)(*a).data;
	if (((*surf).xsize<=0) || ((*surf).ysize<=0)) return ERR_EMPTY_OBJECT;
	if (((*surf).x==NULL) || ((*surf).y==NULL) || ((*surf).z==NULL)) return ERR_EMPTY_OBJECT;

	if ((axis!='x') && (axis!='y')) return ERR_BAD_ARGUMENT;
	if (index<0) return ERR_BAD_ARGUMENT;
	if ((axis=='x') && (index>(*surf).ysize)) return ERR_BAD_ARGUMENT;
	if ((axis=='y') && (index>(*surf).xsize)) return ERR_BAD_ARGUMENT;

	switch ((*b).type) {
		case WAVE:
			if (axis=='x') {
				if ((*surf).xsize!=(*b).size) return ERR_DIFF_MAPPING;
				(*surf).y[index]=dep;
				/* copy the WAVE in the SURFACE */
				wave=(Wave*) (*b).data;
				for (i=0;i<(*surf).xsize;i++) {
					(*surf).x[i]=(Real) i/(*b).samplerate;
					(*surf).z[index+i*(*surf).ysize]=wave[i];
				}
			} else if (axis=='y') {
				if ((*surf).ysize!=(*b).size) return ERR_DIFF_MAPPING;
				(*surf).x[index]=dep;
				/* copy the WAVE in the SURFACE */
				wave=(Wave*) (*b).data;
				for (i=0;i<(*surf).ysize;i++) {
					(*surf).y[i]=(Real) i/(*b).samplerate;
					(*surf).z[i+index*(*surf).ysize]=wave[i];
				}
			}
			break;
		case POLAR:
			if (axis=='x') {
				/* no redundancy */
				if ((*surf).xsize!=(*b).size/2+1) return ERR_DIFF_MAPPING;
				(*surf).y[index]=dep;
				/* copy the magnitude in the SURFACE */
				polar=(Polar*) (*b).data;
				for (i=0;i<(*surf).xsize;i++) {
					(*surf).x[i]=(Real) i*(*b).samplerate/(*b).size;
					(*surf).z[index+i*(*surf).ysize]=polar[i].abs;
				}

			} else if (axis=='y') {
				if ((*surf).ysize!=(*b).size/2+1) return ERR_DIFF_MAPPING;
				(*surf).x[index]=dep;
				/* copy the magnitude in the SURFACE */
				polar=(Polar*) (*b).data;
				for (i=0;i<(*surf).ysize;i++) {
					(*surf).y[i]=(Real) i*(*b).samplerate/(*b).size;
					(*surf).z[i+index*(*surf).ysize]=polar[i].abs;
				}
			}
			break;
		default:
			break;
	}
	return ERR_OK;
}
