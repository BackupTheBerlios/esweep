/*
 * Copyright (c) 2009 Jochen Fabricius <jfab@berlios.de>
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

/* 
 * This examples shows, how to filter an input stream with the help of fast convolution
 * and plays it back simultaneously. 
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esweep.h"

/*
 * Create the impulse response of a first order lowpass, don't worry about this function. 
 */
esweep_object *createLowpass(u_int samplerate, u_int length); 

int main() {
	u_int samplerate=48000; 
	u_int L, M, fft_size; // L: input size, M: filter size
	u_int blocksize, tmp; 
	u_int offset; // playback and record offset
	esweep_audio *hdl; // the soundcard handle

	/* 
	 * Of course, FFT size has to be a power of 2. 
	 */
	fft_size=131072; 

	/* 
	 * For seamless playback and record, the FFT size must be an
	 * integer multiple of the soundcard blocksize. Thus, blocksize must be also
	 * a power of 2. We use 2048, so remember not to use a smaller FFT size. 
	 */
	blocksize=2048; 

	/* 
	 * Disrecte convolution creates an output of length L+M-1. 
	 * Thus, L and M must be set that the formula above is equal to fft_size. 
	 * Nothing could be easier. 
	 */
	L=fft_size/2; 
	M=fft_size-L+1; 

	/* 
	 * We use the overlap-save (or overlap-scrap/overlap-discard)
	 * algorithm. To implement this with esweep, we need two buffers, each in stereo. 
	 * Every buffer must be of type COMPLEX this saves a lot of time during convolution. 
	 * For the OLS algorithm, we need to record fft_size samples, so don't wonder why
	 * we are not using L here. 
	 */
	esweep_object *in0[2]; 
	in0[0]=esweep_create("complex", samplerate, fft_size); 
	in0[1]=esweep_create("complex", samplerate, fft_size); 
	esweep_object *in1[2]; 
	in1[0]=esweep_create("complex", samplerate, fft_size); 
	in1[1]=esweep_create("complex", samplerate, fft_size); 

	/*
	 * This is the filter. This function creates the impulse response of a simple 
	 * first order low pass, just for demonstration purposes. 
	 */
	esweep_object *ir=createLowpass(samplerate, M); 

	/* 
	 * The impulse response should be pre-transformed, this saves a lot of time. 
	 * Esweep does not know in-place transforms, so we need another esweep container. 
	 */
	esweep_object *filter=esweep_create("complex", samplerate, fft_size); 

	/* 
	 * Another huge speed-up is possible by using a pre-calculated FFT table. Otherwise, 
	 * each call to esweep_fft() will create its own table, which is computationally
	 * expensive. esweep_createFFTTable() will set the size of the containers anyway, 
	 * so we can simply create the container with size 0. 
	 */
	esweep_object *fft_table=esweep_create("complex", samplerate, 0); 
	esweep_createFFTTable(fft_table, fft_size); 

	/* 
	 * Now the pre-transformation. The impulse response is of size M, so it has to be zero-padded. 
	 * Thanks to the new FFT interface in esweep, this is done automatically. But be careful: automatic 
	 * zero-padding adds zeroes only up to the next power of 2. For other FFT sizes, you still have to 
	 * pad manually. 
	 */

	esweep_fft(filter, ir, fft_table); 

	/* 
	 * We don't need the impulse response any longer
	 */
	esweep_free(ir); 

	/*
	 * Open the soundcard, then configure it for the desired blocksize
	 */

	hdl=esweep_audioOpen("/dev/audio", samplerate); 
	esweep_audioConfigure(hdl, "framesize", blocksize); 

	/*
	 * We have to check if the blocksize is set
	 */

	esweep_audioQuery(hdl, "framesize", &tmp); 
	if (tmp != blocksize) {
		fprintf(stderr, "Blocksize not possible\n"); 
		return -1; 
	}

	/* Sync input and output buffer */
	esweep_audioSync(hdl); 
	
	/*
	 * And now the record-playback-loop
	 */

	while (1) {
		
		/* 
		 * Start play/recording at the end of the filter length. This discards
		 * the first M-1 samples, which are corrupted by the convolution wrap-around. 
		 */
		offset=M-1; 
		while (offset<fft_size) {
			esweep_audioIn(hdl, in0, 2, offset); 
			offset=esweep_audioOut(hdl, in1, 2, offset);  
		}

		/* 
		 * The last M-1 samples of the input will be wrapped around during 
		 * convolution. Thus, we have to save them into the other buffer (hence the name). 
		 */
		esweep_copy(in1[0], in0[0], 0, L, M-1); 
		esweep_copy(in1[1], in0[1], 0, L, M-1); 
		/* 
		 * Now it's time for convolution!
		 */
		esweep_convolve(in0[0], filter, fft_table); 
		esweep_convolve(in0[1], filter, fft_table); 
		/*
		 * Simply switch the buffers. We reuse ir here as a temporary switching variable. 
		 */
		ir=in0[0]; 
		in0[0]=in1[0]; 
		in1[0]=ir; 

		ir=in0[1]; 
		in0[1]=in1[1]; 
		in1[1]=ir; 
		/* That's all!!! */
	}

	return 0; 
}


esweep_object *createLowpass(u_int samplerate, u_int length) {
	esweep_object *ir=esweep_create("wave", samplerate, length); 
	Wave *wave; 
	u_int i; 

	/*
	 * This is really low level. Don't do this at home. 
	 */

	/*
	 * Create the step response for a low-pass filter with fc=1000 Hz.  
	 */
	wave=(Wave*) (ir->data); 
	for (i=0; i<length; i++) {
		wave[i]=1-exp(-2*M_PI*1000*i/samplerate); 
	}
	esweep_differentiate(ir); 
	esweep_divScalar(ir, 20*esweep_max(ir), 0); 

	return ir; 
}
