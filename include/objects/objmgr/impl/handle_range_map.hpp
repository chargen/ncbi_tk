#ifndef HANDLE_RANGE_MAP__HPP
#define HANDLE_RANGE_MAP__HPP

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
* Author: Aleksey Grichenko, Michael Kimelman
*
* File Description:
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.4  2002/02/21 19:27:06  grichenk
* Rearranged includes. Added scope history. Added searching for the
* best seq-id match in data sources and scopes. Updated tests.
*
* Revision 1.3  2002/02/15 20:35:38  gouriano
* changed implementation of HandleRangeMap
*
* Revision 1.2  2002/01/23 21:59:31  grichenk
* Redesigned seq-id handles and mapper
*
* Revision 1.1  2002/01/11 19:06:20  gouriano
* restructured objmgr
*
*
* ===========================================================================
*/

#include "handle_range.hpp"
#include <corelib/ncbiobj.hpp>
#include <map>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


// Seq_loc substitution for internal use by iterators and data sources
class CHandleRangeMap
{
public:
    typedef map<CSeq_id_Handle, CHandleRange> TLocMap;

    CHandleRangeMap(CSeq_id_Mapper& id_mapper);
    CHandleRangeMap(const CHandleRangeMap& rmap);
    ~CHandleRangeMap(void);

    CHandleRangeMap& operator= (const CHandleRangeMap& rmap);

    // Add all ranges for each seq-id from a seq-loc
    void AddLocation(const CSeq_loc& loc);
    // Add ranges from "range" substituting their handle with "h"
    void AddRanges(const CSeq_id_Handle& h, const CHandleRange& range);
    // Get the ranges map
    const TLocMap& GetMap(void) const { return m_LocMap; }

    bool IntersectingWithLoc(const CSeq_loc& loc);
    bool IntersectingWithMap(const CHandleRangeMap& rmap);

private:
    // Split the location and add range lists to the locmap
    void x_ProcessLocation(const CSeq_loc& loc);

    CRef<CSeq_id_Mapper> m_IdMapper;
    TLocMap      m_LocMap;
};

END_SCOPE(objects)
END_NCBI_SCOPE

#endif  // HANDLE_RANGE_MAP__HPP
