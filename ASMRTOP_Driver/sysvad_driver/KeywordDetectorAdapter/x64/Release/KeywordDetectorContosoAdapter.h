

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* Compiler settings for KeywordDetectorContosoAdapter.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __KeywordDetectorContosoAdapter_h__
#define __KeywordDetectorContosoAdapter_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __KeywordDetectorContosoAdapter_FWD_DEFINED__
#define __KeywordDetectorContosoAdapter_FWD_DEFINED__

#ifdef __cplusplus
typedef class KeywordDetectorContosoAdapter KeywordDetectorContosoAdapter;
#else
typedef struct KeywordDetectorContosoAdapter KeywordDetectorContosoAdapter;
#endif /* __cplusplus */

#endif 	/* __KeywordDetectorContosoAdapter_FWD_DEFINED__ */


#ifndef __EventDetectorContosoAdapter_FWD_DEFINED__
#define __EventDetectorContosoAdapter_FWD_DEFINED__

#ifdef __cplusplus
typedef class EventDetectorContosoAdapter EventDetectorContosoAdapter;
#else
typedef struct EventDetectorContosoAdapter EventDetectorContosoAdapter;
#endif /* __cplusplus */

#endif 	/* __EventDetectorContosoAdapter_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "KeywordDetectorOemAdapter.h"
#include "EventDetectorOemAdapter.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __KeywordDetectorContosoAdapterLib_LIBRARY_DEFINED__
#define __KeywordDetectorContosoAdapterLib_LIBRARY_DEFINED__

/* library KeywordDetectorContosoAdapterLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_KeywordDetectorContosoAdapterLib;

EXTERN_C const CLSID CLSID_KeywordDetectorContosoAdapter;

#ifdef __cplusplus

class DECLSPEC_UUID("6F7DBCC1-202E-498D-99C5-61C36C4EB2DC")
KeywordDetectorContosoAdapter;
#endif

EXTERN_C const CLSID CLSID_EventDetectorContosoAdapter;

#ifdef __cplusplus

class DECLSPEC_UUID("207F3D0C-5C79-496F-A94C-D3D2934DBFA9")
EventDetectorContosoAdapter;
#endif
#endif /* __KeywordDetectorContosoAdapterLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


