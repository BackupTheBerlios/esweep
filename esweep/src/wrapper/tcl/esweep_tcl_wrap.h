/* 
 * Tcl-Module as a wrapper around the esweep library
 * the functions are added when they are needed
 */

#ifndef DLL_EXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#	define DLL_EXPORT  extern __declspec(dllexport)
# else
# 	define DLL_EXPORT 
# endif
#endif

#ifndef STDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define STDCALL __stdcall
# else
#   define STDCALL
# endif 
#endif

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <tcl.h>
#include "../../esweep.h"
#include "../../esweep_priv.h"

#ifndef __ESWEEP_TCL_WRAP_H
#define __ESWEEP_TCL_WRAP_H

/* often used macros */

/*
 * Check if the parameter at index idx is an esweep object and if so, set obj to the internal representation 
 * If necessary, the object will be duplicated. 
 * The esweep function wrappers will not share objects by themselves. But they have to respect
 * sharing done by the Tcl interpreter itself. 
 */
#define CHECK_ESWEEP_OBJECT(idx, obj) if (objv[idx]->typePtr != &tclEsweepObjType) { \
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Parameter %i not an esweep object", idx)); \
			return TCL_ERROR; \
		} else {obj=(esweep_object*) objv[idx]->internalRep.otherValuePtr;}

/*
 * same as above, but it takes objv[idx] as a name for the variable
 * This resembles upvar
 */
#define CHECK_ESWEEP_OBJECT2(idx, tclObj, obj) \
	tclObj=Tcl_ObjGetVar2(interp, objv[idx], NULL, TCL_LEAVE_ERR_MSG); \
	if (tclObj == NULL) {return TCL_ERROR;} \
	if (tclObj->typePtr != &tclEsweepObjType) { \
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Named variable %s not an esweep object", Tcl_GetString(objv[idx]))); \
			return TCL_ERROR; \
	} else {obj=(esweep_object*) tclObj->internalRep.otherValuePtr;}

/* Again, this time for the esweep_audio objects */
#define CHECK_ESWEEP_AUDIO(idx, obj) if (objv[idx]->typePtr != &tclEsweepAudioType) { \
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Parameter %i not an esweep audio object", idx)); \
			return TCL_ERROR; \
		} else {obj=(esweep_audio*) ((TclEsweepAudio*) (objv[idx]->internalRep.otherValuePtr))->handle;}

/* 
 * execute the esweep function and, if failed, create an error message and return TCL_ERROR; 
 */
#define ESWEEP_TCL_ASSERT(expr) if (!(expr)) { \
					Tcl_SetObjResult(interp, Tcl_NewStringObj(errmsg, -1)); \
					return TCL_ERROR; \
				}

/*
 * check if the option is set; idx is a run variable
 */
#define CHECK_MISSING_OPTIONS(opts, mask, idx)  \
	for (idx=0; opts[idx]!=NULL; idx++) { \
		if (mask[idx]) { \
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("missing option: %s", opts[idx])); \
			return TCL_ERROR; \
		} \
	}


/* 
 * Check for the right number of arguments and construct an error message
 */
#define CHECK_NUM_ARGS(expr, args) if (!(expr)) { \
		Tcl_WrongNumArgs(interp, 1, objv, args); \
		return TCL_ERROR; \
	}

/* 
 * Duplicate an esweep object when needed 
 * The esweep function wrappers will not share objects by themselves. But they have to respect
 * sharing done by the Tcl interpreter itself. 
 */
//#define DUPLICATE_WHEN_SHARED(tclObj, esweepObj) if (Tcl_IsShared(tclObj)) {tclObj = Tcl_DuplicateObj(tclObj); esweepObj=(esweep_object*) tclObj->internalRep.otherValuePtr;}
#define DUPLICATE_WHEN_SHARED(tclObj, esweepObj)

extern const Tcl_ObjType tclEsweepObjType;
extern const Tcl_ObjType tclEsweepAudioType; 

/* this struct is needed to allow Tcl work flawless with the tclEsweepAudioType */
typedef struct {
	int refCount; 
	esweep_audio *handle; 
} TclEsweepAudio; 

#define TCLESWEEPAUDIO_INCREFCOUNT(ea) ea->refCount++;
#define TCLESWEEPAUDIO_DECREFCOUNT(ea) if (ea->refCount > 0) { \
						ea->refCount--; \
						if (ea->refCount == 0) { \
							esweep_audioClose(ea->handle); \
						} \
					}

/* Command prototypes */

/* mem */
int esweepCreate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepClone(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSparseSurface(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepMove(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 
int esweepCopy(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 

/* base */
int esweepGet(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepIndex(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSize(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepType(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSamplerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSetSamplerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* conv */
int esweepToWave(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 
int esweepToComplex(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 
int esweepToPolar(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 

/* generate */
int esweepGenerate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 

/* dsp */
int esweepDeconvolve(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]); 
int esweepFFT(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepIFFT(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepCreateFFTTable(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepDelay(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSmooth(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepUnwrapPhase(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepWrapPhase(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepRestoreHermitian(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepWindow(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepPeakDetect(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepIntegrate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepDifferentiate(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* filter */
int esweepCreateFilterFromCoeff(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepCreateFilterFromList(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAppendFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepCloneFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepResetFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepFilter(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* file */
int esweepToAscii(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepLoad(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSave(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* math */
int esweepExpr(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepMin(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepMax(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepMinPos(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepMaxPos(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAvg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSum(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSqsum(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepReal(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepImag(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAbs(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepArg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepLg(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepLn(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepExp(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepPow(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepSchroeder(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* audio */
int esweepAudioOpen(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioQuery(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioConfigure(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioSync(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioPause(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioOut(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioIn(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
int esweepAudioClose(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* display */
int esweepGetCoords(ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);  
#endif
