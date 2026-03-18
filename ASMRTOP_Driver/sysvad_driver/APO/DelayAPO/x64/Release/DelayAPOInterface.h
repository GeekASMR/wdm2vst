

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* Compiler settings for DelayAPOInterface.idl:
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

#ifndef __DelayAPOInterface_h__
#define __DelayAPOInterface_h__

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

#ifndef __IDelayAPOMFX_FWD_DEFINED__
#define __IDelayAPOMFX_FWD_DEFINED__
typedef interface IDelayAPOMFX IDelayAPOMFX;

#endif 	/* __IDelayAPOMFX_FWD_DEFINED__ */


#ifndef __IDelayAPOSFX_FWD_DEFINED__
#define __IDelayAPOSFX_FWD_DEFINED__
typedef interface IDelayAPOSFX IDelayAPOSFX;

#endif 	/* __IDelayAPOSFX_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "audioenginebaseapo.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IDelayAPOMFX_INTERFACE_DEFINED__
#define __IDelayAPOMFX_INTERFACE_DEFINED__

/* interface IDelayAPOMFX */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IDelayAPOMFX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4c0531b4-f2ca-4eae-80cd-020266923bc1")
    IDelayAPOMFX : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IDelayAPOMFXVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDelayAPOMFX * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDelayAPOMFX * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDelayAPOMFX * This);
        
        END_INTERFACE
    } IDelayAPOMFXVtbl;

    interface IDelayAPOMFX
    {
        CONST_VTBL struct IDelayAPOMFXVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDelayAPOMFX_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDelayAPOMFX_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDelayAPOMFX_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDelayAPOMFX_INTERFACE_DEFINED__ */


#ifndef __IDelayAPOSFX_INTERFACE_DEFINED__
#define __IDelayAPOSFX_INTERFACE_DEFINED__

/* interface IDelayAPOSFX */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IDelayAPOSFX;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6269b338-5162-4f14-a94d-8422ee93ec13")
    IDelayAPOSFX : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IDelayAPOSFXVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDelayAPOSFX * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDelayAPOSFX * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDelayAPOSFX * This);
        
        END_INTERFACE
    } IDelayAPOSFXVtbl;

    interface IDelayAPOSFX
    {
        CONST_VTBL struct IDelayAPOSFXVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDelayAPOSFX_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDelayAPOSFX_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDelayAPOSFX_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDelayAPOSFX_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


