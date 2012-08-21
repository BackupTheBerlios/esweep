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
 * esweep_tcl_wrap_dsp.c
 * Wraps the esweep_dsp.c source file
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

int esweepFFT(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL, *table=NULL, *fftObj=NULL; 
	Tcl_Obj *tclObj=NULL, *ret=NULL; 
	const char *opts[] = {"-obj", "-table", "-inplace", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {sigIdx, tabIdx, inplaceIdx};
	int obji;
	int index; 
	int inplace=0; 
	int samplerate; 
	int size; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 6, "-obj objVarName ?-table obj? ?-inplace?"); 

	for (obji=1; obji < objc; obji++) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case sigIdx: 
				obji++; 
				CHECK_ESWEEP_OBJECT2(obji, tclObj, obj); 
				break;
			case tabIdx:
				obji++; 
				CHECK_ESWEEP_OBJECT(obji, table); 
				break;
			case inplaceIdx: 
				inplace=1; 
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	if (inplace) {
		DUPLICATE_WHEN_SHARED(tclObj, obj);
		fftObj=obj; 
		ret=tclObj; 
	} else {
		ESWEEP_TCL_ASSERT(esweep_size(obj, &size)==ERR_OK); 
		ESWEEP_TCL_ASSERT(size > 0); 
		ESWEEP_TCL_ASSERT(esweep_samplerate(obj, &samplerate)==ERR_OK); 
		ESWEEP_TCL_ASSERT((fftObj=esweep_create("complex", samplerate, size))!=NULL);
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", fftObj); 
		ret=Tcl_NewObj(); 
		ret->internalRep.otherValuePtr=fftObj; 
		ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
	}

	ESWEEP_TCL_ASSERT(esweep_fft(fftObj, obj, table) == ERR_OK); 
	Tcl_SetObjResult(interp, ret); 
	Tcl_InvalidateStringRep(ret);  
	return TCL_OK; 
}

int esweepIFFT(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL, *table=NULL, *fftObj=NULL; 
	Tcl_Obj *tclObj=NULL, *ret=NULL; 
	const char *opts[] = {"-obj", "-table", "-inplace", NULL};
	int optMask[] = {1, 0, 0}; // necessary options
	enum optIdx {sigIdx, tabIdx, inplaceIdx};
	int obji;
	int index; 
	int inplace=0; 
	int samplerate; 
	int size; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 6, "-obj objVarName ?-table obj? ?-inplace?"); 

	for (obji=1; obji < objc; obji++) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case sigIdx: 
				obji++; 
				CHECK_ESWEEP_OBJECT2(obji, tclObj, obj); 
				break;
			case tabIdx:
				obji++; 
				CHECK_ESWEEP_OBJECT(obji, table); 
				break;
			case inplaceIdx: 
				inplace=1; 
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	if (inplace) {
		DUPLICATE_WHEN_SHARED(tclObj, obj);
		fftObj=obj; 
		ret=tclObj; 
	} else {
		ESWEEP_TCL_ASSERT(esweep_size(obj, &size)==ERR_OK); 
		ESWEEP_TCL_ASSERT(size > 0); 
		ESWEEP_TCL_ASSERT(esweep_samplerate(obj, &samplerate)==ERR_OK); 
		ESWEEP_TCL_ASSERT((fftObj=esweep_create("complex", samplerate, size))!=NULL);
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", fftObj); 
		ret=Tcl_NewObj(); 
		ret->internalRep.otherValuePtr=fftObj; 
		ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
	}

	ESWEEP_TCL_ASSERT(esweep_ifft(fftObj, obj, table) == ERR_OK); 
	Tcl_SetObjResult(interp, ret); 
	Tcl_InvalidateStringRep(ret);  
	return TCL_OK; 
}

int esweepCreateFFTTable(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", "-size", NULL};
	int optMask[] = {1, 0}; // necessary options
	enum optIdx {objIdx, sizeIdx};
	int obji;
	int index; 
	int size=-1; 

	CHECK_NUM_ARGS(objc == 3 || objc == 5, "-obj objVarName -size value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
			case sizeIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &size)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -size invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_createFFTTable(obj, size) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepDeconvolve(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *in=NULL, *filter=NULL, *table=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-signal", "-filter", "-table", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {sigIdx, filtIdx, tabIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc >= 5 && objc <= 7 && (objc-1)%2 == 0, "-signal objVarName -filter obj ?-table obj?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case sigIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, in); 
				break;
			case filtIdx:
				CHECK_ESWEEP_OBJECT(obji+1, filter); 
				break;
			case tabIdx:
				CHECK_ESWEEP_OBJECT(obji+1, table); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);
	ESWEEP_TCL_ASSERT(esweep_deconvolve(in, filter, table) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepDelay(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *signal=NULL, *line=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-signal", "-line", "-offset", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {sigIdx, lineIdx, offIdx};
	int obji;
	int index; 
	int offset=0; 

	CHECK_NUM_ARGS(objc == 5, "-obj objVarName -factor value -offset value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case sigIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, signal); 
				break;
			case lineIdx:
				CHECK_ESWEEP_OBJECT(obji+1, line); 
				break;
			case offIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &offset)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -offset invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, signal);
	ESWEEP_TCL_ASSERT(esweep_delay(signal, line, &offset) == ERR_OK); 
	Tcl_InvalidateStringRep(tclObj);  
	Tcl_SetObjResult(interp, Tcl_NewIntObj(offset)); 
	return TCL_OK; 
}	

int esweepSmooth(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", "-factor", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {objIdx, factorIdx};
	int obji;
	int index; 
	double factor; 

	CHECK_NUM_ARGS(objc == 5, "-obj objVarName -factor value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
			case factorIdx:
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &factor)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -factor invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_smooth(obj, factor) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepUnwrapPhase(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_unwrapPhase(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepWrapPhase(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_wrapPhase(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepRestoreHermitian(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx, factorIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_restoreHermitian(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepWindow(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", "-rightwin", "-rightwidth", "-leftwin", "-leftwidth", NULL};
	int optMask[] = {1, 0, 0, 0, 0}; // necessary options
	enum optIdx {objIdx, rightWinIdx, rightWidthIdx, leftWinIdx, leftWidthIdx};
	int obji;
	int index; 
	char *rightWin="rect"; 
	char *leftWin="rect"; 
	double rightWidth=0.0, leftWidth=0.0; 

	CHECK_NUM_ARGS(objc >= 3 && objc <= 11 && objc % 2 == 1, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
			case rightWinIdx:
				if ((rightWin=Tcl_GetString(objv[obji+1]))==NULL) {
				/* almost impossible */
					Tcl_SetResult(interp, "option -rightwin invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case rightWidthIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &rightWidth)!=TCL_OK) {
					Tcl_SetResult(interp, "option -rightwidth invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case leftWinIdx:
				if ((leftWin=Tcl_GetString(objv[obji+1]))==NULL) {
				/* almost impossible */
					Tcl_SetResult(interp, "option -leftwin invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case leftWidthIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &leftWidth)!=TCL_OK) {
					Tcl_SetResult(interp, "option -leftwidth invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_window(obj, leftWin, leftWidth, rightWin, rightWidth) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepPeakDetect(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *listPtr=NULL; 

	const char *opts[] = {"-obj", "-threshold", "-numPeaks", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {objIdx, thresIdx, numIdx};
	int obji;
	int index; 
	double threshold; 
	int *peaks; 
	int n=0, i; 

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-obj objVar -threshold value ?-numPeaks value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break;
			case thresIdx:
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &threshold)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -threshold invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
			case numIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &n)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -numPeaks invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	if (n <= 0) {
		ESWEEP_TCL_ASSERT(esweep_size(obj, &n) == ERR_OK); 
	}
	ESWEEP_TCL_ASSERT((peaks = (int*) calloc(n, sizeof(int))) != NULL); 

	ESWEEP_TCL_ASSERT(esweep_peakDetect(obj, threshold, peaks, &n) == ERR_OK); 
	
	listPtr=Tcl_NewListObj(0, NULL); 
	for (i=0; i < n; i++) {
		Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(peaks[i])); 
	}
	free(peaks); 

	Tcl_SetObjResult(interp, listPtr);
	return TCL_OK; 
}

int esweepIntegrate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_integrate(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}

int esweepDifferentiate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 

	CHECK_NUM_ARGS(objc == 3, "-obj objVarName"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	DUPLICATE_WHEN_SHARED(tclObj, in);

	ESWEEP_TCL_ASSERT(esweep_differentiate(obj) == ERR_OK); 
	Tcl_SetObjResult(interp, tclObj); 
	Tcl_InvalidateStringRep(tclObj);  
	return TCL_OK; 
}
