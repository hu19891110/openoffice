/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_embeddedobj.hxx"
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/EmbedMapUnits.hpp>
#include <com/sun/star/embed/EmbedMisc.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/embed/NoVisualAreaSizeException.hpp>

#include <rtl/logfile.hxx>

#include <oleembobj.hxx>
#include <olecomponent.hxx>
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/seqstream.hxx>

using namespace ::com::sun::star;
using namespace ::comphelper;

embed::VisualRepresentation OleEmbeddedObject::GetVisualRepresentationInNativeFormat_Impl(
					const uno::Reference< io::XStream > xCachedVisRepr )
		throw ( uno::Exception )
{
	embed::VisualRepresentation aVisualRepr;

	// TODO: detect the format in the future for now use workaround
	uno::Reference< io::XInputStream > xInStream = xCachedVisRepr->getInputStream();
	uno::Reference< io::XSeekable > xSeekable( xCachedVisRepr, uno::UNO_QUERY );
	if ( !xInStream.is() || !xSeekable.is() )
		throw uno::RuntimeException();

	uno::Sequence< sal_Int8 > aSeq( 2 );
	xInStream->readBytes( aSeq, 2 );
	xSeekable->seek( 0 );
	if ( aSeq.getLength() == 2 && aSeq[0] == 'B' && aSeq[1] == 'M' )
	{
		// it's a bitmap
		aVisualRepr.Flavor = datatransfer::DataFlavor(
            ::rtl::OUString::createFromAscii( "application/x-openoffice-bitmap;windows_formatname=\"Bitmap\"" ),
			::rtl::OUString::createFromAscii( "Bitmap" ),
			::getCppuType( (const uno::Sequence< sal_Int8 >*) NULL ) );
	}
	else
	{
		// it's a metafile
		aVisualRepr.Flavor = datatransfer::DataFlavor(
            ::rtl::OUString::createFromAscii( "application/x-openoffice-wmf;windows_formatname=\"Image WMF\"" ),
			::rtl::OUString::createFromAscii( "Windows Metafile" ),
			::getCppuType( (const uno::Sequence< sal_Int8 >*) NULL ) );
	}

	sal_Int32 nStreamLength = (sal_Int32)xSeekable->getLength();
	uno::Sequence< sal_Int8 > aRepresent( nStreamLength );
	xInStream->readBytes( aRepresent, nStreamLength );
	aVisualRepr.Data <<= aRepresent;

	return aVisualRepr;
}

void SAL_CALL OleEmbeddedObject::setVisualAreaSize( sal_Int64 nAspect, const awt::Size& aSize )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OleEmbeddedObject::setVisualAreaSize" );

    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setVisualAreaSize( nAspect, aSize );
        return;
    }
    // end wrapping related part ====================

	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	OSL_ENSURE( nAspect != embed::Aspects::MSOLE_ICON, "For iconified objects no graphical replacement is required!\n" );
	if ( nAspect == embed::Aspects::MSOLE_ICON )
		// no representation can be retrieved
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Illegal call!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_nObjectState == -1 )
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object is not loaded!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

#ifdef WNT
	// RECOMPOSE_ON_RESIZE misc flag means that the object has to be switched to running state on resize.
	// SetExtent() is called only for objects that require it,
	// it should not be called for MSWord documents to workaround problem i49369
	// If cached size is not set, that means that this is the size initialization, so there is no need to set the real size
	sal_Bool bAllowToSetExtent = 
	  ( ( getStatus( nAspect ) & embed::EmbedMisc::MS_EMBED_RECOMPOSEONRESIZE )
	  && !MimeConfigurationHelper::ClassIDsEqual( m_aClassID, MimeConfigurationHelper::GetSequenceClassID( 0x00020906L, 0x0000, 0x0000,
	  													 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 ) ) 
	  && m_bHasCachedSize );

	if ( m_nObjectState == embed::EmbedStates::LOADED && bAllowToSetExtent )
	{
		aGuard.clear();
		try {
			changeState( embed::EmbedStates::RUNNING );
		}
		catch( uno::Exception& )
		{
			OSL_ENSURE( sal_False, "The object should not be resized without activation!\n" );
		}
		aGuard.reset();
	}

	if ( m_pOleComponent && m_nObjectState != embed::EmbedStates::LOADED && bAllowToSetExtent )
	{
		awt::Size aSizeToSet = aSize;
		aGuard.clear();
		try {
			m_pOleComponent->SetExtent( aSizeToSet, nAspect ); // will throw an exception in case of failure
			m_bHasSizeToSet = sal_False;
		}
		catch( uno::Exception& )
		{
			// some objects do not allow to set the size even in running state
			m_bHasSizeToSet = sal_True;
			m_aSizeToSet = aSizeToSet;
			m_nAspectToSet = nAspect;
		}
		aGuard.reset();
	}
#endif

	// cache the values
	m_bHasCachedSize = sal_True;
	m_aCachedSize = aSize;
	m_nCachedAspect = nAspect;
}

awt::Size SAL_CALL OleEmbeddedObject::getVisualAreaSize( sal_Int64 nAspect )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OleEmbeddedObject::getVisualAreaSize" );

    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getVisualAreaSize( nAspect );
    }
    // end wrapping related part ====================

	::osl::ResettableMutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	OSL_ENSURE( nAspect != embed::Aspects::MSOLE_ICON, "For iconified objects no graphical replacement is required!\n" );
	if ( nAspect == embed::Aspects::MSOLE_ICON )
		// no representation can be retrieved
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Illegal call!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_nObjectState == -1 )
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object is not loaded!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	awt::Size aResult;

#ifdef WNT
	// TODO/LATER: Support different aspects
	if ( m_pOleComponent && !m_bHasSizeToSet && nAspect == embed::Aspects::MSOLE_CONTENT )
	{
		try
		{
			// the cached size updated every time the object is stored
			if ( m_bHasCachedSize )
			{
				aResult = m_aCachedSize;
			}
			else
			{
				// there is no internal cache
				awt::Size aSize;
				aGuard.clear();
	
				sal_Bool bSuccess = sal_False;
				if ( getCurrentState() == embed::EmbedStates::LOADED )
				{
					OSL_ENSURE( sal_False, "Loaded object has no cached size!\n" );

					// try to switch the object to RUNNING state and request the value again
					try {
						changeState( embed::EmbedStates::RUNNING );
					}
					catch( uno::Exception )
					{
						throw embed::NoVisualAreaSizeException(
								::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "No size available!\n" ) ),
								uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
					}
				}

				try
				{
					// first try to get size using replacement image
					aSize = m_pOleComponent->GetExtent( nAspect ); // will throw an exception in case of failure
					bSuccess = sal_True;
				}
				catch( uno::Exception& )
				{
				}

				if ( !bSuccess )
				{
					try
					{
						// second try the cached replacement image
						aSize = m_pOleComponent->GetCachedExtent( nAspect ); // will throw an exception in case of failure
						bSuccess = sal_True;
					}
					catch( uno::Exception& )
					{
					}
				}

				if ( !bSuccess )
				{
					try
					{
						// third try the size reported by the object
						aSize = m_pOleComponent->GetReccomendedExtent( nAspect ); // will throw an exception in case of failure
						bSuccess = sal_True;
					}
					catch( uno::Exception& )
					{
					}
				}

				if ( !bSuccess )
					throw embed::NoVisualAreaSizeException(
									::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "No size available!\n" ) ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

				aGuard.reset();
				
				m_aCachedSize = aSize;
				m_nCachedAspect = nAspect;
				m_bHasCachedSize = sal_True;

				aResult = m_aCachedSize;
			}
		}
		catch ( embed::NoVisualAreaSizeException& )
		{
			throw;
		}
		catch ( uno::Exception& )
		{
			throw embed::NoVisualAreaSizeException(
							::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "No size available!\n" ) ),
							uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
		}
	}
	else
#endif
	{
		// return cached value
		if ( m_bHasCachedSize )
		{
			OSL_ENSURE( nAspect == m_nCachedAspect, "Unexpected aspect is requested!\n" );
			aResult = m_aCachedSize;
		}
		else
		{
			throw embed::NoVisualAreaSizeException(
							::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "No size available!\n" ) ),
							uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
		}
	}

	return aResult;
}

embed::VisualRepresentation SAL_CALL OleEmbeddedObject::getPreferredVisualRepresentation( sal_Int64 nAspect )
		throw ( lang::IllegalArgumentException,
				embed::WrongStateException,
				uno::Exception,
				uno::RuntimeException )
{
	RTL_LOGFILE_CONTEXT( aLog, "embeddedobj (mv76033) OleEmbeddedObject::getPreferredVisualRepresentation" );

    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getPreferredVisualRepresentation( nAspect );
    }
    // end wrapping related part ====================

	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	OSL_ENSURE( nAspect != embed::Aspects::MSOLE_ICON, "For iconified objects no graphical replacement is required!\n" );
	if ( nAspect == embed::Aspects::MSOLE_ICON )
		// no representation can be retrieved
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Illegal call!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	// TODO: if the object has cached representation then it should be returned
	// TODO: if the object has no cached representation and is in loaded state it should switch itself to the running state
	if ( m_nObjectState == -1 )
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object is not loaded!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	embed::VisualRepresentation aVisualRepr;

	// TODO: in case of different aspects they must be applied to the mediatype and XTransferable must be used
	// the cache is used only as a fallback if object is not in loaded state
	if ( !m_xCachedVisualRepresentation.is() && ( !m_bVisReplInitialized || m_bVisReplInStream )
	  && m_nObjectState == embed::EmbedStates::LOADED )
	{
		m_xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( m_xObjectStream, sal_True );
		SetVisReplInStream( m_xCachedVisualRepresentation.is() );
	}

	if ( m_xCachedVisualRepresentation.is() )
	{
		return GetVisualRepresentationInNativeFormat_Impl( m_xCachedVisualRepresentation );
	}
#ifdef WNT
	else if ( m_pOleComponent )
	{
		try
		{
			if ( m_nObjectState == embed::EmbedStates::LOADED )
				changeState( embed::EmbedStates::RUNNING );

			datatransfer::DataFlavor aDataFlavor(
                	::rtl::OUString::createFromAscii( "application/x-openoffice-wmf;windows_formatname=\"Image WMF\"" ),
					::rtl::OUString::createFromAscii( "Windows Metafile" ),
					::getCppuType( (const uno::Sequence< sal_Int8 >*) NULL ) );

			aVisualRepr.Data = m_pOleComponent->getTransferData( aDataFlavor );
			aVisualRepr.Flavor = aDataFlavor;

			uno::Sequence< sal_Int8 > aVisReplSeq;
			aVisualRepr.Data >>= aVisReplSeq;
			if ( aVisReplSeq.getLength() )
			{
				m_xCachedVisualRepresentation = GetNewFilledTempStream_Impl( 
						uno::Reference< io::XInputStream > ( static_cast< io::XInputStream* > (
							new ::comphelper::SequenceInputStream( aVisReplSeq ) ) ) );
			}

			return aVisualRepr;
		}
		catch( uno::Exception& )
		{}
	}
#endif

	// the cache is used only as a fallback if object is not in loaded state
	if ( !m_xCachedVisualRepresentation.is() && ( !m_bVisReplInitialized || m_bVisReplInStream ) )
	{
		m_xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( m_xObjectStream );
		SetVisReplInStream( m_xCachedVisualRepresentation.is() );
	}

	if ( !m_xCachedVisualRepresentation.is() )
	{
		// no representation can be retrieved
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Illegal call!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );
	}

	return GetVisualRepresentationInNativeFormat_Impl( m_xCachedVisualRepresentation );
}

sal_Int32 SAL_CALL OleEmbeddedObject::getMapUnit( sal_Int64 nAspect )
		throw ( uno::Exception,
				uno::RuntimeException)
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbeddedObject > xWrappedObject = m_xWrappedObject;
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getMapUnit( nAspect );
    }
    // end wrapping related part ====================

	::osl::MutexGuard aGuard( m_aMutex );
	if ( m_bDisposed )
		throw lang::DisposedException(); // TODO

	OSL_ENSURE( nAspect != embed::Aspects::MSOLE_ICON, "For iconified objects no graphical replacement is required!\n" );
	if ( nAspect == embed::Aspects::MSOLE_ICON )
		// no representation can be retrieved
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "Illegal call!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	if ( m_nObjectState == -1 )
		throw embed::WrongStateException( ::rtl::OUString::createFromAscii( "The object is not loaded!\n" ),
									uno::Reference< uno::XInterface >( static_cast< ::cppu::OWeakObject* >(this) ) );

	return embed::EmbedMapUnits::ONE_100TH_MM;
}


