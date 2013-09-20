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

/* 
 * This examples shows, how to record from the soundcard and play the input with adjustable delay. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esweep.h"

int main() {
	u_int samplerate=48000; 
	float delay=0.1; // 100 ms delay
	u_int blocksize, tmp; 
	esweep_audio *hdl; // the soundcard handle
	int offset; 

	esweep_object *buffer[4]; // audio buffer, stereo
	esweep_object *table; 

	/* 
	 * The blocksize depends on the samplerate and the delay. 
	 */
	blocksize=(u_int) (delay*samplerate+0.5); 
	blocksize=8192; 

	/* 
	 * Create buffers, for seamless playback and record the size of the buffers must
	 * be an integer multiple of blocksize. In this case, blocksize is right. 
	 */
	buffer[0]=esweep_create("complex", samplerate, blocksize); 
	buffer[1]=esweep_create("complex", samplerate, blocksize); 
	buffer[2]=esweep_create("complex", samplerate, blocksize); 
	buffer[3]=esweep_create("complex", samplerate, blocksize); 
	table=esweep_create("wave", samplerate, blocksize); 
	esweep_createFFTTable(table, blocksize); 

	/*
	 * Open the soundcard, then configure it for the desired blocksize
	 */

	hdl=esweep_audioOpen("audio:/dev/audio"); 
	if (hdl==NULL) exit(1); 
	esweep_audioConfigure(hdl, "framesize", blocksize); 
	esweep_audioConfigure(hdl, "samplerate", samplerate); 

	/*
	 * We have to check if the blocksize is set
	 */

	
	esweep_audioQuery(hdl, "framesize", &tmp); 
	if (tmp != blocksize) {
		fprintf(stderr, "Delay not possible\n"); 
		return -1; 
	}

	/* Sync input and output buffer */
	esweep_audioSync(hdl); 
	
	/*
	 * And now the record-playback-loop
	 */

	offset=0; 
	while (1) {
		esweep_audioIn(hdl, buffer, 2, &offset); 
		esweep_audioOut(hdl, buffer, 2, &offset); 
		esweep_fft(buffer[0], buffer[0], table); 
		esweep_fft(buffer[1], buffer[1], table); 
	}

	return 0; 
}

