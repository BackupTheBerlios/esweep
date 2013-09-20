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
 * src/audio_file.c
 */

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esweep_priv.h"
#include "audio_file.h"

#define AUDIO_FILE_WAV 0
#define AUDIO_FILE_FLAC 1

static int createWaveHeader(int fd, esweep_audio* au_hdl); 
static int readRIFFInfo(int fd, esweep_audio* au_hdl); 
static int readFLACInfo(int fd, esweep_audio* au_hdl); 

static int close_on_error(int fd, esweep_audio* au_hdl); 

/* size of header w/o 1st 4 bytes and file size, but incl. data chunk header*/
#define WAVE_HEADER_SIZE 36

esweep_audio *audio_file_open(const char *path) {
	int fd; // file descriptor
	esweep_audio *au_hdl; 
	char fourcc[5]; 
	
	/* open file with mode 644 (u+rw, go+r) */
	if ((fd=open(path, O_RDWR | O_CREAT, 0644))<0) {
		fprintf(stderr, "audio_file_open(): can't open file\n");
		return NULL;
	}

	ESWEEP_MALLOC(au_hdl, 1, sizeof(esweep_audio), NULL); 
	au_hdl->au_hdl=fd; 

	/* create defaults */
	au_hdl->file_size=WAVE_HEADER_SIZE; 
	au_hdl->data_size=0; 
	au_hdl->data_block_start=WAVE_HEADER_SIZE+4; 
	au_hdl->play_channels=au_hdl->rec_channels=2; 
	au_hdl->samplerate=44100; 
	au_hdl->precision=16; 
	au_hdl->bps=au_hdl->precision / 8 + (au_hdl->precision % 8 > 0 ? 1 : 0); // bytes / sample
	au_hdl->framesize=1024; 
	au_hdl->play_blocksize=au_hdl->rec_blocksize=au_hdl->framesize*au_hdl->play_channels*au_hdl->bps; 
	au_hdl->play_pause=0; 
	au_hdl->rec_pause=0; 
	/* read the first 4 bytes */
	if (read(fd, fourcc, 4) < 4) {
		/* new file. We will create a WAVE-file ourselves */
		ESWEEP_ASSERT(createWaveHeader(fd, au_hdl) == ERR_OK, NULL);
	} else {
		/* is it RIFF? */
		if (strcmp(fourcc, "RIFF") == 0) {
			ESWEEP_ASSERT(readRIFFInfo(fd, au_hdl) == ERR_OK, NULL); 
		} else if (strcmp(fourcc, "fLaC")) {
				readFLACInfo(fd, au_hdl);
		} else {
			fprintf(stderr, "audio_file_open(): unsupported file format: %s\n", fourcc);
			close(fd); 
			free(au_hdl); 
			return NULL;
		}
	}

	
	/* set the callback functions */
	au_hdl->audio_query=(audio_query_ptr) &audio_file_query; 
	au_hdl->audio_configure=(audio_configure_ptr) &audio_file_configure; 
	au_hdl->audio_sync=(audio_sync_ptr) &audio_file_sync; 
	au_hdl->audio_pause=(audio_pause_ptr) &audio_file_pause; 
	au_hdl->audio_out=(audio_out_ptr) &audio_file_out; 
	au_hdl->audio_in=(audio_in_ptr) &audio_file_in; 
	au_hdl->audio_close=(audio_close_ptr) &audio_file_close; 

	ESWEEP_MALLOC(au_hdl->ibuf, au_hdl->play_blocksize, sizeof(char), NULL); 
	ESWEEP_MALLOC(au_hdl->obuf, au_hdl->rec_blocksize, sizeof(char), NULL); 

	/* We use a high pass dither generator
	 * (see "Theory of non-subtractive dither", by Wannamker, Lipshitz and Vanderkooy)
	 */
	ESWEEP_MALLOC(au_hdl->dither, au_hdl->play_channels, sizeof(Real), NULL); 
	srandom(1); // no need for a good seed
	au_hdl->dither[0]=au_hdl->dither[1]=2*(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF

	/* return the handle */
	return au_hdl;
}

static int readFLACInfo(int fd, esweep_audio* au_hdl) {
	return 0; 
}

#define WAVE_FORMAT_PCM	0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003	
#define WAVE_FORMAT_ALAW 0x0006	
#define WAVE_FORMAT_MULAW 0x0007	
#define WAVE_FORMAT_EXTENSIBL 0xFFFE	

static int readRIFFInfo(int fd, esweep_audio* au_hdl) {
	u_int32_t u32; 
	u_int16_t u16; 
	u_int16_t block_align; 
	u_int32_t header_size; 
	char fourcc[4]; 

	/* in the following read events, if an error occurs, the file is not 
	 * correct, so bail out */

	/* file size */
	ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	au_hdl->file_size=letoh32(u32); 
	ESWEEP_ASSERT(au_hdl->file_size >= WAVE_HEADER_SIZE, close_on_error(fd, au_hdl)); 

	/* is it WAVE */
	ESWEEP_ASSERT(read(fd, fourcc, 4) == 4, close_on_error(fd, au_hdl)); 
	if (strcmp(fourcc, "WAVE") > 0) {
		fprintf(stderr, "audio_file_open(): unsupported file format: %s\n", fourcc);
		return close_on_error(fd, au_hdl); 
	}

	/* fmt chunk */
	ESWEEP_ASSERT(read(fd, fourcc, 4) == 4, close_on_error(fd, au_hdl)); 

	/* length of fmt data */
	ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	header_size=letoh32(u32); 
	/* the fmt chunk size may be 16 (standard), 18 (extension size 0), or 40 (max. size of extension) */
	if (header_size != 16 && header_size != 18 && header_size != 40) {
		/* something's wrong */
		fprintf(stderr, "FMT chunk size not allowed\n"); 
		return close_on_error(fd, au_hdl); 
	}
	au_hdl->data_block_start=header_size+16; 

	/* format tag, we allow only PCM at the moment */
	ESWEEP_ASSERT(read(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 
	u16=letoh16(u16); 
	if (u16 !=  WAVE_FORMAT_PCM) {
		fprintf(stderr, "Sample format not supported\n"); 
		return close_on_error(fd, au_hdl); 
	}
	/* number of channels */
	ESWEEP_ASSERT(read(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 
	au_hdl->play_channels=au_hdl->rec_channels=letoh16(u16); 
	if (au_hdl->play_channels < 1) {
		fprintf(stderr, "Unsupported number of channels\n"); 
		return close_on_error(fd, au_hdl); 
	}
	/* samplerate */
	ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	au_hdl->samplerate=letoh32(u32); 
	if (au_hdl->play_channels < 1) {
		fprintf(stderr, "Unsupported samplerate\n"); 
		return close_on_error(fd, au_hdl); 
	}
	/* bytes per second */
	ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	au_hdl->bps=letoh32(u32)/(au_hdl->play_channels * au_hdl->samplerate); 
	if (au_hdl->bps < 1) {
		fprintf(stderr, "Bytes per second not meaningful\n"); 
		return close_on_error(fd, au_hdl); 
	}

	/* block align */
	ESWEEP_ASSERT(read(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 
	block_align=letoh16(u16); 
	if (block_align < 1) {
		fprintf(stderr, "Bytes per sample not meaningful\n"); 
		return close_on_error(fd, au_hdl); 
	}

	/* bits/sample */
	ESWEEP_ASSERT(read(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 
	au_hdl->precision=letoh16(u16); 
	if (au_hdl->precision < 8 && au_hdl->precision > 32) {
		fprintf(stderr, "Precision not supported\n"); 
		return close_on_error(fd, au_hdl); 
	}

	/* we should check for consistency */
	if ((au_hdl->precision / 8 + (au_hdl->precision % 8 > 0 ? 1 : 0)) != au_hdl->bps) {
		fprintf(stderr, "Metadata not consistent\n"); 
		return close_on_error(fd, au_hdl); 
	}

	/* we have read 16 bytes, seek to the next chunk */
	ESWEEP_ASSERT(lseek(fd, header_size-16, SEEK_CUR) > 0, close_on_error(fd, au_hdl)); 

	/* skip additional chunks */
	ESWEEP_ASSERT(read(fd, fourcc, 4) == 4, close_on_error(fd, au_hdl)); 
	while (strcmp(fourcc, "data") > 0) {
		ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
		ESWEEP_ASSERT(lseek(fd, letoh32(u32), SEEK_CUR) > 0, close_on_error(fd, au_hdl)); 
		au_hdl->data_block_start+=letoh32(u32); 
		ESWEEP_ASSERT(read(fd, fourcc, 4) == 4, close_on_error(fd, au_hdl)); 
	}

	/* data chunk reached */
	ESWEEP_ASSERT(read(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	au_hdl->data_size=letoh32(u32); 

	/* adust the blocksize */
	au_hdl->play_blocksize=au_hdl->rec_blocksize=au_hdl->framesize*au_hdl->play_channels*au_hdl->bps; 

	return ERR_OK; 
}

static int createWaveHeader(int fd, esweep_audio* au_hdl) {
	u_int32_t u32; 
	u_int16_t u16; 

	/* seek to beginning of file */
	ESWEEP_ASSERT(lseek(fd, 0, SEEK_SET) == 0, close_on_error(fd, au_hdl)); 

	/* fourcc */
	ESWEEP_ASSERT(write(fd, "RIFF", 4) == 4, close_on_error(fd, au_hdl)); 

	/* file size */
	u32=htole32(au_hdl->file_size); 
	ESWEEP_ASSERT(write(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 

	/* file type */
	ESWEEP_ASSERT(write(fd, "WAVE", 4) == 4, close_on_error(fd, au_hdl)); 

	/* fmt chunk */
	ESWEEP_ASSERT(write(fd, "fmt ", 4) == 4, close_on_error(fd, au_hdl)); 

	/* length of fmt data */
	u32=htole32((u_int32_t) 16); 
	ESWEEP_ASSERT(write(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 

	/* format tag: 1 (PCM) */
	u16=htole16((u_int16_t) 1); 
	ESWEEP_ASSERT(write(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 

	/* channels */
	u16=htole16((u_int16_t) au_hdl->play_channels); 
	ESWEEP_ASSERT(write(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 

	/* samplerate */
	u32=htole32((u_int32_t) au_hdl->samplerate); 
	ESWEEP_ASSERT(write(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 

	/* bytes per second */
	u32=htole32((u_int32_t) (au_hdl->bps*au_hdl->play_channels*au_hdl->samplerate)); 
	ESWEEP_ASSERT(write(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 

	/* block align */
	u16=htole16((u_int16_t) (au_hdl->play_channels*au_hdl->bps)); 
	ESWEEP_ASSERT(write(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 

	/* bits per sample */
	u16=htole16((u_int16_t) (8*au_hdl->bps)); 
	ESWEEP_ASSERT(write(fd, &u16, sizeof(u_int16_t)) == sizeof(u_int16_t), close_on_error(fd, au_hdl)); 

	/* data chunk, now empty */
	ESWEEP_ASSERT(write(fd, "data", 4) == 4, close_on_error(fd, au_hdl)); 

	/* size of data chunk */
	u32=htole32((u_int32_t) au_hdl->data_size); 
	ESWEEP_ASSERT(write(fd, &u32, sizeof(u_int32_t)) == sizeof(u_int32_t), close_on_error(fd, au_hdl)); 
	
	return ERR_OK; 
}

static int close_on_error(int fd, esweep_audio* au_hdl) {
	close(fd); 
	free(au_hdl); 
	return ERR_UNKNOWN; 
}

int audio_file_query(const esweep_audio *au_hdl, const char *query, int *result) {
	if (strcmp("samplerate", query)==0) {
		*result=au_hdl->samplerate; 
		return 0; 
	}
	if (strcmp("channels", query)==0) {
		*result=au_hdl->play_channels; 
		return 0; 
	}
	if (strcmp("precision", query)==0) {
		*result=au_hdl->precision; 
		return 0; 
	}
	if (strcmp("framesize", query)==0) {
		*result=au_hdl->framesize; 
		return 0; 
	}
	if (strcmp("size", query)==0) {
		*result=au_hdl->data_size/(au_hdl->bps*au_hdl->rec_channels); 
		return 0; 
	}
	return 0; 
}

int audio_file_configure(esweep_audio *au_hdl, const char *param, int value) {
	Real *dither; 
	u_int i; 

	if (au_hdl->data_size > 0) {
		fprintf(stderr, "audio_file_configure(): non-empty file, configuration not allowed\n");
		return -1;
	}

	if (strcmp("samplerate", param)==0) {
		ESWEEP_ASSERT(value > 0, ERR_BAD_PARAMETER); 
		au_hdl->samplerate=value; 
	}
	if (strcmp("channels", param)==0) {
		ESWEEP_ASSERT(value > 0, ERR_BAD_PARAMETER); 
		au_hdl->play_channels=au_hdl->rec_channels=value; 

		au_hdl->play_blocksize=au_hdl->rec_blocksize= \
			au_hdl->framesize*au_hdl->play_channels*au_hdl->bps;
		/* Adjust the audio buffer */
		free(au_hdl->obuf); 
		free(au_hdl->ibuf); 
		ESWEEP_MALLOC(au_hdl->obuf, au_hdl->play_blocksize, sizeof(char), -1); 
		ESWEEP_MALLOC(au_hdl->ibuf, au_hdl->rec_blocksize, sizeof(char), -1); 

		/* Create new dither */
		ESWEEP_MALLOC(dither, value, sizeof(Real), -1); 
		for (i=0; i < au_hdl->play_channels; i++) {
			dither[i]=au_hdl->dither[i]; 
		}
		free(au_hdl->dither); 
		for (;i < value; i++) {
			dither[i]=dither[0]; 
		}
		au_hdl->dither=dither; 
		au_hdl->play_channels=value;
	}

	if (strcmp("framesize", param)==0) {
		ESWEEP_ASSERT(value > 0, ERR_BAD_PARAMETER); 
		/* 
		 * the user enters the framesize in samples, we have
		 * to convert it to bytes per samples, with respect to the number
		 * of channels 
		 */
		au_hdl->rec_blocksize=au_hdl->bps*value*au_hdl->rec_channels; 	
		au_hdl->play_blocksize=au_hdl->bps*value*au_hdl->play_channels; 	

		au_hdl->framesize=value; 
		free(au_hdl->ibuf); 
		free(au_hdl->obuf); 
		ESWEEP_MALLOC(au_hdl->ibuf, au_hdl->rec_blocksize, sizeof(char), -1); 
		ESWEEP_MALLOC(au_hdl->obuf, au_hdl->play_blocksize, sizeof(char), -1); 
	}

	/* write the new configuration to the file */
	ESWEEP_ASSERT(createWaveHeader(au_hdl->au_hdl, au_hdl) == ERR_OK, -1);
	
	return 0; 
}

int audio_file_sync(esweep_audio *handle) {
	memset(handle->obuf, 0, handle->play_blocksize); 
	memset(handle->ibuf, 0, handle->rec_blocksize); 
	handle->play_pause=0; 
	handle->rec_pause=0; 
	return 0;
}

int audio_file_pause(esweep_audio *handle) {
	handle->play_pause=1; 
	handle->rec_pause=1; 
	return 0; 
}

/* seek to offset */
int audio_file_seek(esweep_audio *handle, int offset) {
	return 0; 
}

/* 
 * Write to file 
 */
int audio_file_out(esweep_audio *au_hdl, esweep_object *out[], u_int channels, int offset) {
	ssize_t write_ok=0;
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	Real coeff; 
	Real dither; 
	Real out_real; 
	u_int16_t out16; 
	u_int16_t *buffer; 

	/* set coefficient */
	coeff=(Real) (1 << (au_hdl->precision-1)); 
	buffer=(u_int16_t*) au_hdl->obuf; 
	memset(buffer, 0, au_hdl->play_blocksize); 

	/* if the device has less channels than we want to play, then use only this number of channels */
	if (au_hdl->play_channels < channels) channels=au_hdl->play_channels; 
	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy audio to output buffer */
		switch (out[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) out[i]->data;
				for (j=offset; j < au_hdl->framesize+offset && j < out[i]->size; j++) {
					dither=(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF with 1 LSB pp
					out_real=cpx[j].real+(dither+au_hdl->dither[i])/coeff; // TPDF with 2 LSB pp
					au_hdl->dither[i]=dither; 
					/* clipping */
					out_real=out_real > 1.0 ? 1.0 : out_real; 
					out_real=out_real < -1.0 ? -1.0 : out_real; 
					out16=(int16_t) (out_real*coeff); 
					buffer[i+(j-offset)*au_hdl->play_channels]=htole16((u_int16_t) out16); 
				}
				break; 
			case WAVE: 
				wave=(Wave*) out[i]->data; 
				for (j=offset; j < au_hdl->framesize+offset && j < out[i]->size; j++) {
					dither=(random() - (1.0+RAND_MAX)/2)/(1.0+RAND_MAX); // RPDF with 1 LSB pp
					out_real=wave[j]+(dither+au_hdl->dither[i])/coeff; // TPDF with 2 LSB pp
					au_hdl->dither[i]=dither; 
					/* clipping */
					out_real=out_real > 1.0 ? 1.0 : out_real; 
					out_real=out_real < -1.0 ? -1.0 : out_real; 
					out16=(int16_t) (out_real*coeff); 
					buffer[i+(j-offset)*au_hdl->play_channels]=htole16((u_int16_t) out16); 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}
	/* write the buffer to the device */
	write_ok=write(au_hdl->au_hdl, buffer, au_hdl->play_blocksize);	
	if (write_ok == -1) {
		fprintf(stderr, "audio_file_out(): I/O error\n");
		return -1;
	}
	au_hdl->data_size+=au_hdl->play_blocksize; 
	au_hdl->file_size+=au_hdl->play_blocksize; 
	return offset+au_hdl->framesize;
}

/* 
 * Record
 */
int audio_file_in(esweep_audio *au_hdl, esweep_object *in[], u_int channels, int offset) {
	ssize_t read_ok=0;
	Wave *wave; 
	Complex *cpx; 
	int i, j;
	Real coeff; 
	u_int16_t in16; 
	u_int16_t *buffer; 
	
	/* set coefficient */
	coeff=(Real) (1 << (au_hdl->precision-1)); 
	buffer=(u_int16_t*) au_hdl->ibuf; 

	/* if the device has less channels than we want to record, then use only this number of channels */
	if (au_hdl->rec_channels<channels) channels=au_hdl->rec_channels; 
	
	/* read from the device into the buffer */
	read_ok=read(au_hdl->au_hdl, buffer, au_hdl->rec_blocksize);	
	if (read_ok == 0) {
		fprintf(stderr, "audio_file_in(): end-of-file\n");
		return au_hdl->data_size; 
	}
	if (read_ok == -1) {
		fprintf(stderr, "audio_file_in(): I/O error\n");
		return -1;
	}

	/* for every channel */
	for (i=0; i<channels; i++) {
		/* copy input buffer to output channels */
		switch (in[i]->type) {
			case COMPLEX: 
				cpx=(Complex*) in[i]->data; 
				for (j=offset; j < au_hdl->framesize+offset && j < in[i]->size; j++) {
					in16=letoh16(buffer[i+(j-offset)*au_hdl->rec_channels]); 
					cpx[j].real=((int16_t) in16)/coeff; 
					cpx[j].imag=0.0; 
				}
				break; 
			case WAVE: 
				wave=(Wave*) in[i]->data; 
				for (j=offset; j < au_hdl->framesize+offset && j < in[i]->size; j++) {
					in16=letoh16(buffer[i+(j-offset)*au_hdl->rec_channels]);
					wave[j]=((int16_t) in16)/coeff; 
				}
				break; 
			default: /* do nothing */
				break; 
		}
	}
	return offset+au_hdl->framesize;
}

void audio_file_close(esweep_audio *au_hdl) {
	/* write the header */
	createWaveHeader(au_hdl->au_hdl, au_hdl); 
	close(au_hdl->au_hdl);
	free(au_hdl->dither); 
	free(au_hdl->obuf); 
	free(au_hdl->ibuf); 
	free(au_hdl); 
}
