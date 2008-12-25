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

#ifdef OPENBSD


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esweep.h"
#include "sound.h"

long snd_open(const char *device, long samplerate) {
	audio_info_t info;
	long sc;
	long temp;
	
	/* initialize audio info structure */
	
	AUDIO_INITINFO(&info);
	
	/* 16 bit sample format */
	
	info.play.precision=info.record.precision=(u_int) 16;
	
	/* we want signed linear encoding in the machines native endianess */
	
	info.play.encoding=info.record.encoding=AUDIO_ENCODING_SLINEAR;
	
	/* 2 channels */
	
	info.play.channels=info.record.channels=(u_int) 2;
	
	/* the samplerate */
	
	info.play.sample_rate=info.record.sample_rate=samplerate;
	
	/* simultaneous record and play, full-duplex is set later */
	
	info.mode=AUMODE_PLAY_ALL | AUMODE_PLAY | AUMODE_RECORD;
	
	/* open audio device */
	
	if ((sc=open(device, O_RDWR, NULL))<0) {
		printf("Could not open soundcard\n");
		return -1;
	}
	
	if (ioctl(sc, AUDIO_SETINFO, &info)<0) {
		printf("Could not set audio parameters\n");
		return -1;
	}
	
	/* set full-duplex mode */
		
	temp=1;
	if (ioctl(sc, AUDIO_SETFD, &temp)<0) {
		printf("Could not set full-duplex mode\n");
		return -1;
	}
	
	/* return the handle */
	
	return sc;
}

long snd_play(short *out, long size, long handle) {
	long offset=0;
	long bytes_left=2*size; /* factor 2 because we use 16 bit = 2 Bytes encoding */
	long bytes_to_rw;
	long write_ok=1;
	audio_info_t info;
	
	if (ioctl(handle, AUDIO_GETINFO, &info)<0) {
		printf("Could not get audio parameters\n");
		return 1;
	}
	bytes_to_rw=info.blocksize;
	
	while (bytes_left>0) {
		write_ok=write(handle, &(out[offset]), bytes_to_rw);	
		if (write_ok == -1) {
			printf("I/O error\n");
			return 1;
		}
		
		bytes_left-=info.blocksize;
		offset+=info.blocksize/2;
		if (bytes_left < info.blocksize) bytes_to_rw=bytes_left;
	}
	
	return 0;
}

long snd_rec(short *in, long size, long handle) {
	
	int offset=0;
	int bytes_left=2*size; /* factor 2 because we use 16 bit = 2 Bytes encoding */
	int bytes_to_rw;
	int read_ok=1;
	audio_info_t info;
	
	if (ioctl(handle, AUDIO_GETINFO, &info)<0) {
		printf("Could not get audio parameters\n");
		return 1;
	}
	bytes_to_rw=info.blocksize;
	/* write and read bytes_to_rw bytes */
	
	while (bytes_left>0) {
		read_ok=read(handle, &(in[offset]), bytes_to_rw);
		
		if (read_ok == -1) {
			printf("I/O error\n");
			return 1;
		}
		
		bytes_left-=info.blocksize;
		offset+=info.blocksize/2;
		if (bytes_left < info.blocksize) bytes_to_rw=bytes_left;
	}
	
	return 0;
}

long snd_pnr(short *in, short *out, long size, long handle) { 
	int offset=0;
	int bytes_left=2*size; /* factor 2 because we use 16 bit = 2 Bytes encoding */
	int bytes_to_rw;
	int read_ok=1, write_ok=1;
	audio_info_t info;
	
	if (ioctl(handle, AUDIO_GETINFO, &info)<0) {
		printf("Could not get audio parameters\n");
		return 1;
	}
	bytes_to_rw=info.blocksize;
	while (bytes_left>0) {
		write_ok=write(handle, &(out[offset]), bytes_to_rw);
		read_ok=read(handle, &(in[offset]), bytes_to_rw);
		
		if ((write_ok == -1) || (read_ok == -1)) {
			printf("I/O error\n");
			return 1;
		}
		
		bytes_left-=info.blocksize;
		offset+=info.blocksize/2;
		if (bytes_left < info.blocksize) bytes_to_rw=bytes_left;
	}
	
	return 0;
}

void snd_close(long handle) {
	ioctl(handle, AUDIO_DRAIN, NULL);
	ioctl(handle, AUDIO_FLUSH, NULL);
	close(handle);
}

#endif /* OPENBSD */ 

