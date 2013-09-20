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
 * esweep_tcl_wrap_base.c
 * Wraps the esweep_base.c source file
 * 07.10.2011, jfab:	initial creation
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

int esweepGet(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *src=NULL, *dst=NULL; 
	Tcl_Obj *ret; 

	const char *opts[] = {"-obj", "-from", "-to", NULL};
	int optMask[] = {1, 1, 1}; // necessary options
	enum optIdx {objIdx, fromIdx, toIdx};
	int obji;
	int index; 
	int from, to; 

	CHECK_NUM_ARGS(objc == 7, "-obj esweepObject -from index -to index"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, src); 
				break; 
			case fromIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &from)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case toIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &to)!=TCL_OK) {
					Tcl_SetResult(interp, "option -to invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((dst=esweep_get(src, from, to)) != NULL); 
	ESWEEP_DEBUG_PRINT("Creating object: %p\n", dst); 

	ret=Tcl_NewObj(); 
	ret->internalRep.otherValuePtr=dst; 
	ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
	Tcl_InvalidateStringRep(ret);  
	Tcl_SetObjResult(interp, ret);
	return TCL_OK;
}

int esweepSize(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 

	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 
	int size; 
	
	CHECK_NUM_ARGS(objc == 3, "-obj esweepObject"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((esweep_size(obj, &size)) == ERR_OK);

	Tcl_SetObjResult(interp, Tcl_NewIntObj(size)); 
	return TCL_OK; 
}

int esweepSamplerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 

	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 
	int samplerate; 
	
	CHECK_NUM_ARGS(objc == 3, "-obj esweepObject"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((esweep_samplerate(obj, &samplerate)) == ERR_OK);

	Tcl_SetObjResult(interp, Tcl_NewIntObj(samplerate)); 
	return TCL_OK; 
}

int esweepType(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 

	const char *opts[] = {"-obj", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {objIdx};
	int obji;
	int index; 
	const char *type=NULL; 
	
	CHECK_NUM_ARGS(objc == 3, "-obj esweepObject"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((esweep_type(obj, &type)) == ERR_OK);

	Tcl_SetObjResult(interp, Tcl_NewStringObj(type, -1)); 
	return TCL_OK; 
}

int esweepSetSamplerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj *tclObj=NULL; 

	const char *opts[] = {"-obj", "-samplerate", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {objIdx, rateIdx};
	int obji;
	int index; 
	int samplerate; 

	CHECK_NUM_ARGS(objc == 5, "-obj objectVarName -samplerate value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, obj); 
				break; 
			case rateIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &samplerate)!=TCL_OK) {
					Tcl_SetResult(interp, "option -from invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_setSamplerate(obj, samplerate) == ERR_OK); 

	Tcl_InvalidateStringRep(tclObj);  
	Tcl_SetObjResult(interp, Tcl_NewIntObj(samplerate));
	return TCL_OK;
}

int esweepIndex(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *obj=NULL; 
	Tcl_Obj **listPtrPtr=NULL;

	const char *opts[] = {"-obj", "-index", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {objIdx, idxIdx};
	int obji;
	int index; 
	Real a, b, x; 
	int i; 
	int listLength=3; 
	
	CHECK_NUM_ARGS(objc == 5, "-obj esweepObject -index value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, obj); 
				break; 
			case idxIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &i)!=TCL_OK) {
					Tcl_SetResult(interp, "option -index invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((esweep_index(obj, i, &x, &a, &b)) == ERR_OK);

	if (obj->type == WAVE) {
		listLength=2; 
		ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(listLength*sizeof(Tcl_Obj*)))!=NULL); 
		listPtrPtr[0]=Tcl_NewDoubleObj(x); 
		listPtrPtr[1]=Tcl_NewDoubleObj(a); 
	} else {
		listLength=3; 
		ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(listLength*sizeof(Tcl_Obj*)))!=NULL); 
		listPtrPtr[0]=Tcl_NewDoubleObj(x); 
		listPtrPtr[1]=Tcl_NewDoubleObj(a); 
		listPtrPtr[2]=Tcl_NewDoubleObj(b); 
	}

	Tcl_SetObjResult(interp, Tcl_NewListObj(listLength, listPtrPtr));
	return TCL_OK; 
}

