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
 * esweep_tcl_wrap.c
 * Wrapper around the esweep library for Tcl
 * 03.10.2011: initial creation
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

/* Structure for command table */
typedef struct {
  const char *name;
  int       (*wrapper)(ClientData, Tcl_Interp *, int, Tcl_Obj *CONST []);
  ClientData  clientdata;
} command_info;

/* command table */
static command_info mod_commands[] = {
	{"::esweep::create", esweepCreate, NULL},
	{"::esweep::clone", esweepClone, NULL},
	{"::esweep::sparseSurface", esweepSparseSurface, NULL},
	{"::esweep::move", esweepMove, NULL},
	{"::esweep::copy", esweepCopy, NULL},

	{"::esweep::get", esweepGet, NULL},
	{"::esweep::index", esweepIndex, NULL},
	{"::esweep::size", esweepSize, NULL},
	{"::esweep::type", esweepType, NULL},
	{"::esweep::samplerate", esweepSamplerate, NULL},
	{"::esweep::setSamplerate", esweepSetSamplerate, NULL},

	{"::esweep::toWave", esweepToWave, NULL},
	{"::esweep::toComplex", esweepToComplex, NULL},
	{"::esweep::toPolar", esweepToPolar, NULL},

	{"::esweep::generate", esweepGenerate, NULL},

	{"::esweep::deconvolve", esweepDeconvolve, NULL},
	{"::esweep::fft", esweepFFT, NULL},
	{"::esweep::ifft", esweepIFFT, NULL},
	{"::esweep::createFFTTable", esweepCreateFFTTable, NULL},
	{"::esweep::delay", esweepDelay, NULL},
	{"::esweep::smooth", esweepSmooth, NULL},
	{"::esweep::unwrapPhase", esweepUnwrapPhase, NULL},
	{"::esweep::wrapPhase", esweepWrapPhase, NULL},
	{"::esweep::restoreHermitian", esweepRestoreHermitian, NULL},
	{"::esweep::window", esweepWindow, NULL},
	{"::esweep::peakDetect", esweepPeakDetect, NULL},
	{"::esweep::integrate", esweepIntegrate, NULL},
	{"::esweep::differentiate", esweepDifferentiate, NULL},

	{"::esweep::createFilterFromCoeff", esweepCreateFilterFromCoeff, NULL},
	/*{"::esweep::createFilterFromList", esweepCreateFilterFromList, NULL},*/
	{"::esweep::appendFilter", esweepAppendFilter, NULL},
	{"::esweep::cloneFilter", esweepCloneFilter, NULL},
	{"::esweep::resetFilter", esweepResetFilter, NULL},
	{"::esweep::filter", esweepFilter, NULL},

	{"::esweep::toAscii", esweepToAscii, NULL},
	{"::esweep::load", esweepLoad, NULL},
	{"::esweep::save", esweepSave, NULL},

	{"::esweep::expr", esweepExpr, NULL},
	{"::esweep::min", esweepMin, NULL},
	{"::esweep::max", esweepMax, NULL},
	{"::esweep::minPos", esweepMinPos, NULL},
	{"::esweep::maxPos", esweepMaxPos, NULL},
	{"::esweep::avg", esweepAvg, NULL},
	{"::esweep::sum", esweepSum, NULL},
	{"::esweep::sqsum", esweepSqsum, NULL},
	{"::esweep::real", esweepReal, NULL},
	{"::esweep::imag", esweepImag, NULL},
	{"::esweep::abs", esweepAbs, NULL},
	{"::esweep::arg", esweepArg, NULL},
	{"::esweep::lg", esweepLg, NULL},
	{"::esweep::ln", esweepLn, NULL},
	{"::esweep::exp", esweepExp, NULL},
	{"::esweep::pow", esweepPow, NULL},
	{"::esweep::schroeder", esweepSchroeder, NULL},
#ifndef NOAUDIO
	{"::esweep::audioOpen", esweepAudioOpen, NULL},
	{"::esweep::audioQuery", esweepAudioQuery, NULL},
	{"::esweep::audioConfigure", esweepAudioConfigure, NULL},
	{"::esweep::audioSync", esweepAudioSync, NULL},
	{"::esweep::audioPause", esweepAudioPause, NULL},
	{"::esweep::audioOut", esweepAudioOut, NULL},
	{"::esweep::audioIn", esweepAudioIn, NULL},
	{"::esweep::audioClose", esweepAudioClose, NULL},
#endif
	{"::esweep::getCoords", esweepGetCoords, NULL},

	{0, 0, 0}
};

/* Functions to fill the Tcl_ObjType struct */
/* These functions are borrowed from TclListObj.c from the Tcl project */
static void DupEsweepObjInternalRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static void FreeEsweepObjInternalRep(Tcl_Obj *esweepObjPtr);
/* This will not be implemented in the near future */
//static int SetEsweepObjFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void UpdateStringOfEsweepObj(Tcl_Obj *esweepObjPtr);

/* The Tcl_ObjType structure. The internal representation is a single pointer value
 * (void *otherValuePtr;). The type is not registered, so we may set setFromAnyProc to NULL */

#define ESWEEP_TYPE_NAME "esweep"

const Tcl_ObjType tclEsweepObjType = {
	ESWEEP_TYPE_NAME, /* name */
	FreeEsweepObjInternalRep, /* freeIntRepProc */
	DupEsweepObjInternalRep, /* dupIntRepProc */
#ifdef NOSTRINGREP
	NULL,
#else
	UpdateStringOfEsweepObj, /* updateStringProc */
#endif /* NOSTRINGREP */
	NULL
	//SetFromAnyEsweepObj /* setFromAnyProc */
}; 

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT int Esweep_Init(Tcl_Interp *interp) {
	int i;
	if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
		return TCL_ERROR;
	}
	if (Tcl_PkgProvide(interp, "esweep", "0.5") == TCL_ERROR) {
		return TCL_ERROR;
	}
	if (Tcl_CreateNamespace(interp, "::esweep", NULL, NULL) == NULL) {
		return TCL_ERROR;
	}
	for (i=0;mod_commands[i].name;i++) {
		Tcl_CreateObjCommand(interp, mod_commands[i].name, mod_commands[i].wrapper, mod_commands[i].clientdata, NULL);
	}	
	return TCL_OK;
}

#ifdef __cplusplus
}
#endif

/* DupEsweepObjInternalRep() 
 *
 * Duplicate an esweep object. At the moment this functions performs a deep copy. 
 * It is planned that a shared object scheme is used in the future. 
 */
static void DupEsweepObjInternalRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr) {
	esweep_object *srcObj=(esweep_object*) srcPtr->internalRep.otherValuePtr; 
	esweep_object *copyObj=esweep_clone(srcObj);
	copyPtr->internalRep.otherValuePtr=copyObj; 
	ESWEEP_DEBUG_PRINT("Duplicating object %p to %p\n", srcObj, copyObj); 
}
	
static void FreeEsweepObjInternalRep(Tcl_Obj *esweepObjPtr) {
	esweep_object *obj=(esweep_object*) esweepObjPtr->internalRep.otherValuePtr;
	ESWEEP_DEBUG_PRINT("Freeing object: %p\n", obj);
	esweep_free(obj);
}

#if !(__BSD_VISIBLE || __XPG_VISIBLE >= 400)
static int nstrncat(char* dst, const char* src, size_t size) {
	   char *str=strncat(dst, src, size); 
	   return strlen(str); 
}
#endif
	   
	   

static void UpdateStringOfEsweepObj(Tcl_Obj *esweepObjPtr) {
# define TMP_SIZE 256
	esweep_object *obj=(esweep_object*) esweepObjPtr->internalRep.otherValuePtr;
	unsigned strSize, size; 
	char tmpStr[TMP_SIZE]; 
	char *resStr; 
	int i; 
	Wave *wave; 
	Polar *polar; 
	Complex *cpx; 
	//Surface *surf; 

	ESWEEP_DEBUG_PRINT("Creating string rep of object: %p", obj);
	switch (obj->type) {
		case WAVE: 
			/* 
			 * The object is converted as the following form
			 * "wave samplerate size n0 n1 ..."
			 */

			/* 
			 * estimate the size of the string 
			 */

			/* each number needs TCL_DOUBLE_SPACE */
			strSize=TCL_DOUBLE_SPACE*obj->size; 
			/* after each number there must be a whitespace */
			strSize+=obj->size; 
			/* the type is written first ("wave" == 5 bytes inclusive whitespace) */
			strSize+=5; 
			/* Then the samplerate; we simply assume a size of 32+1 (for whitespace). This will be enough for the next million years */
			strSize+=33; 
			/* The same for the size */
			strSize+=33; 
			/* now allocate the string */
			resStr=ckalloc(strSize); 
			/* initialize the stringRep with whitespaces */
			memset(resStr, '\0', strSize); 

			/* copy the string */
			STRCPY(resStr, "wave", strSize); 
			resStr[4]=' '; 
			snprintf(tmpStr, TMP_SIZE, "%i %i ", obj->samplerate, obj->size); 
			size=STRCAT(resStr, tmpStr, strSize); 
			wave=(Wave*) obj->data; 
			for (i=0; i<obj->size; i++) {
				/* empty tmp str */
				memset(tmpStr, '\0', TMP_SIZE); 
				snprintf(tmpStr, TMP_SIZE, "%g ", wave[i]); 
				size=STRCAT(resStr, tmpStr, strSize); 
			}
			esweepObjPtr->length=size; 
			esweepObjPtr->bytes=ckalloc((unsigned) esweepObjPtr->length); 
			STRCPY(esweepObjPtr->bytes, resStr, esweepObjPtr->length); 
			ckfree(resStr); 
			break; 
		case COMPLEX: 
			/* 
			 * The object is converted as the following form
			 * "complex samplerate size {real0 imag0} {real1 imag1} ..."
			 */

			/* 
			 * estimate the size of the string 
			 */

			/* each number needs TCL_DOUBLE_SPACE, Complex consists of 2 numbers, plus 
			 * braces {} and a whitespace for each complex number */
			strSize=(2*TCL_DOUBLE_SPACE+3)*obj->size; 
			/* after each complex number there must be a whitespace */
			strSize+=obj->size; 
			/* the type is written first ("complex" == 8 bytes inclusive whitespace) */
			strSize+=8; 
			/* Then the samplerate; we simply assume a size of 32+1 (for whitespace). This will be enough for the next million years */
			strSize+=33; 
			/* The same for the size */
			strSize+=33; 
			/* now allocate the string */
			resStr=ckalloc(strSize); 
			/* initialize the stringRep with whitespaces */
			memset(resStr, '\0', strSize); 

			/* copy the string */
			STRCPY(resStr, "complex", strSize); 
			resStr[7]=' '; 
			snprintf(tmpStr, TMP_SIZE, "%i %i ", obj->samplerate, obj->size); 
			size=STRCAT(resStr, tmpStr, strSize); 
			cpx=(Complex*) obj->data; 
			for (i=0; i<obj->size; i++) {
				/* empty tmp str */
				memset(tmpStr, '\0', TMP_SIZE); 
				snprintf(tmpStr, TMP_SIZE, "{%g %g} ", cpx[i].real, cpx[i].imag); 
				size=STRCAT(resStr, tmpStr, strSize); 
			}
			esweepObjPtr->length=size; 
			esweepObjPtr->bytes=ckalloc((unsigned) esweepObjPtr->length); 
			STRCPY(esweepObjPtr->bytes, resStr, esweepObjPtr->length); 
			ckfree(resStr); 

			break; 
		case POLAR: 
			/* 
			 * The object is converted as the following form
			 * "polar samplerate size {abs0 arg0} {abs1 arg1} ..."
			 */

			/* 
			 * estimate the size of the string 
			 */

			/* each number needs TCL_DOUBLE_SPACE, Polar consists of 2 numbers, plus 
			 * braces {} and a whitespace for each polar number */
			strSize=(2*TCL_DOUBLE_SPACE+3)*obj->size; 
			/* after each complex number there must be a whitespace */
			strSize+=obj->size; 
			/* the type is written first ("polar" == 6 bytes inclusive whitespace) */
			strSize+=6; 
			/* Then the samplerate; we simply assume a size of 32+1 (for whitespace). This will be enough for the next million years */
			strSize+=33; 
			/* The same for the size */
			strSize+=33; 

			/* now allocate the string */
			resStr=ckalloc(strSize); 
			/* initialize the stringRep with whitespaces */
			memset(resStr, '\0', strSize); 

			/* copy the string */
			STRCPY(resStr, "polar", strSize); 
			resStr[5]=' '; 
			snprintf(tmpStr, TMP_SIZE, "%i %i ", obj->samplerate, obj->size); 
			size=STRCAT(resStr, tmpStr, strSize); 
			polar=(Polar*) obj->data; 
			for (i=0; i<obj->size; i++) {
				/* empty tmp str */
				memset(tmpStr, '\0', TMP_SIZE); 
				snprintf(tmpStr, TMP_SIZE, "{%g %g} ", polar[i].abs, polar[i].arg); 
				size=STRCAT(resStr, tmpStr, strSize); 
			}
			esweepObjPtr->length=size; 
			esweepObjPtr->bytes=ckalloc((unsigned) esweepObjPtr->length); 
			STRCPY(esweepObjPtr->bytes, resStr, esweepObjPtr->length); 
			ckfree(resStr); 

			break; 
		case SURFACE: 
			break; 
		default: 
			break; 
	}
}
#ifndef NOAUDIO

/*
 * Definition of tclEsweepAudioType
 */

static void DupEsweepAudio(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static void UpdateStringOfEsweepAudioObj(Tcl_Obj *objPtr); 
static void CloseEsweepAudio(Tcl_Obj *esweepAudioPtr);

const Tcl_ObjType tclEsweepAudioType = {
	ESWEEP_TYPE_NAME"_audio", /* name */
	CloseEsweepAudio, /* freeIntRepProc */
	DupEsweepAudio, /* dupIntRepProc */
	UpdateStringOfEsweepAudioObj, /* updateStringProc */
	NULL
	//SetFromAnyEsweepObj /* setFromAnyProc */
}; 


static void DupEsweepAudio(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr) {
	TclEsweepAudio *srcEa=(TclEsweepAudio*) srcPtr->internalRep.otherValuePtr; 
	copyPtr->internalRep.otherValuePtr=(void*) srcEa; 

	TCLESWEEPAUDIO_INCREFCOUNT(srcEa); 
}

static void UpdateStringOfEsweepAudioObj(Tcl_Obj *objPtr) {
	/* Create a string from the pointer
	 * We use at max 34+1 characters, this is enough for even 
	 * 128 bit long pointers */
	TclEsweepAudio *eaPtr=(TclEsweepAudio*) objPtr->internalRep.otherValuePtr; 
	char pstr[35]; 
	snprintf(pstr, sizeof(pstr), "%p", eaPtr->handle); 
	objPtr->length=strnlen(pstr, sizeof(pstr)); 
	objPtr->bytes=ckalloc((unsigned) objPtr->length); 
	STRCPY(objPtr->bytes, pstr, objPtr->length); 
}

static void CloseEsweepAudio(Tcl_Obj *objPtr) {
	TclEsweepAudio *eaPtr=(TclEsweepAudio*) objPtr->internalRep.otherValuePtr; 
	TCLESWEEPAUDIO_DECREFCOUNT(eaPtr); 
	if (eaPtr->refCount == 0) free(eaPtr); 
}
#endif
