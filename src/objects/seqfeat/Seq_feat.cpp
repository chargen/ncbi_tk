/* $Id$
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
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'seqfeat.asn'.
 */

// generated includes
#include <objects/seqfeat/Seq_feat.hpp>

#include <objects/seqloc/Seq_loc.hpp>


BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


// destructor
CSeq_feat::~CSeq_feat(void)
{
}

static int s_SeqFeatTypeOrder[] = {
    3, // e_not_set = 0,
    0, // e_Gene,
    3, // e_Org,
    2, // e_Cdregion,
    3, // e_Prot,
    1, // e_Rna,
    3  // e_Pub, and the rest
};

// Corresponds to SortFeatItemListByPos from the C toolkit
int CSeq_feat::x_CompareLong(const CSeq_feat& f2,
                             const CSeq_loc& loc1, const CSeq_loc& loc2) const
{
    CSeqFeatData::E_Choice type1 = GetData().Which();
    CSeqFeatData::E_Choice type2 = f2.GetData().Which();

    {{ // order by feature type
        const size_t MAX_TYPE =
            sizeof(s_SeqFeatTypeOrder)/sizeof(s_SeqFeatTypeOrder[0]);
        int order1 = size_t(type1) < MAX_TYPE?
            s_SeqFeatTypeOrder[type1]: s_SeqFeatTypeOrder[MAX_TYPE-1];
        int order2 = size_t(type2) < MAX_TYPE?
            s_SeqFeatTypeOrder[type2]: s_SeqFeatTypeOrder[MAX_TYPE-1];
        int diff = order1 - order2;
        if ( diff != 0 )
            return diff;
    }}

    // compare internal intervals
    if ( loc1.IsMix()  &&  loc2.IsMix() ) {
        const CSeq_loc_mix::Tdata& ivals1 = loc1.GetMix().Get();
        const CSeq_loc_mix::Tdata& ivals2 = loc2.GetMix().Get();
        for ( CSeq_loc_mix::Tdata::const_iterator
                  it1 = ivals1.begin(), it2 = ivals2.begin();
              it1 != ivals1.end()  &&  it2 != ivals2.end();  it1++, it2++) {
            int diff = CompareLocations(**it1, **it2);
            if ( diff != 0 )
                return diff;
        }
    }

    // compare frames of identical CDS ranges
    if (type1 == CSeqFeatData::e_Cdregion) {
        _ASSERT(type2 == CSeqFeatData::e_Cdregion);
        CCdregion::EFrame frame1 = GetData().GetCdregion().GetFrame();
        CCdregion::EFrame frame2 = f2.GetData().GetCdregion().GetFrame();
        if (frame1 > CCdregion::eFrame_one
            ||  frame2 > CCdregion::eFrame_one) {
            int diff = frame1 - frame2;
            if ( diff != 0 )
                return diff;
        }
    }

    {{ // compare subtypes
        CSeqFeatData::ESubtype subtype1 = GetData().GetSubtype();
        CSeqFeatData::ESubtype subtype2 = f2.GetData().GetSubtype();
        int diff = subtype1 - subtype2;
        if ( diff != 0 )
            return diff;
    }}

    // XXX - should compare feature content labels and parent seq-annots

    return 0; // unknown
}

const CGene_ref* CSeq_feat::GetGeneXref(void) const

{
    iterate(CSeq_feat::TXref, it, GetXref ()) {
        if ((*it)->IsSetData () && (*it)->GetData ().IsGene ()) {
            return &((*it)->GetData ().GetGene ());
        }
    }
    return 0;
}

const CProt_ref* CSeq_feat::GetProtXref(void) const

{
    iterate(CSeq_feat::TXref, it, GetXref ()) {
        if ((*it)->IsSetData () && (*it)->GetData ().IsProt ()) {
            return &((*it)->GetData ().GetProt ());
        }
    }
    return 0;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 6.11  2003/02/24 18:52:57  vasilche
 * Added optional mapped locations arguments to feature comparison.
 *
 * Revision 6.10  2003/02/10 15:52:08  grichenk
 * CSeq_feat::Compare() takes optional seq-locs for remapped features
 *
 * Revision 6.9  2003/02/06 22:24:23  vasilche
 * Added int CSeq_feat::Compare().
 * Fixed slow comparison of CSeq_feat with mix seq locs.
 *
 * Revision 6.8  2003/01/29 17:43:23  vasilche
 * Added Compare(CSeq_feat, CSeq_feat) returning int for easier comparison.
 * operator<(CSeq_feat, CSeq_feat) uses Compare().
 *
 * Revision 6.7  2002/12/19 21:31:29  kans
 * added GetGeneXref and GetProtXref
 *
 * Revision 6.6  2002/09/18 18:31:11  ucko
 * operator <: Remove bogus comparison of feature type labels.
 * operator <: Mention content labels in to-do comment.
 * general: Move CVS log to end per current practice.
 *
 * Revision 6.5  2002/06/06 20:55:48  clausen
 * Moved GetLabel to objects/util/feature.cpp
 *
 * Revision 6.4  2002/05/06 03:39:11  vakatov
 * OM/OM1 renaming
 *
 * Revision 6.3  2002/01/10 19:53:48  clausen
 * Added GetLabel
 *
 * Revision 6.2  2002/01/09 15:59:28  grichenk
 * Fixed includes
 *
 * Revision 6.1  2001/10/30 20:25:58  ucko
 * Implement feature labels/keys, subtypes, and sorting
 *
 *
 * ===========================================================================
 */

/* Original file checksum: lines: 61, chars: 1885, CRC32: 417ca6e0 */
