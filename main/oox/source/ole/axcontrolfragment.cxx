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



#include "oox/ole/axcontrolfragment.hxx"

#include "oox/core/xmlfilterbase.hxx"
#include "oox/helper/binaryinputstream.hxx"
#include "oox/helper/binaryoutputstream.hxx"
#include "oox/ole/axcontrol.hxx"
#include "oox/ole/olehelper.hxx"
#include "oox/ole/olestorage.hxx"

namespace oox {
namespace ole {

// ============================================================================

using namespace ::com::sun::star::io;
using namespace ::com::sun::star::uno;

using ::oox::core::ContextHandler2;
using ::oox::core::ContextHandlerRef;
using ::oox::core::FragmentHandler2;
using ::oox::core::XmlFilterBase;
using ::rtl::OUString;

// ============================================================================

AxControlPropertyContext::AxControlPropertyContext( FragmentHandler2& rFragment, ControlModelBase& rModel ) :
    ContextHandler2( rFragment ),
    mrModel( rModel ),
    mnPropId( XML_TOKEN_INVALID )
{
}

ContextHandlerRef AxControlPropertyContext::onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs )
{
    switch( getCurrentElement() )
    {
        case AX_TOKEN( ocx ):
            if( nElement == AX_TOKEN( ocxPr ) )
            {
                mnPropId = rAttribs.getToken( AX_TOKEN( name ), XML_TOKEN_INVALID );
                switch( mnPropId )
                {
                    case XML_TOKEN_INVALID:
                        return 0;
                    case XML_Picture:
                    case XML_MouseIcon:
                        return this;        // import picture path from ax:picture child element
                    default:
                        mrModel.importProperty( mnPropId, rAttribs.getString( AX_TOKEN( value ), OUString() ) );
                }
            }
        break;

        case AX_TOKEN( ocxPr ):
            if( nElement == AX_TOKEN( picture ) )
            {
                OUString aPicturePath = getFragmentPathFromRelId( rAttribs.getString( R_TOKEN( id ), OUString() ) );
                if( aPicturePath.getLength() > 0 )
                {
                    BinaryXInputStream aInStrm( getFilter().openInputStream( aPicturePath ), true );
                    mrModel.importPictureData( mnPropId, aInStrm );
                }
            }
        break;
    }
    return 0;
}

// ============================================================================

AxControlFragment::AxControlFragment( XmlFilterBase& rFilter, const OUString& rFragmentPath, EmbeddedControl& rControl ) :
    FragmentHandler2( rFilter, rFragmentPath, true ),
    mrControl( rControl )
{
}

ContextHandlerRef AxControlFragment::onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs )
{
    if( isRootElement() && (nElement == AX_TOKEN( ocx )) )
    {
        OUString aClassId = rAttribs.getString( AX_TOKEN( classid ), OUString() );
        switch( rAttribs.getToken( AX_TOKEN( persistence ), XML_TOKEN_INVALID ) )
        {
            case XML_persistPropertyBag:
                if( ControlModelBase* pModel = mrControl.createModelFromGuid( aClassId ) )
                    return new AxControlPropertyContext( *this, *pModel );
            break;

            case XML_persistStreamInit:
            {
                OUString aFragmentPath = getFragmentPathFromRelId( rAttribs.getString( R_TOKEN( id ), OUString() ) );
                if( aFragmentPath.getLength() > 0 )
                {
                    BinaryXInputStream aInStrm( getFilter().openInputStream( aFragmentPath ), true );
                    if( !aInStrm.isEof() )
                    {
                        // binary stream contains a copy of the class ID, must be equal to attribute value
                        OUString aStrmClassId = OleHelper::importGuid( aInStrm );
                        OSL_ENSURE( aClassId.equalsIgnoreAsciiCase( aStrmClassId ),
                            "AxControlFragment::importBinaryControl - form control class ID mismatch" );
                        if( ControlModelBase* pModel = mrControl.createModelFromGuid( aStrmClassId ) )
                            pModel->importBinaryModel( aInStrm );
                    }
                }
            }
            break;

            case XML_persistStorage:
            {
                OUString aFragmentPath = getFragmentPathFromRelId( rAttribs.getString( R_TOKEN( id ), OUString() ) );
                if( aFragmentPath.getLength() > 0 )
                {
                    Reference< XInputStream > xStrgStrm = getFilter().openInputStream( aFragmentPath );
                    if( xStrgStrm.is() )
                    {
                        OleStorage aStorage( getFilter().getComponentContext(), xStrgStrm, false );
                        BinaryXInputStream aInStrm( aStorage.openInputStream( CREATE_OUSTRING( "f" ) ), true );
                        if( !aInStrm.isEof() )
                            if( AxContainerModelBase* pModel = dynamic_cast< AxContainerModelBase* >( mrControl.createModelFromGuid( aClassId ) ) )
                                pModel->importBinaryModel( aInStrm );
                    }
                }
            }
            break;
        }
    }
    return 0;
}

// ============================================================================

} // namespace ole
} // namespace oox
