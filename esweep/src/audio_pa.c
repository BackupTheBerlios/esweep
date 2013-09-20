/*
 * Copyright (c) 2012 Jochen Fabricius <jfab@berlios.de>
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
 * src/audio_pa.c:
 *  Audio I/O with the Portaudio library
 */


#ifdef PORTAUDIO

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <portaudio.h>

#include "esweep_priv.h"
#include "audio_device.h"

typedef struct __paAudioData {
	PaStream *stream; 
#define PAAUDIODATA_BUFFERS 2
	char *outBuf[PAAUDIODATA_BUFFERS]; // output buffer
	char *inBuf[PAAUDIODATA_BUFFERS]; // input buffer
#define PAAUDIODATA_BUF_FINISHED 0
#define PAAUDIODATA_BUF_PENDING 1
	char outBufMask[PAAUDIODATA_BUFFERS]; // is the buffer finished? 
	char inBufMask[PAAUDIODATA_BUFFERS]; 
	int outBufQueue; // which buffer is next in queue
	int inBufQueue; 
	int out_channels; 
	int in_channels; 
	PaDeviceIndex inDev, outDev; 
#define PAAUDIODATA_FLAG_NOFLAG  0x0000
#define PAAUDIODATA_FLAG_PERSIST 0x0001
	int flags; 
} paAudioData; 

int paAudioDeviceCallback( const void *input,
                                      void *output,
                                      unsigned long frameCount,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData ) ;

esweep_audio *audio_device_open(const char *device) {
	esweep_audio *handle; 
	paAudioData *audioData; 
	PaError err; 
	int i; 
	const int defSamplerates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000, 384000, 0}; 
	PaStreamParameters inParams, outParams; 

	
	/* Initialize PortAudio */
	if ((err=Pa_Initialize()) != paNoError) {
		fprintf(stderr, "audio_device_open(): can't initialize PortAudio: %s\n", Pa_GetErrorText(err));
		return NULL;
	}

	/* create the paAudioData structure */
	ESWEEP_MALLOC(audioData, 1, sizeof(paAudioData), NULL); 

	/* get the default devices */
	if ((audioData->inDev=Pa_GetDefaultInputDevice()) == paNoDevice) {
		fprintf(stderr, "audio_device_open(): no default input device\n");
		free(audioData); 
		return NULL;
	}
		
	if ((audioData->outDev=Pa_GetDefaultOutputDevice()) == paNoDevice) {
		fprintf(stderr, "audio_device_open(): no default output device\n");
		free(audioData); 
		return NULL;
	}

	/* set the default parameters */
	inParams.channelCount = outParams.channelCount = 2; 
	inParams.hostApiSpecificStreamInfo = outParams.hostApiSpecificStreamInfo = NULL;  
	inParams.sampleFormat = outParams.sampleFormat = paFloat32; 
	inParams.device = audioData->inDev; 
	outParams.device = audioData->outDev; 

	/* find the first matching samplerate */
	for (i=0; defSamplerates[i] > 0; i++) {
		if (Pa_IsFormatSupported(&inParams, &outParams, defSamplerates[i]) == 0) break; 
	}
	if (defSamplerates[i] <= 0) {
		fprintf(stderr, "audio_device_open(): couldn't find a matching samplerate\n");
		free(audioData); 
		return NULL;
	}

	/* allocate the esweep_audio structure, and set some defaults */
	ESWEEP_MALLOC(handle, 1, sizeof(esweep_audio), NULL); 

	handle->samplerate=defSamplerates[i]; 
	handle->play_channels=2; 
	handle->rec_channels=2; 
	handle->precision=32; /* we use the paFloat32 sample format */
	handle->bps=Pa_GetSampleSize(paFloat32); 
	handle->msb=1; /* not used */
	handle->framesize=2048; 
	handle->play_blocksize=handle->play_channels*handle->framesize*handle->bps; 
	handle->rec_blocksize=handle->rec_channels*handle->framesize*handle->bps; 

	/* open the PortAudio stream
	   we use defaults at the moment, this means we open the default device, ignoring
	   the "device"-parameter */

	if ((err=Pa_OpenDefaultStream(	&(audioData->stream), 
				  	handle->rec_channels,	
					handle->play_channels,
					paFloat32, 		/* use the PortAudio sample converter from FP to Int */
					handle->samplerate,	
					handle->framesize,
					paAudioDeviceCallback, 
					audioData)) != paNoError) {
		fprintf(stderr, "audio_device_open(): can't open PortAudio stream: %s\n", Pa_GetErrorText(err));
		free(handle); 
		free(audioData); 
		return NULL;
	}

	handle->pData=(char*) audioData; /* use the audioData as the device handle */
	handle->play_pause=1; 
	handle->rec_pause=1; 

	/* allocate in/out buffers. We use PAAUDIODATA_BUFFERS times the blocksize, and let the audioData buffers
	   point to each sub-buffer. This avoids underruns when the callback  */
	ESWEEP_MALLOC(handle->obuf, PAAUDIODATA_BUFFERS*handle->play_blocksize, sizeof(char), NULL); 
	ESWEEP_MALLOC(handle->ibuf, PAAUDIODATA_BUFFERS*handle->rec_blocksize, sizeof(char), NULL); 
	for (i=0; i < PAAUDIODATA_BUFFERS; i++) {
		audioData->outBuf[i]=handle->obuf+i*handle->play_blocksize; 
		audioData->outBufMask[i]=PAAUDIODATA_BUF_FINISHED; 
		audioData->inBuf[i]=handle->ibuf+i*handle->rec_blocksize; 
		audioData->inBufMask[i]=PAAUDIODATA_BUF_FINISHED; 
	}

	audioData->out_channels=handle->play_channels; 
	audioData->in_channels=handle->rec_channels; 
	/* The first buffer is in the queue */
	audioData->outBufQueue=0; 
	audioData->inBufQueue=0; 
	
	audioData->flags=PAAUDIODATA_FLAG_NOFLAG; 
	/* set the callback functions */
	handle->audio_query=(audio_query_ptr) &audio_device_query; 
	handle->audio_configure=(audio_configure_ptr) &audio_device_configure; 
	handle->audio_sync=(audio_sync_ptr) &audio_device_sync; 
	handle->audio_pause=(audio_pause_ptr) &audio_device_pause; 
	handle->audio_seek=(audio_seek_ptr) &audio_device_seek; 
	handle->audio_out=(audio_out_ptr) &audio_device_out; 
	handle->audio_in=(audio_in_ptr) &audio_device_in; 
	handle->audio_close=(audio_close_ptr) &audio_device_close; 

	/* return the handle */
	return handle;
}

int audio_device_query(const esweep_audio *handle, const char *query, int *result) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	if (strcmp("samplerate", query)==0) {
		*result=handle->samplerate; 
		return 0; 
	}
	if (strcmp("play_channels", query)==0) {
		*result=handle->play_channels; 
		return 0; 
	}
	if (strcmp("record_channels", query)==0) {
		*result=handle->rec_channels; 
		return 0; 
	}
	if (strcmp("precision", query)==0) {
		*result=handle->precision; 
		return 0; 
	}
	if (strcmp("framesize", query)==0) {
		*result=handle->framesize; 
		return 0; 
	}
	if (strcmp("persist", query)==0) {
		*result=audioData->flags & PAAUDIODATA_FLAG_PERSIST; 
		return 0; 
	}
	return 0; 
}

int audio_device_configure(esweep_audio *handle, const char *param, int value) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	esweep_audio newHdl; // structure with new configuration data */
	PaError err; 
	int i; 

	/* Close the stream */
	if ((err=Pa_CloseStream(audioData->stream)) != paNoError) {
		fprintf(stderr, "audio_device_configure(): can't close audio stream: %s\n",  Pa_GetErrorText(err));
		return -1;
	}

	handle->play_pause=1; 
	handle->rec_pause=1; 

	/* get defaults */
	newHdl.bps=handle->bps; 
	newHdl.samplerate=handle->samplerate; 
	newHdl.framesize=handle->framesize; 
	newHdl.play_channels=handle->play_channels; 
	newHdl.rec_channels=handle->rec_channels; 
	newHdl.play_blocksize=handle->play_blocksize; 
	newHdl.rec_blocksize=handle->rec_blocksize; 

	/* set the parameters */
	if (strcmp("samplerate", param)==0) {
		newHdl.samplerate=value; 
	} else if (strcmp("play_channels", param)==0) {
		newHdl.play_channels=value; 
		/* we need to adjust the blocksize */
		newHdl.play_blocksize=newHdl.play_channels*newHdl.framesize*newHdl.bps; 
	} else if (strcmp("record_channels", param)==0) {
		newHdl.rec_channels=value; 
		/* we need to adjust the blocksize */
		newHdl.rec_blocksize=newHdl.rec_channels*newHdl.framesize*newHdl.bps; 
	} else 	if (strcmp("framesize", param)==0) {
		newHdl.framesize=value; 
		/* 
		 * the user enters the framesize in samples, we have
		 * to convert it to bytes per samples, with respect to the number
		 * of channels 
		 */
		newHdl.play_blocksize=newHdl.play_channels*newHdl.framesize*newHdl.bps; 
		newHdl.rec_blocksize=newHdl.rec_channels*newHdl.framesize*newHdl.bps; 
	} else if (strcmp("persist", param)==0) {
			if (value) audioData->flags |= PAAUDIODATA_FLAG_PERSIST; else audioData->flags ^= PAAUDIODATA_FLAG_PERSIST;
	} else {
		fprintf(stderr, "audio_device_configure(): not supported configuration option\n");
	}
		
	/* now open the stream again with the new parameters */
	if ((err=Pa_OpenDefaultStream(	&(audioData->stream), 
				  	newHdl.rec_channels,	
					newHdl.play_channels,
					paFloat32, 		/* use the PortAudio sample converter from FP to Int */
					newHdl.samplerate,	
					newHdl.framesize,
					paAudioDeviceCallback, 
					audioData)) != paNoError) {

		fprintf(stderr, "audio_device_configure(): can't configure device: %s\n", Pa_GetErrorText(err));
		/* open the stream again with the old parameters */
		if ((err=Pa_OpenDefaultStream(	&(audioData->stream), 
						handle->rec_channels,	
						handle->play_channels,
						paFloat32, 		/* use the PortAudio sample converter from FP to Int */
						handle->samplerate,	
						handle->framesize,
						paAudioDeviceCallback, 
						audioData)) != paNoError) {
			/* something really wicked is going on, just exit */
			fprintf(stderr, "audio_device_configure(): can't turn back to old configuration: %s, exiting\n", Pa_GetErrorText(err));
			exit(1); 
		} else {
			return 1; 
		}
	}
	/* configure successful, write back the new configuration */
	handle->bps=newHdl.bps; 
	handle->samplerate=newHdl.samplerate; 
	handle->framesize=newHdl.framesize; 
	handle->play_channels=newHdl.play_channels; 
	handle->rec_channels=newHdl.rec_channels; 
	handle->play_blocksize=newHdl.play_blocksize; 
	handle->rec_blocksize=newHdl.rec_blocksize; 

	/* re-fresh the buffers */
	free(handle->obuf); 
	free(handle->ibuf); 

	ESWEEP_MALLOC(handle->obuf, PAAUDIODATA_BUFFERS*handle->play_blocksize, sizeof(char), 0); 
	ESWEEP_MALLOC(handle->ibuf, PAAUDIODATA_BUFFERS*handle->rec_blocksize, sizeof(char), 0); 
	for (i=0; i < PAAUDIODATA_BUFFERS; i++) {
		audioData->outBuf[i]=handle->obuf+i*handle->play_blocksize; 
		audioData->outBufMask[i]=PAAUDIODATA_BUF_FINISHED; 
		audioData->inBuf[i]=handle->ibuf+i*handle->rec_blocksize; 
		audioData->inBufMask[i]=PAAUDIODATA_BUF_FINISHED; 
	}
	
	audioData->out_channels=handle->play_channels; 
	audioData->in_channels=handle->rec_channels; 
	/* The first buffer is in the queue */
	audioData->outBufQueue=0; 
	audioData->inBufQueue=0; 

	return 0; 
}

/* wait till all pending buffers have been processed, then restart the stream */
int audio_device_sync(esweep_audio *handle) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	PaError err; 

	if (Pa_IsStreamStopped(audioData->stream)==1) return 0; 
	if ((err=Pa_StopStream(audioData->stream)) != paNoError) {
		fprintf(stderr, "audio_device_sync(): can't stop stream: %s\n", Pa_GetErrorText(err));
		return -1; 
	}
	/* make sure that the stream is marked as paused when restart is not possible */
	handle->play_pause=1; 
	handle->rec_pause=1; 
	/* mark all buffers as finished, i. e. waiting for data */
	audioData->outBufMask[0]=PAAUDIODATA_BUF_FINISHED; 
	audioData->outBufMask[1]=PAAUDIODATA_BUF_FINISHED; 
	audioData->inBufMask[0]=PAAUDIODATA_BUF_FINISHED; 
	audioData->inBufMask[1]=PAAUDIODATA_BUF_FINISHED; 
	audioData->outBufQueue=0; 
	audioData->inBufQueue=0; 
	if ((err=Pa_StartStream(audioData->stream)) != paNoError) {
		fprintf(stderr, "audio_device_sync(): can't restart stream: %s\n", Pa_GetErrorText(err));
		return -1; 
	}
	handle->play_pause=0; 
	handle->rec_pause=0; 
		
	return 0;
}

/* Pause playback after draining the buffers */
int audio_device_pause(esweep_audio *handle) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	PaError err; 

	if (Pa_IsStreamStopped(audioData->stream)==1) return 0; 
	if ((err=Pa_StopStream(audioData->stream)) != paNoError) {
		fprintf(stderr, "audio_device_sync(): can't stop stream: %s\n", Pa_GetErrorText(err));
		return -1; 
	}

	handle->play_pause=1; 
	handle->rec_pause=1; 
	return 0; 
}

/* seeking not supported */
int audio_device_seek(esweep_audio *handle, int offset) {
	return 0; 
}

/* 
 * Playback 
 * This function simply fills the output buffer. The playback itself is done by the callback
 */
int audio_device_out(esweep_audio *handle, esweep_object *out[], u_int channels, int offset) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	float *buffer; 
	PaError err; 
	
	if (Pa_IsStreamStopped(audioData->stream)==1) {
		if ((err=Pa_StartStream(audioData->stream)) != paNoError) {
			fprintf(stderr, "audio_device_out(): can't start stream: %s\n", Pa_GetErrorText(err));
			return -1; 
		}
		handle->play_pause=0; 
		handle->rec_pause=0; 
	}

	/* get the buffer where we are allowed to write to */
	for (i=audioData->outBufQueue; i < PAAUDIODATA_BUFFERS; i++) {
		if (audioData->outBufMask[i]==PAAUDIODATA_BUF_FINISHED) {
			break; 
		}
	}
	if (i == PAAUDIODATA_BUFFERS) {
		/* no buffer available */
		return offset; 
	}
	buffer=(float*) audioData->outBuf[i]; 
	audioData->outBufMask[i]=PAAUDIODATA_BUF_PENDING; 


	/* if the device has less channels than we want to play, then use only this number of channels */
	if (handle->play_channels<channels) channels=handle->play_channels; 
	
	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy audio to output buffer */
		switch (out[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) out[i]->data;
				for (j=offset; j < handle->framesize+offset && j < out[i]->size; j++) {
					buffer[i+(j-offset)*handle->play_channels]=(float) (cpx[j].real > 1.0 ? 1.0 : cpx[j].real); 
				}
				break; 
			case WAVE: 
				wave=(Wave*) out[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < out[i]->size; j++) {
					buffer[i+(j-offset)*handle->play_channels]=(float) (wave[j] > 1.0 ? 1.0 : wave[j]); 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}

	return offset+handle->framesize;
}

/* 
 * Record
 */
int audio_device_in(esweep_audio *handle, esweep_object *in[], u_int channels, int offset) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	float *buffer; 
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	PaError err; 
	
	if (Pa_IsStreamStopped(audioData->stream)==1) {
		if ((err=Pa_StartStream(audioData->stream)) != paNoError) {
			fprintf(stderr, "audio_device_in(): can't start stream: %s\n", Pa_GetErrorText(err));
			return -1; 
		}
		handle->play_pause=0; 
		handle->rec_pause=0; 
	}

	/* get the buffer where we are allowed to read from */
	for (i=audioData->inBufQueue; i < PAAUDIODATA_BUFFERS; i++) {
		if (audioData->inBufMask[i]==PAAUDIODATA_BUF_FINISHED) {
			break; 
		}
	}
	if (i == PAAUDIODATA_BUFFERS) {
		/* no buffer available */
		return offset; 
	}
	
	buffer=(float*) audioData->inBuf[i]; 
	audioData->inBufMask[i]=PAAUDIODATA_BUF_PENDING; 

	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy input buffer to output channels */
		switch (in[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) in[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < in[i]->size; j++) {
					cpx[j].real=(Real) buffer[i+(j-offset)*handle->rec_channels]; 
					cpx[j].imag=0.0; 
				}
				break; 
			case WAVE: 
				wave=(Wave*) in[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < in[i]->size; j++) {
					wave[j]=(Real) buffer[i+(j-offset)*handle->rec_channels]; 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}
	return offset+handle->framesize;
}

void audio_device_close(esweep_audio *handle) {
	paAudioData *audioData=(paAudioData*) handle->pData; 
	PaError err; 

	if ((err=Pa_StopStream(audioData->stream)) != paNoError) {
		fprintf(stderr, "audio_device_close(): can't stop stream: %s\n", Pa_GetErrorText(err));
		return; 
	}

	if ((err=Pa_Terminate()) != paNoError) {
		fprintf(stderr, "audio_device_close(): can't terminate PortAudio: %s\n", Pa_GetErrorText(err));
		return; 
	}

	free(audioData); 
	free(handle->obuf); 
	free(handle->ibuf); 
	free(handle); 
}

int paAudioDeviceCallback( const void *input,
                                      void *output,
                                      unsigned long frameCount,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData )  {

	paAudioData *audioData=(paAudioData*) userData; 
	/* copy the queued buffer */
	if (audioData->inBufMask[audioData->inBufQueue] == PAAUDIODATA_BUF_PENDING) {
		memcpy(audioData->inBuf[audioData->inBufQueue], input, audioData->in_channels*frameCount*sizeof(float)); 
		audioData->inBufMask[audioData->inBufQueue]=PAAUDIODATA_BUF_FINISHED; 
		audioData->inBufQueue=(audioData->inBufQueue + 1) % PAAUDIODATA_BUFFERS; 
	}

	if (audioData->outBufMask[audioData->outBufQueue] == PAAUDIODATA_BUF_PENDING) {
		memcpy(output, audioData->outBuf[audioData->outBufQueue], audioData->out_channels*frameCount*sizeof(float)); 
		memset(audioData->outBuf[audioData->outBufQueue], 0, audioData->out_channels*frameCount*sizeof(float));
		audioData->outBufMask[audioData->outBufQueue]=PAAUDIODATA_BUF_FINISHED; 
	} else {
		if (!(audioData->flags & PAAUDIODATA_FLAG_PERSIST)) memset(output, 0, audioData->out_channels*frameCount*sizeof(float)); 
	}
	audioData->outBufQueue=(audioData->outBufQueue + 1) % PAAUDIODATA_BUFFERS; 

	return paContinue; 
}
#endif /* PORTAUDIO */ 

