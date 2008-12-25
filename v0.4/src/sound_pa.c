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
 * PortAudio Portable Real-Time Audio Library 
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
*/

#ifdef PORTAUDIO

#include <stdlib.h>
#include <stdio.h>

#include "portaudio.h"

#include "esweep.h"
#include "sound.h"

/* we need this on MACs */
#define FRAMES_PER_BUFFER 1024

long snd_open(const char *device, long samplerate) {
	PaStream *stream;
	PaError err;
	PaStreamParameters inputParameters, outputParameters;
	
	/* initialize portaudio */
	
	err=Pa_Initialize();
	if (err!=paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}
	
	/* 
	 * we work in stereo 
	 * mono would mix both channels together, in stereo we can guarantee
	 * that one channel is absolutely quiet
	*/

    /* setup input and output parameters */
    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* setup stream */
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              &outputParameters,
              samplerate,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL ); /* no callback, so no callback userData */
	if (err!=paNoError) { 
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}
	
	err=Pa_StartStream(stream);
    if (err != paNoError) { 
	      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
	      return 1;
    }
	
	return (long) stream;
}

long snd_play(Audio *out, long size, long handle) {
    PaStream *stream=(PaStream*) handle;
    long frames_left=size/2; /* one frame <=> 2 stereo samples */
    long frames_to_rw=FRAMES_PER_BUFFER;
    long offset=0;
    long err;

    while (frames_left>0) {
        err=Pa_WriteStream(stream, &(out[offset]), frames_to_rw);
        if (err != paNoError) { 
		      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		      return 1;
    	}
		frames_left-=FRAMES_PER_BUFFER;
		offset+=2*FRAMES_PER_BUFFER;
		if (frames_left<FRAMES_PER_BUFFER) frames_to_rw=frames_left;
    }
    return 0;
}

long snd_rec(Audio *in, long size, long handle) {
	PaStream *stream=(PaStream*) handle;
    long frames_left=size/2; /* one frame <=> 2 stereo samples */
    long frames_to_rw=FRAMES_PER_BUFFER;
    long offset=0;
    long err;
    
    while (frames_left>0) {
        err=Pa_ReadStream(stream, &(in[offset]), frames_to_rw);
        if (err != paNoError) { 
		      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		      return 1;
    	}
		frames_left-=FRAMES_PER_BUFFER;
		offset+=2*FRAMES_PER_BUFFER;
		if (frames_left<FRAMES_PER_BUFFER) frames_to_rw=frames_left;
    }
    return 0;
}

long snd_pnr(Audio *in, Audio *out, long size, long handle) {
	PaStream *stream=(PaStream*) handle;
    long frames_left=size/2; /* one frame <=> 2 stereo samples */
    long frames_to_rw=FRAMES_PER_BUFFER;
    long offset=0;
    long err;
    
    while (frames_left>0) {
        err=Pa_WriteStream(stream, &(out[offset]), frames_to_rw);
        if (err != paNoError) { 
		      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		      return 1;
    	}
    	err=Pa_ReadStream(stream, &(in[offset]), frames_to_rw);
        if (err != paNoError) { 
		      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		      return 1;
    	}
		frames_left-=FRAMES_PER_BUFFER;
		offset+=2*FRAMES_PER_BUFFER;
		if (frames_left<FRAMES_PER_BUFFER) frames_to_rw=frames_left;
    }
    return 0;
}

void snd_close(long handle) {
	PaStream *stream=(PaStream*) handle;
	long err;
	
    err=Pa_StopStream(stream);
    if (err != paNoError) { 
	      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    }
	Pa_CloseStream(stream);
    if (err != paNoError) { 
	      printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    }
	Pa_Terminate();
}

#endif /* PORTAUDIO */
