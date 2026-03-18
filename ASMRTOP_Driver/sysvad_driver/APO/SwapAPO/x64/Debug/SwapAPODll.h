

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* Compiler settings for SwapAPODll.idl:
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


#ifndef __SwapAPODll_h__
#define __SwapAPODll_h__

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

#ifndef __SwapAPOMFX_FWD_DEFINED__
#define __SwapAPOMFX_FWD_DEFINED__

#ifdef __cplusplus
typedef class SwapAPOMFX SwapAPOMFX;
#else
typedef struct SwapAPOMFX SwapAPOMFX;
#endif /* __cplusplus */

#endif 	/* __SwapAPOMFX_FWD_DEFINED__ */


#ifndef __SwapAPOSFX_FWD_DEFINED__
#define __SwapAPOSFX_FWD_DEFINED__

#ifdef __cplusplus
typedef class SwapAPOSFX SwapAPOSFX;
#else
typedef struct SwapAPOSFX SwapAPOSFX;
#endif /* __cplusplus */

#endif 	/* __SwapAPOSFX_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "SwapAPOInterface.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __SwapAPODlllib_LIBRARY_DEFINED__
#define __SwapAPODlllib_LIBRARY_DEFINED__

/* library SwapAPODlllib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_SwapAPODlllib;

EXTERN_C const CLSID CLSID_SwapAPOMFX;

#ifdef __cplusplus

class DECLSPEC_UUID("06687E71-F043-403A-BF49-CB591BA6E103")
SwapAPOMFX;
#endif

EXTERN_C const CLSID CLSID_SwapAPOSFX;

#ifdef __cplusplus

class DECLSPEC_UUID("B48DEA3F-D962-425a-8D9A-9A5BB37A9904")
SwapAPOSFX;
#endif
#endif /* __SwapAPODlllib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


