/*
 * Copyright (c) 2007, 2009 Jochen Fabricius <jfab@berlios.de>
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

/* Functions for controlling audio I/O. Not directly called from esweep. 
 * This is the OpenBSD dependant part. */


#ifdef OPENBSD

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esweep_priv.h"
#include "audio_device.h"

esweep_audio *audio_device_open(const char *device) {
	audio_info_t info;
	int au_hdl;
	int temp;
	esweep_audio *handle; 
	u_int prec=16; // currently only 16 bits per sample are supported

	
	/* initialize audio info structure */
	
	AUDIO_INITINFO(&info);
	
	/* sample format */
	
	info.play.precision=info.record.precision=prec;
	
	/* we want signed linear encoding in the machines native endianess */
	
	info.play.encoding=info.record.encoding=AUDIO_ENCODING_SLINEAR;

	/* 2 channels */
	info.play.channels=info.record.channels=2; 
	
	/* record and playback must have the same samplerate */
	info.record.sample_rate=info.play.sample_rate; 
	
	/* simultaneous record and play, full-duplex is set later */
	
	info.mode=AUMODE_PLAY | AUMODE_RECORD | AUMODE_PLAY_ALL; 
	
	/* pause record and play */
	info.play.pause=1; 
	info.record.pause=1; 

	/* open audio device */
	
	if ((au_hdl=open(device, O_RDWR, NULL))<0) {
		fprintf(stderr, "audio_device_open(): can't open soundcard\n");
		return NULL;
	}

	/* Set parameters */
	
	if (ioctl(au_hdl, AUDIO_SETINFO, &info)<0) {
		fprintf(stderr, "audio_device_open(): can't set audio parameters\n");
		close(au_hdl); 
		return NULL;
	}

	/* Check if the parameters are set */

	if (ioctl(au_hdl, AUDIO_GETINFO, &info)<0) {
		fprintf(stderr, "audio_device_open(): can't get audio parameters\n");
		close(au_hdl); 
		return NULL;
	}

	if (info.play.precision!=prec || info.record.precision!=prec) {
		fprintf(stderr, "audio_device_open(): can't set audio encoding precision to 16 bits/sample\n");
		close(au_hdl); 
		return NULL;
	}

	if ((info.play.encoding != AUDIO_ENCODING_SLINEAR_LE && \
			info.play.encoding != AUDIO_ENCODING_SLINEAR_BE && \
			info.play.encoding != AUDIO_ENCODING_SLINEAR) || \
			info.play.encoding != info.record.encoding) {
			fprintf(stderr, "audio_device_open(): can't set audio encoding to signed linear\n");
			fprintf(stderr, "audio_device_open(): current encodings are: info.play.encoding=%u info.record.encoding=%u\n", info.play.encoding, info.record.encoding);
		close(au_hdl); 
		return NULL;
	}

	if (info.play.sample_rate != info.record.sample_rate) {
		fprintf(stderr, "audio_device_open(): can't set identical play and record samplerates\n");
		close(au_hdl); 
		return NULL;
	}

	/* set full-duplex mode */
		
	temp=1;
	if (ioctl(au_hdl, AUDIO_SETFD, &temp)<0) {
		fprintf(stderr, "audio_device_open(): can't set full-duplex mode\n");
		close(au_hdl); 
		return NULL;
	}
	
       
	ESWEEP_MALLOC(handle, 1, sizeof(esweep_audio), NULL); 

	handle->au_hdl=au_hdl; 
	handle->samplerate=info.play.sample_rate; 
	handle->play_channels=info.play.channels; 
	handle->rec_channels=info.record.channels; 
	handle->precision=info.play.precision; 
	handle->bps=info.play.bps; 
	handle->msb=info.play.msb; 
	handle->play_blocksize=handle->rec_blocksize=info.play.block_size; 
	handle->framesize=handle->play_blocksize/(handle->play_channels*handle->bps); 
	handle->play_pause=1; 
	handle->rec_pause=1; 

	/* set the callback functions */
	handle->audio_query=(audio_query_ptr) &audio_device_query; 
	handle->audio_configure=(audio_configure_ptr) &audio_device_configure; 
	handle->audio_sync=(audio_sync_ptr) &audio_device_sync; 
	handle->audio_pause=(audio_pause_ptr) &audio_device_pause; 
	handle->audio_seek=(audio_seek_ptr) &audio_device_seek; 
	handle->audio_out=(audio_out_ptr) &audio_device_out; 
	handle->audio_in=(audio_in_ptr) &audio_device_in; 
	handle->audio_close=(audio_close_ptr) &audio_device_close; 

	ESWEEP_MALLOC(handle->ibuf, handle->play_blocksize, sizeof(char), NULL); 
	ESWEEP_MALLOC(handle->obuf, handle->rec_blocksize, sizeof(char), NULL); 

	/* We use a high pass dither generator
	 * (see "Theory of non-subtractive dither", by Wannamker, Lipshitz and Vanderkooy)
	 */
	ESWEEP_MALLOC(handle->dither, handle->play_channels, sizeof(Real), NULL); 
	srandom(1); // no need for a good seed
	handle->dither[0]=handle->dither[1]=2*(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF

	/* return the handle */
	return handle;
}

int audio_device_query(const esweep_audio *handle, const char *query, int *result) {
	audio_info_t info;

	if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
		fprintf(stderr, "Could not get audio parameters\n");
		return -1;
	}

	if (strcmp("samplerate", query)==0) {
		*result=handle->samplerate; 
		return 0; 
	}
	if (strcmp("encoding", query)==0) {
		*result=info.play.encoding; 
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
	return 0; 
}

int audio_device_configure(esweep_audio *handle, const char *param, int value) {
	audio_info_t info;
	u_int rec_blocksize, play_blocksize; 
	Real *dither; 
	u_int i; 

	/* Get the current audio parameters */
	if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
		fprintf(stderr, "audio_device_configure(): could not get audio parameters\n");
		return -1;
	}

	/* Before configure, we need to pause playback and record */
	info.play.pause=1;
	info.record.pause=1;
	if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
		fprintf(stderr, "audio_device_configure(): could not pause audio I/O\n");
		return -1;
	}
	handle->play_pause=1; 
	handle->rec_pause=1; 

	if (strcmp("samplerate", param)==0) {
		info.play.sample_rate=info.record.sample_rate=value; 
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_configure(): Could not set audio parameters\n");
			return -1;
		}
		ioctl(handle->au_hdl, AUDIO_GETINFO, &info); 
		if (info.play.sample_rate!=value || info.record.sample_rate!=value) {
			fprintf(stderr, "audio_device_configure(): Samplerate %i Hz not supported\n", value);
			return -1;
		}
		handle->samplerate=value; 
	}
	if (strcmp("play_channels", param)==0) {
		info.play.channels=value; 
		/*
		 * Because we might set the audio framesize manually, it may have become sticky. 
		 * So we need to set it according to our desired latency. 
		 */
		info.play.block_size=handle->framesize*value*handle->bps; 
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_configure(): Could not set audio parameters\n");
			return -1;
		}
		ioctl(handle->au_hdl, AUDIO_GETINFO, &info); 
		if (info.play.channels!=value) {
			fprintf(stderr, "audio_device_configure(): %i playback channels not supported\n", value);
			return -1;
		}
		/* Create new dither */
		ESWEEP_MALLOC(dither, value, sizeof(Real), -1); 
		for (i=0; i < handle->play_channels; i++) {
			dither[i]=handle->dither[i]; 
		}
		free(handle->dither); 
		for (;i < value; i++) {
			dither[i]=dither[0]; 
		}
		handle->dither=dither; 
		handle->play_channels=value;
		handle->play_blocksize=info.play.block_size; 
		/* Adjust the audio buffer */
		free(handle->obuf); 
		ESWEEP_MALLOC(handle->obuf, handle->play_blocksize, sizeof(char), -1); 
	}
	if (strcmp("record_channels", param)==0) {
		info.record.channels=value; 
		/*
		 * Because we might set the audio framesize manually, it may have become sticky. 
		 * So we need to set it according to our desired latency. 
		 */
		info.record.block_size=handle->framesize*value*handle->bps; 
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_configure(): Could not set audio parameters\n");
			return -1;
		}
		ioctl(handle->au_hdl, AUDIO_GETINFO, &info); 
		if (info.record.channels != value) {
			fprintf(stderr, "audio_device_configure(): %i record channels not supported\n", value);
			return -1;
		}
		handle->rec_channels=value;
		handle->rec_blocksize=info.record.block_size; 
		/* Adjust the audio buffer */
		free(handle->ibuf); 
		ESWEEP_MALLOC(handle->ibuf, handle->rec_blocksize, sizeof(char), -1); 
	}

	if (strcmp("framesize", param)==0) {
		/* 
		 * the user enters the framesize in samples, we have
		 * to convert it to bytes per samples, with respect to the number
		 * of channels 
		 */
		rec_blocksize=info.record.block_size=handle->bps*value*handle->rec_channels; 	
		play_blocksize=info.play.block_size=handle->bps*value*handle->play_channels; 	

		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_configure(): Could not set audio parameters\n");
			return -1;
		}
		ioctl(handle->au_hdl, AUDIO_GETINFO, &info); 
		if (info.play.block_size != play_blocksize) {
			fprintf(stderr, "audio_device_configure(): playback blocksize %i not supported\n", value);
			play_blocksize=info.play.block_size/(handle->play_channels*handle->bps); 
			fprintf(stderr, "audio_device_configure(): playback blocksize set to: %u samples\n", play_blocksize); 
			return -1;
		}
		if (info.record.block_size != rec_blocksize) {
			fprintf(stderr, "audio_device_configure(): record blocksize %i not supported\n", value);
			rec_blocksize=info.play.block_size/(handle->rec_channels*handle->bps); 
			fprintf(stderr, "audio_device_configure(): record blocksize set to: %u samples\n", rec_blocksize); 
			return -1;
		}
		handle->play_blocksize=play_blocksize; 
		handle->rec_blocksize=rec_blocksize; 
		handle->framesize=handle->play_blocksize/(handle->play_channels*handle->bps); 
		free(handle->ibuf); 
		free(handle->obuf); 
		ESWEEP_MALLOC(handle->ibuf, handle->rec_blocksize, sizeof(char), -1); 
		ESWEEP_MALLOC(handle->obuf, handle->play_blocksize, sizeof(char), -1); 
	}
	
	return 0; 
}

/* after opening the device, this function fills the output buffer with silence till info.lowat */
int audio_device_sync(esweep_audio *handle) {
	audio_info_t info;
	u_int i; 
	ssize_t write_ok=0;
	ssize_t read_ok=0;

	if (handle->play_pause!=0 || handle->rec_pause!=0) {
		if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
			fprintf(stderr, "audio_device_sync(): could not get audio parameters\n");
			return -1;
		}

		info.play.pause=0;
		info.record.pause=0;
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_sync(): could not unpause audio I/O\n");
			return -1;
		}
		handle->play_pause=0; 
		handle->rec_pause=0; 
	}

	memset(handle->obuf, 0, handle->play_blocksize); 
	memset(handle->ibuf, 0, handle->rec_blocksize); 
	for (i=0; i < info.lowat; i++) {
		/* write the buffer to the device */
		write_ok=write(handle->au_hdl, handle->obuf, handle->play_blocksize);	
		read_ok=read(handle->au_hdl, handle->ibuf, handle->rec_blocksize);	
		if (write_ok < 0 || read_ok < 0) {
			fprintf(stderr, "audio_device_sync(): I/O error\n");
			return -1;
		}
	}
	return 0;
}

/* Pause playback after draining the buffers */
int audio_device_pause(esweep_audio *handle) {
	audio_info_t info;
	if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
		fprintf(stderr, "audio_device_pause(): could not get audio parameters\n");
		return -1;
	}

	info.play.pause=1;
	info.record.pause=1;
	if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
		fprintf(stderr, "audio_device_pause(): could not pause audio I/O\n");
		return -1;
	}
	handle->play_pause=1; 
	handle->rec_pause=1; 
	return 0; 
}

/* seeking not possible */
int audio_device_seek(esweep_audio *handle, int offset) {
	return 0; 
}

/* 
 * Playback 
 */
int audio_device_out(esweep_audio *handle, esweep_object *out[], u_int channels, int offset) {
	ssize_t write_ok=0;
	audio_info_t info;
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	Real coeff; 
	short *buffer; 
	Real dither; 
	
 	if (handle->play_pause!=0) {
		if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
			fprintf(stderr, "audio_device_out(): could not get audio parameters\n");
			return -1;
		}

		info.play.pause=0;
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_out(): not unpause audio I/O\n");
			return -1;
		}
		handle->play_pause=0; 
	}

	/* set coefficient */
	coeff=(Real) (1 << (handle->precision-1)); 
	buffer=(short*) handle->obuf; 
	memset(buffer, 0, handle->play_blocksize); 

	/* if the device has less channels than we want to play, then use only this number of channels */
	if (handle->play_channels<channels) channels=handle->play_channels; 
	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy audio to output buffer */
		switch (out[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) out[i]->data;
				for (j=offset; j < handle->framesize+offset && j < out[i]->size; j++) {
					dither=(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF with 1 LSB pp
					cpx[j].real+=(dither+handle->dither[i])/coeff; // TPDF with 2 LSB pp
					handle->dither[i]=dither; 
					buffer[i+(j-offset)*handle->play_channels]=(short) ((cpx[j].real > 1.0 ? 1.0 : cpx[j].real)*coeff); 
				}
				break; 
			case WAVE: 
				wave=(Wave*) out[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < out[i]->size; j++) {
					dither=(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF with 1 LSB pp
					wave[j]+=(dither+handle->dither[i])/coeff; // TPDF with 2 LSB pp
					handle->dither[i]=dither; 
					buffer[i+(j-offset)*handle->play_channels]=(short) ((wave[j] > 1.0 ? 1.0 : wave[j])*coeff); 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}
	/* write the buffer to the device */
	write_ok=write(handle->au_hdl, buffer, handle->play_blocksize);	
	if (write_ok == -1) {
		fprintf(stderr, "audio_device_out(): I/O error\n");
		return -1;
	}
	return offset+handle->framesize;
}

/* 
 * Record
 */
int audio_device_in(esweep_audio *handle, esweep_object *in[], u_int channels, int offset) {
	ssize_t read_ok=0;
	short *buffer; 
	audio_info_t info;
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	Real coeff; 
	
 	if (handle->rec_pause!=0) {
		if (ioctl(handle->au_hdl, AUDIO_GETINFO, &info)<0) {
			fprintf(stderr, "audio_device_in(): could not get audio parameters\n");
			return -1;
		}

		info.record.pause=0;
		if (ioctl(handle->au_hdl, AUDIO_SETINFO, &info)<0) {
			fprintf(stderr, "audio_device_in(): could not unpause audio I/O\n");
			return -1;
		}
		handle->rec_pause=0; 
	}

	/* set coefficient */
	coeff=(Real) (1 << (handle->precision-1)); 
	buffer=(short*) handle->ibuf; 
	if (handle->rec_channels<channels) channels=handle->rec_channels; 
	
	/* read from the device into the buffer */
	read_ok=read(handle->au_hdl, buffer, handle->rec_blocksize);	
	if (read_ok == -1) {
		fprintf(stderr, "audio_device_in(): I/O error\n");
		return -1;
	}

	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy input buffer to output channels */
		switch (in[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) in[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < in[i]->size; j++) {
					cpx[j].real=buffer[i+(j-offset)*handle->rec_channels]/coeff; 
					cpx[j].imag=0.0; 
				}
				break; 
			case WAVE: 
				wave=(Wave*) in[i]->data; 
				for (j=offset; j < handle->framesize+offset && j < in[i]->size; j++) {
					wave[j]=buffer[i+(j-offset)*handle->rec_channels]/coeff; 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}
	return offset+handle->framesize;
}

void audio_device_close(esweep_audio *handle) {
	//ioctl(handle->au_hdl, AUDIO_DRAIN, NULL);
	//ioctl(handle->au_hdl, AUDIO_FLUSH, NULL);
	close(handle->au_hdl);
	free(handle->dither); 
	free(handle->obuf); 
	free(handle->ibuf); 
	free(handle); 
}

#endif /* OPENBSD */ 

