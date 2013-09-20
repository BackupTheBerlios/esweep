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

/* 
 * This examples shows a 2-channel 3-way IIR crossover application
 */

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esweep.h"

void usage() {
	fprintf(stderr, "usage: xover -l latency -s samplerate -g gain\n");
	exit(1); 
}

#define PLAY_CHANNELS 6
#define REC_CHANNELS 2

int main(int argc, char *argv[]) {
	int samplerate; 
	double latency; 
	int framesize; 
	int offset; 

	int i; 

	double max, min; 

	// gain variable 
	esweep_object *gain; 
	double g; 

	// audio buffers
	esweep_object *in[REC_CHANNELS]; 
	esweep_object *out[PLAY_CHANNELS]; 

	// filter array
	esweep_object **filter[PLAY_CHANNELS]; 

	// audio device name
	char device[256]="/dev/audio"; 
	// audio handle
	esweep_audio *hdl; 

	// name of filter files, dynamically generated
	char filename[256]; 

	// set defaults
	g=0.9; 

	samplerate=48000; // Hz
	latency=20.0; // ms

	// get cmdline options
	char *errPtr; 
	char ch; 
	while ((ch = getopt(argc, argv, "l:s:g:f:")) != -1) {
		switch (ch) {
			case 'l':
				errno=0; 
				latency=strtod(optarg, &errPtr); 
				// minimum latency is 5 ms
				if (errno == ERANGE || latency < 5) usage(); 
				break; 
			case 's':
				samplerate=strtol(optarg, &errPtr, 10); 
				if (optarg[0] == '\0' || *errPtr != '\0') usage(); 
				if (errno == ERANGE && (samplerate == INT_MAX || samplerate == INT_MIN)) usage(); 
				if (samplerate < 0) usage(); 
				break; 
			case 'g':
				errno=0; 
				g=strtod(optarg, &errPtr); 
				if (errno == ERANGE || g < 0 || g > 1) usage(); 
				break; 
			case 'f':
				strlcpy(device, optarg, 256); 
				break; 
			default: 
				usage(); 
				/* NOT REACHED */
		}
	}

	// set gain
	gain=esweep_create("wave", samplerate, 1); 
	esweep_buildWave(gain, &g, 1); 

	// calculate framesize
	framesize=(int) (0.5+samplerate*latency/1000); 

	// open audio device, test if the configuration works
	hdl=esweep_audioOpen(device, samplerate); 
	if (hdl == NULL) {
		fprintf(stderr, "Can't open audio device %s with samplerate %d\n", device, samplerate); 
		exit(1); 
	}
	if (esweep_audioConfigure(hdl, "play_channels", PLAY_CHANNELS) != ERR_OK) {
		exit(1); 
	}
	if (esweep_audioConfigure(hdl, "rec_channels", REC_CHANNELS) != ERR_OK) {
		exit(1); 
	}
	if (esweep_audioConfigure(hdl, "framesize", framesize) != ERR_OK) {
		esweep_audioQuery(hdl, "framesize", &framesize); 
	}
	 
	// load the filters, they must exist in the execution directory, and must be named according
	// to their channel: 
	// CH1: ch1.filter; CH2: ch2.filter; and so on
	// If no filter file exists for a channel, a NULL filter is created
	for (i=0; i < PLAY_CHANNELS; i++) {
		snprintf(filename, 256, "ch%i.filter", i); 
		if ((filter[i]=esweep_loadFilter(filename)) == NULL) {
			// create dummy filter
			filter[i]=esweep_createFilterFromCoeff("gain", 1, 1, 1, 1, 1, samplerate); 
		}
		if (filter[i] == NULL) {
			fprintf(stderr, "Error creating filter for channels %i\n", i); 
			exit(1); 
		}
	}

	// create input buffer objects
	for (i=0; i < REC_CHANNELS; i++) {
		in[i]=esweep_create("wave", samplerate, framesize);
	}

	// create output buffer objects
	for (i=0; i < PLAY_CHANNELS; i++) {
		out[i]=esweep_create("wave", samplerate, framesize);
	}

	// start audio
	esweep_audioSync(hdl); 
	while (1) {
		offset=0; 
		esweep_audioOut(hdl, out, PLAY_CHANNELS, &offset); 

		offset=0; 
		esweep_audioIn(hdl, in, REC_CHANNELS, &offset); 
		// apply gain
		esweep_mul(in[0], gain); 
		esweep_mul(in[1], gain); 
		esweep_max(in[0], &max); 
		esweep_min(in[1], &min); 
		if (max > 0.9 || min < -0.95) fprintf(stderr, "Overload\n"); 


		// copy audio data to the output buffers
		// input channel 1 goes into output channels 1, 2, 3
		// input channel 2 goes into output channels 4, 5, 6
		for (i=0; i < PLAY_CHANNELS/2; i++) {
			esweep_copy(out[i], in[0], 0, 0, &framesize); 
		}
		for (; i < PLAY_CHANNELS; i++) {
			esweep_copy(out[i], in[1], 0, 0, &framesize); 
		}
		// filtering
		for (i=0; i < PLAY_CHANNELS; i++) {
			esweep_filter(out[i], filter[i]); 
		}
	}

	return 0; 
}

