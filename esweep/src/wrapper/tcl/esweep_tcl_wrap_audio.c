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
 * esweep_tcl_wrap_audio.c
 * Wraps the esweep_audio.c source file
 * 03.10.2011, jfab:	initial creation
 */

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <tcl.h>
#include "esweep_tcl_wrap.h"

int esweepAudioOpen(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	char *device=NULL; 
	esweep_audio *handle=NULL;
	TclEsweepAudio *ea; 
	Tcl_Obj *ret; 

	const char *opts[] = {"-device", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {devIdx, sizeIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-device value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case devIdx: 
				if ((device=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -device invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((handle=esweep_audioOpen(device)) != NULL); 

	ESWEEP_MALLOC(ea, 1, sizeof(TclEsweepAudio), TCL_ERROR);
	ea->refCount=1; 
	ea->handle=handle; 
	ret=Tcl_NewObj(); 
	ret->internalRep.otherValuePtr=ea; 
	ret->typePtr = (Tcl_ObjType*) &tclEsweepAudioType; 
	/* The objects string representation is simply the pointer address */
	ret->bytes=Tcl_Alloc(256);
	snprintf(ret->bytes, 256, "%p", handle); 
	Tcl_SetObjResult(interp, ret);
	return TCL_OK;
}

int esweepAudioQuery(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	const char *opts[] = {"-handle", "-param", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {handleIdx, paramIdx};
	int obji;
	int index; 
	int result=-1; 
	char *param=NULL; 

	CHECK_NUM_ARGS(objc == 5, "-handle esweepAudioHandle -param value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				break;
			case paramIdx: 
				if ((param=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -query invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioQuery(handle, param, &result) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewIntObj(result)); 
	return TCL_OK; 
}

int esweepAudioConfigure(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	const char *opts[] = {"-handle", "-param", "-value", NULL};
	int optMask[] = {1, 1, 1, 0}; // necessary options
	enum optIdx {handleIdx, paramIdx, valIdx};
	int obji;
	int index; 
	int value=-1; 
	char *param=NULL; 

	CHECK_NUM_ARGS(objc == 7, "-handle esweepAudioHandle -param value -value value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				break;
			case paramIdx: 
				if ((param=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -query invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case valIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &value)!=TCL_OK) {
					Tcl_SetResult(interp, "option -value invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioConfigure(handle, param, value) == ERR_OK); 
	Tcl_SetObjResult(interp, Tcl_NewIntObj(value)); 
	return TCL_OK; 
}

int esweepAudioSync(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	Tcl_Obj *tclObj=NULL; 

	const char *opts[] = {"-handle", NULL};
	int optMask[] = {1, 0}; // necessary options
	enum optIdx {handleIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-handle esweepAudioHandle"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				tclObj=objv[obji+1];
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioSync(handle) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepAudioPause(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-handle", NULL};
	int optMask[] = {1, 0}; // necessary options
	enum optIdx {handleIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-handle esweepAudioHandle"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				tclObj=objv[obji+1];
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioPause(handle) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

int esweepAudioOut(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	esweep_object **out=NULL; 
	Tcl_Obj *tclObj; 
	const char *opts[] = {"-handle", "-signals", "-offset", NULL};
	int optMask[] = {1, 1, 0, 0}; // necessary options
	enum optIdx {handleIdx, signalsIdx, offsetIdx};
	int obji;
	int index, i; 
	int offset=0; 
	int channels=0; 

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-handle esweepAudioHandle -signals list ?-offset value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				break;
			case offsetIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &offset)!=TCL_OK) {
					Tcl_SetResult(interp, "option -offset invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case signalsIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &channels)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -signals is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				if (channels <= 0) {
					Tcl_SetResult(interp, "no output signals defined", TCL_STATIC); 
					return TCL_ERROR; 
				}
				ESWEEP_MALLOC(out, channels, sizeof(*out), TCL_ERROR); 
				for (i=0; i < channels; i++) {
					Tcl_ListObjIndex(interp, objv[obji+1], i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {out[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioOut(handle, out, channels, &offset) == ERR_OK); 
	free(out);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(offset)); 
	return TCL_OK; 
}

int esweepAudioIn(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	esweep_object **in=NULL; 
	Tcl_Obj *tclObj; 
	const char *opts[] = {"-handle", "-signals", "-offset", NULL};
	int optMask[] = {1, 1, 0, 0}; // necessary options
	enum optIdx {handleIdx, signalsIdx, offsetIdx};
	int obji;
	int index, i; 
	int offset=0; 
	int channels=0; 

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-handle esweepAudioHandle -signals list ?-offset value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				break;
			case offsetIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &offset)!=TCL_OK) {
					Tcl_SetResult(interp, "option -offset invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case signalsIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &channels)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -signals is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				if (channels <= 0) {
					Tcl_SetResult(interp, "no input signals defined", TCL_STATIC); 
					return TCL_ERROR; 
				}
				ESWEEP_MALLOC(in, channels, sizeof(*in), TCL_ERROR); 
				for (i=0; i < channels; i++) {
					Tcl_ListObjIndex(interp, objv[obji+1], i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {in[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ESWEEP_TCL_ASSERT(esweep_audioIn(handle, in, channels, &offset) == ERR_OK); 
	free(in); 
	Tcl_SetObjResult(interp, Tcl_NewIntObj(offset)); 
	return TCL_OK; 
}

int esweepAudioClose(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_audio *handle=NULL;
	TclEsweepAudio *ea; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-handle", NULL};
	int optMask[] = {1, 0}; // necessary options
	enum optIdx {handleIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-handle esweepAudioHandle"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case handleIdx: 
				CHECK_ESWEEP_AUDIO(obji+1, handle); 
				tclObj=objv[obji+1];
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	
	ea=tclObj->internalRep.otherValuePtr; 
	TCLESWEEPAUDIO_DECREFCOUNT(ea); 
	Tcl_SetObjResult(interp, Tcl_NewIntObj(ea->refCount)); 
	return TCL_OK; 
}

