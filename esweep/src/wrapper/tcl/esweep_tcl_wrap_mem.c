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
 * esweep_tcl_wrap_mem.c
 * Wraps the esweep_mem.c source file
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

/*
 * create an esweep object
 * result is a handle ("NULL" in case of error)
 * Parameters: 
 * Type (string: "wave", "polar", "complex", "surface")
 * Samplerate (int, >0)
 * Size (int, >=0, default 0)
 */ 

int esweepCreate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	char *type=NULL; 
	int samplerate=0; 
	int size=0; 
	esweep_object *handle=NULL; 

	int obji;
	int index; 

	const char *opts[] = {"-type", "-samplerate", "-size", NULL};
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {typeIdx, samplerateIdx, sizeIdx};

	Tcl_Obj *ret; 

	CHECK_NUM_ARGS(objc >= 5 && objc <= 7 && (objc-1)%2 == 0, "-type value -samplerate value ?-size value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case typeIdx: 
				if ((type=Tcl_GetString(objv[obji+1]))==NULL) {
					/* almost impossible */
					Tcl_SetResult(interp, "option -type invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case samplerateIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &samplerate)!=TCL_OK) {
					Tcl_SetResult(interp, "option -samplerate invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case sizeIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &size)!=TCL_OK) {
					Tcl_SetResult(interp, "option -size invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((handle=esweep_create(type, samplerate, size)) != NULL); 
	ESWEEP_DEBUG_PRINT("Creating object: %p\n", handle); 

	ret=Tcl_NewObj(); 
	ret->internalRep.otherValuePtr=handle; 
	ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
	Tcl_InvalidateStringRep(ret);  
	Tcl_SetObjResult(interp, ret);
	return TCL_OK;
}
 
int esweepClone(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *src=NULL, *dst=NULL; 
	Tcl_Obj *ret=NULL; 
	const char *opts[] = {"-src", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {srcIdx};
	int obji;
	int index;

	CHECK_NUM_ARGS(objc == 3, "-src esweepObj"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case srcIdx: 
				CHECK_ESWEEP_OBJECT(obji+1, src); 
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((dst=esweep_clone(src)) != NULL); 
	ESWEEP_DEBUG_PRINT("Creating object: %p\n", dst); 

	ret=Tcl_NewObj(); 
	ret->internalRep.otherValuePtr=dst; 
	ret->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
	Tcl_InvalidateStringRep(ret);  
	Tcl_SetObjResult(interp, ret);
	return TCL_OK;
}

int esweepSparseSurface(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *handle=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-obj", "-xsize", "-ysize", NULL};
	int optMask[] = {1, 1, 1}; // necessary options
	enum optIdx {objIdx, xsizeIdx, ysizeIdx};
	int obji;
	int index; 

	int xsize, ysize; 

	CHECK_NUM_ARGS(objc == 7, "-obj objVarName -xsize value -ysize value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case objIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, handle); 
				break; 
			case xsizeIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &xsize)!=TCL_OK) {
					Tcl_SetResult(interp, "option -xsize invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case ysizeIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &ysize)!=TCL_OK) {
					Tcl_SetResult(interp, "option -ysize invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	DUPLICATE_WHEN_SHARED(tclObj, handle);
	ESWEEP_TCL_ASSERT(esweep_sparseSurface(handle, xsize, ysize) == ERR_OK); 
	Tcl_InvalidateStringRep(tclObj);  

	Tcl_SetObjResult(interp, Tcl_NewIntObj(xsize*ysize)); 
	return TCL_OK; 
}

/* default value for length is src length. The esweep library will trim it to the maximum allowed size */
int esweepMove(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *src=NULL, *dst=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-src", "-dst", "-srcIdx", "-dstIdx", "-length", NULL};
	int optMask[] = {1, 1, 0, 0, 0}; // necessary options
	enum optIdx {srcIdx, dstIdx, srcIndexIdx, dstIndexIdx, lenIdx};
	int obji;
	int index; 

	int srcIndex=0, dstIndex=0, length=-1; 

	CHECK_NUM_ARGS(objc >= 5 && objc <= 11 && (objc-1)%2 == 0, "-dst objVarName -src obj ?-srcIdx value? ?-dstIdx value? ?-length value?"); 


	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case srcIdx:
				CHECK_ESWEEP_OBJECT(obji+1, src); 
				break; 
			case dstIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, dst); 
				break; 
			case srcIndexIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &srcIndex)!=TCL_OK) {
					Tcl_SetResult(interp, "option -srcIdx invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case dstIndexIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &dstIndex)!=TCL_OK) {
					Tcl_SetResult(interp, "option -dstIdx invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case lenIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &length)!=TCL_OK) {
					Tcl_SetResult(interp, "option -length invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	length= (length < 0) ? src->size : length; 

	DUPLICATE_WHEN_SHARED(tclObj, dst);
	ESWEEP_TCL_ASSERT(esweep_move(dst, src, dstIndex, srcIndex, &length) == ERR_OK); 
	Tcl_InvalidateStringRep(tclObj);  

	Tcl_SetObjResult(interp, Tcl_NewIntObj(length)); 
	return TCL_OK; 
}

/* default value for length is src length. The esweep library will trim it to the maximum allowed size */
int esweepCopy(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *src=NULL, *dst=NULL; 
	Tcl_Obj *tclObj=NULL; 
	const char *opts[] = {"-src", "-dst", "-srcIdx", "-dstIdx", "-length", NULL};
	int optMask[] = {1, 1, 0, 0, 0}; // necessary options
	enum optIdx {srcIdx, dstIdx, srcIndexIdx, dstIndexIdx, lenIdx};
	int obji;
	int index; 

	int srcIndex=0, dstIndex=0, length=-1; 

	CHECK_NUM_ARGS(objc >= 5 && objc <= 11 && (objc-1)%2 == 0, "-dst objVarName -src obj ?-srcIdx value? ?-dstIdx value? ?-length value?"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case srcIdx:
				CHECK_ESWEEP_OBJECT(obji+1, src); 
				break; 
			case dstIdx:
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, dst); 
				break; 
			case srcIndexIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &srcIndex)!=TCL_OK) {
					Tcl_SetResult(interp, "option -srcIdx invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case dstIndexIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &dstIndex)!=TCL_OK) {
					Tcl_SetResult(interp, "option -srcIdx invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
			case lenIdx:
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &length)!=TCL_OK) {
					Tcl_SetResult(interp, "option -length invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 
	length= (length < 0) ? src->size : length; 

	DUPLICATE_WHEN_SHARED(tclObj, dst);
	ESWEEP_TCL_ASSERT(esweep_copy(dst, src, dstIndex, srcIndex, &length) == ERR_OK); 
	Tcl_InvalidateStringRep(tclObj);  
	Tcl_SetObjResult(interp, Tcl_NewIntObj(length)); 
	return TCL_OK; 
}

