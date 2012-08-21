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
 * esweep_tcl_wrap_filter.c
 * Wraps the esweep_filter.c source file
 * 30.11.2011, jfab:	initial creation
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

int esweepCreateFilterFromCoeff(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object **filter=NULL; 
	Tcl_Obj **listPtrPtr=NULL; 
	const char *opts[] = {"-type", "-samplerate", "-gain", "-Qp", "-Fp", "-Qz", "-Fz", NULL}; 
	int optMask[] = {1, 1, 0, 0, 0, 0, 0}; // necessary options
	enum optIdx {typeIdx, rateIdx, gainIdx, qpIdx, fpIdx, qzIdx, fzIdx};
	int obji;
	int index; 
	int i; 
	const char *type=NULL; 
	int samplerate; 
	double gain=1.0, qp=1.0, fp=1000.0, qz=1.0, fz=1000.0;

	CHECK_NUM_ARGS(objc >= 5 && objc % 2 == 1, "-type value -samplerate value ?-gain value? ?-Qp value? ?-Fp -value? ?-Qz -value? ?-Fz value? "); 

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
			case gainIdx:
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &gain)!=TCL_OK) {
					Tcl_SetResult(interp, "option -gain invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case qpIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &qp)!=TCL_OK) {
					Tcl_SetResult(interp, "option -qp invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case fpIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &fp)!=TCL_OK) {
					Tcl_SetResult(interp, "option -fp invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case qzIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &qz)!=TCL_OK) {
					Tcl_SetResult(interp, "option -qz invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case fzIdx: 
				if (Tcl_GetDoubleFromObj(NULL, objv[obji+1], &fz)!=TCL_OK) {
					Tcl_SetResult(interp, "option -fz invalid", TCL_STATIC);
					return TCL_ERROR;
				}
				break; 
			case rateIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &samplerate)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -samplerate invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((filter=esweep_createFilterFromCoeff(type, gain, qp, fp, qz, fz, samplerate))!=NULL);

	// make a list of these filter objects
	ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(3*sizeof(Tcl_Obj*)))!=NULL); 
	for (i=0; i < 3; i++) {
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", filter[i]); 
		listPtrPtr[i]=Tcl_NewObj(); 
		listPtrPtr[i]->internalRep.otherValuePtr=filter[i]; 
		listPtrPtr[i]->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
		Tcl_InvalidateStringRep(listPtrPtr[i]);  
	}

	Tcl_SetObjResult(interp, Tcl_NewListObj(3, listPtrPtr)); 
	return TCL_OK; 
}

#if 0
int esweepCreateFilterFromList(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object **filter=NULL; 
	Tcl_Obj **listPtrPtr=NULL; 
	Tcl_Obj *tclObj=NULL, *listObj=NULL; 
	const char *opts[] = {"-num", "-samplerate", "-denom", NULL}; 
	int optMask[] = {1, 1, 0}; // necessary options
	enum optIdx {numIdx, rateIdx, denomIdx};
	int obji;
	int index; 
	int i; 
	int samplerate; 
	double *num=NULL, *denom=NULL; 
	int num_size, denom_size; 
	double val; 

	CHECK_NUM_ARGS(objc == 5 || objc == 7, "-num list ?-denom list? -samplerate value"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case numIdx: 
				if (Tcl_ListObjLength(NULL, objv[obji+1], &num_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -num is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (num_size < 1) {
					Tcl_SetResult(interp, "option -num: list doesnot contain items", TCL_STATIC); 
					return TCL_ERROR; 
				}
				/* create an array */
				if (num != NULL) free(num); 
				ESWEEP_MALLOC(num, num_size, sizeof(double), TCL_ERROR); 
				for (i=0; i < num_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (Tcl_GetDoubleFromObj(NULL, tclObj, &val) == TCL_ERROR) { 
						free(num); 
						if (denom != NULL) free(denom); 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-double item", -1)); 
						return TCL_ERROR; 
					} else {
						num[i]=val; 
					}
				}
				break;
			case denomIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &denom_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -denom is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (num_size < 1) {
					Tcl_SetResult(interp, "option -denom: list doesnot contain items", TCL_STATIC); 
					return TCL_ERROR; 
				}
				/* create an array */
				if (denom != NULL) free(denom); 
				ESWEEP_MALLOC(denom, denom_size, sizeof(double), TCL_ERROR); 
				for (i=0; i < denom_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (Tcl_GetDoubleFromObj(NULL, tclObj, &val) == TCL_ERROR) { 
						if (num != NULL) free(num); 
						free(denom); 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-double item", -1)); 
						return TCL_ERROR; 
					} else {
						denom[i]=val; 
					}
				}
				break;
			case rateIdx: 
				if (Tcl_GetIntFromObj(NULL, objv[obji+1], &samplerate)==TCL_ERROR) {
					Tcl_SetResult(interp, "option -samplerate invalid", TCL_STATIC); 
					return TCL_ERROR;
				}
				break; 
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	if (denom != NULL) {
		if (num_size != denom_size) {
			free(num); 
			free(denom); 
			Tcl_SetObjResult(interp, Tcl_NewStringObj("Size of numerator not equal to size of denominator", -1)); 
			return TCL_ERROR; 
		}
	}


	ESWEEP_TCL_ASSERT((filter=esweep_createFilterFromArray(num, denom, num_size, samplerate))!=NULL);

	// make a list of these filter objects
	ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(3*sizeof(Tcl_Obj*)))!=NULL); 
	for (i=0; i < 3; i++) {
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", filter[i]); 
		listPtrPtr[i]=Tcl_NewObj(); 
		listPtrPtr[i]->internalRep.otherValuePtr=filter[i]; 
		listPtrPtr[i]->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
		Tcl_InvalidateStringRep(listPtrPtr[i]);  
	}

	Tcl_SetObjResult(interp, Tcl_NewListObj(3, listPtrPtr)); 
	return TCL_OK; 
}
#endif

int esweepAppendFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *filter[3], *append[3], *filter_clone[3]; 
	Tcl_Obj *tclObj=NULL, *listObj=NULL, **listPtrPtr=NULL; 
	const char *opts[] = {"-filter", "-append", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {filterIdx, appendIdx};
	int obji;
	int index; 
	int filter_size; 
	int i; 

	CHECK_NUM_ARGS(objc == 5, "-filter filterList -append filterList"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case filterIdx: 
				if (Tcl_ListObjLength(NULL, objv[obji+1], &filter_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -filter is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (filter_size != 3) {
					Tcl_SetResult(interp, "option -filter is no valid esweep filter", TCL_STATIC); 
					return TCL_ERROR; 
				}
				for (i=0; i< filter_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {filter[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break;
			case appendIdx:
				if (Tcl_ListObjLength(NULL, objv[obji+1], &filter_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -append is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				if (filter_size != 3) {
					Tcl_SetResult(interp, "option -append is no valid esweep filter", TCL_STATIC); 
					return TCL_ERROR; 
				}
				for (i=0; i< filter_size; i++) {
					Tcl_ListObjIndex(interp, objv[obji+1], i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {append[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	/*
	 * esweep_appendFilter() frees the filter structure, and therefore the Tcl list produces a core dump. 
	 * We must clone the filters, append to the clone, then replace the list objects, 
	 */
	for (i=0; i < filter_size; i++) {
		filter_clone[i]=esweep_clone(filter[i]); 
	}

	ESWEEP_TCL_ASSERT(esweep_appendFilter(filter_clone, append) == ERR_OK); 

	ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(filter_size*sizeof(Tcl_Obj*)))!=NULL); 
	for (i=0; i < filter_size; i++) {
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", filter_clone[i]); 
		listPtrPtr[i]=Tcl_NewObj(); 
		listPtrPtr[i]->internalRep.otherValuePtr=filter_clone[i]; 
		listPtrPtr[i]->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
		Tcl_InvalidateStringRep(listPtrPtr[i]);  
	}
	if (Tcl_IsShared(listObj)) {
	    listObj = Tcl_DuplicateObj(listObj);
	}
	Tcl_ListObjReplace(interp, listObj, 0, filter_size, filter_size, listPtrPtr); 

	Tcl_InvalidateStringRep(listObj);  
	Tcl_SetObjResult(interp, listObj); 
	return TCL_OK; 
}

int esweepResetFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *filter[3]; 
	Tcl_Obj *tclObj=NULL, *listObj=NULL;
	const char *opts[] = {"-filter", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {filterIdx};
	int obji;
	int index; 
	int filter_size; 
	int i; 

	CHECK_NUM_ARGS(objc == 3, "-filter filterList"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case filterIdx: 
				if (Tcl_ListObjLength(NULL, objv[obji+1], &filter_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -filter is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (filter_size != 3) {
					Tcl_SetResult(interp, "option -filter is no valid esweep filter", TCL_STATIC); 
					return TCL_ERROR; 
				}
				for (i=0; i< filter_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {filter[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_resetFilter(filter) == ERR_OK); 

	Tcl_InvalidateStringRep(listObj);  
	Tcl_SetObjResult(interp, listObj); 
	return TCL_OK; 
}

int esweepCloneFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object **dst, *src[3]; 
	Tcl_Obj *tclObj=NULL, *listObj=NULL;
	Tcl_Obj **listPtrPtr=NULL; 
	const char *opts[] = {"-src", NULL};
	int optMask[] = {1}; // necessary options
	enum optIdx {srcIdx};
	int obji;
	int index; 
	int filter_size; 
	int i; 

	CHECK_NUM_ARGS(objc == 3, "-src filterList"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", 0, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case srcIdx: 
				if (Tcl_ListObjLength(NULL, objv[obji+1], &filter_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -src is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (filter_size != 3) {
					Tcl_SetResult(interp, "option -src is no valid esweep filter", TCL_STATIC); 
					return TCL_ERROR; 
				}
				for (i=0; i< filter_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {src[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT((dst=esweep_cloneFilter(src)) != NULL); 
	// make a list of these filter objects
	ESWEEP_TCL_ASSERT((listPtrPtr=(Tcl_Obj**) ckalloc(3*sizeof(Tcl_Obj*)))!=NULL); 
	for (i=0; i < 3; i++) {
		ESWEEP_DEBUG_PRINT("Creating object: %p\n", dst[i]); 
		listPtrPtr[i]=Tcl_NewObj(); 
		listPtrPtr[i]->internalRep.otherValuePtr=dst[i]; 
		listPtrPtr[i]->typePtr = (Tcl_ObjType*) &tclEsweepObjType; 
		Tcl_InvalidateStringRep(listPtrPtr[i]);  
	}

	Tcl_SetObjResult(interp, Tcl_NewListObj(3, listPtrPtr)); 
	return TCL_OK; 
}

int esweepFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	esweep_object *filter[3]={NULL, NULL, NULL}, *signal=NULL; 
	Tcl_Obj *tclObj=NULL, *listObj=NULL; 
	const char *opts[] = {"-signal", "-filter", NULL};
	int optMask[] = {1, 1}; // necessary options
	enum optIdx {signalIdx, filterIdx};
	int obji;
	int index; 
	int filter_size; 
	int i; 

	CHECK_NUM_ARGS(objc == 5, "-signal objVarName -filter filterList"); 

	for (obji=1; obji < objc; obji+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[obji], opts, "option", TCL_EXACT, &index) != TCL_OK) {
			return TCL_ERROR; 
		}
		switch (index) {
			case signalIdx: 
				CHECK_ESWEEP_OBJECT2(obji+1, tclObj, signal); 
				break;
			case filterIdx: 
				if (Tcl_ListObjLength(NULL, objv[obji+1], &filter_size)!=TCL_OK) {
					Tcl_SetResult(interp, "parameter of option -filter is not a list", TCL_STATIC);
					return TCL_ERROR; 
				}
				listObj=objv[obji+1]; 
				if (filter_size != 3) {
					Tcl_SetResult(interp, "option -filter is no valid esweep filter", TCL_STATIC); 
					return TCL_ERROR; 
				}
				for (i=0; i< filter_size; i++) {
					Tcl_ListObjIndex(interp, listObj, i, &tclObj); 
					if (tclObj->typePtr != &tclEsweepObjType) { 
						Tcl_SetObjResult(interp, Tcl_NewStringObj("List contains non-esweep objects", -1)); 
						return TCL_ERROR; 
					} else {filter[i]=(esweep_object*) tclObj->internalRep.otherValuePtr;}
				}
				break;
		}
		optMask[index]=0; 
	}
	CHECK_MISSING_OPTIONS(opts, optMask, index); 

	ESWEEP_TCL_ASSERT(esweep_filter(signal, filter) == ERR_OK); 
	Tcl_InvalidateStringRep(tclObj);  
	Tcl_SetObjResult(interp, tclObj); 
	return TCL_OK; 
}

