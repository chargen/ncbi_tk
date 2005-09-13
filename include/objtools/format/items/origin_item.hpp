#ifndef OBJTOOLS_FORMAT_ITEMS___ORIGIN_ITEM__HPP
#define OBJTOOLS_FORMAT_ITEMS___ORIGIN_ITEM__HPP

/*  $Id$
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
* Author:  Mati Shomrat
*
* File Description:
*   Origin item for flat-file generator
*
*/
#include <corelib/ncbistd.hpp>

#include <objtools/format/items/item_base.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


class CBioseqContext;
class IFormatter;


///////////////////////////////////////////////////////////////////////////
//
// ORIGIN

class NCBI_FORMAT_EXPORT COriginItem : public CFlatItem
{
public:
    COriginItem(CBioseqContext& ctx);
    void Format(IFormatter& formatter, IFlatTextOStream& text_os) const;
    
    const string& GetOrigin(void) const { return m_Origin; }

private:
    void x_GatherInfo(CBioseqContext& ctx);

    // data
    string  m_Origin;
};


END_SCOPE(objects)
END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.3  2005/09/13 17:16:38  jcherry
* Added export specifiers
*
* Revision 1.2  2004/04/22 15:37:43  shomrat
* Changes in context
*
* Revision 1.1  2004/02/19 17:48:41  shomrat
* Initial Revision
*
*
* ===========================================================================
*/


#endif  /* OBJTOOLS_FORMAT_ITEMS___ORIGIN_ITEM__HPP */
