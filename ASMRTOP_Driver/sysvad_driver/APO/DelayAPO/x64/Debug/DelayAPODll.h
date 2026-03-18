

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* Compiler settings for DelayAPODll.idl:
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


#ifndef __DelayAPODll_h__
#define __DelayAPODll_h__

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

#ifndef __DelayAPOMFX_FWD_DEFINED__
#define __DelayAPOMFX_FWD_DEFINED__

#ifdef __cplusplus
typedef class DelayAPOMFX DelayAPOMFX;
#else
typedef struct DelayAPOMFX DelayAPOMFX;
#endif /* __cplusplus */

#endif 	/* __DelayAPOMFX_FWD_DEFINED__ */


#ifndef __DelayAPOSFX_FWD_DEFINED__
#define __DelayAPOSFX_FWD_DEFINED__

#ifdef __cplusplus
typedef class DelayAPOSFX DelayAPOSFX;
#else
typedef struct DelayAPOSFX DelayAPOSFX;
#endif /* __cplusplus */

#endif 	/* __DelayAPOSFX_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "DelayAPOInterface.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __DelayAPODlllib_LIBRARY_DEFINED__
#define __DelayAPODlllib_LIBRARY_DEFINED__

/* library DelayAPODlllib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_DelayAPODlllib;

EXTERN_C const CLSID CLSID_DelayAPOMFX;

#ifdef __cplusplus

class DECLSPEC_UUID("b6c7032b-1f17-4cc6-bcdb-fd96deabc8a9")
DelayAPOMFX;
#endif

EXTERN_C const CLSID CLSID_DelayAPOSFX;

#ifdef __cplusplus

class DECLSPEC_UUID("77802b45-a5a0-455a-8204-3dba30eee7b4")
DelayAPOSFX;
#endif
#endif /* __DelayAPODlllib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


