

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* Compiler settings for SwapAPOInterface.idl:
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __SwapAPOInterface_h__
#define __SwapAPOInterface_h__

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

#ifndef __ISwapAPOMFX_FWD_DEFINED__
#define __ISwapAPOMFX_FWD_DEFINED__
typedef interface ISwapAPOMFX ISwapAPOMFX;

#endif 	/* __ISwapAPOMFX_FWD_DEFINED__ */


#ifndef __ISwapAPOSFX_FWD_DEFINED__
#define __ISwapAPOSFX_FWD_DEFINED__
typedef interface ISwapAPOSFX ISwapAPOSFX;

#endif 	/* __ISwapAPOSFX_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "audioenginebaseapo.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ISwapAPOMFX_INTERFACE_DEFINED__
#define __ISwapAPOMFX_INTERFACE_DEFINED__

/* interface ISwapAPOMFX */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISwapAPOMFX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3865B91A-096E-4ACA-BF56-B17D49C77406")
    ISwapAPOMFX : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ISwapAPOMFXVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ISwapAPOMFX * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ISwapAPOMFX * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ISwapAPOMFX * This);
        
        END_INTERFACE
    } ISwapAPOMFXVtbl;

    interface ISwapAPOMFX
    {
        CONST_VTBL struct ISwapAPOMFXVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISwapAPOMFX_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISwapAPOMFX_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISwapAPOMFX_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISwapAPOMFX_INTERFACE_DEFINED__ */


#ifndef __ISwapAPOSFX_INTERFACE_DEFINED__
#define __ISwapAPOSFX_INTERFACE_DEFINED__

/* interface ISwapAPOSFX */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISwapAPOSFX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("658A4077-B277-4d14-97E1-0356044D8110")
    ISwapAPOSFX : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ISwapAPOSFXVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ISwapAPOSFX * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ISwapAPOSFX * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ISwapAPOSFX * This);
        
        END_INTERFACE
    } ISwapAPOSFXVtbl;

    interface ISwapAPOSFX
    {
        CONST_VTBL struct ISwapAPOSFXVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISwapAPOSFX_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISwapAPOSFX_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISwapAPOSFX_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISwapAPOSFX_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


