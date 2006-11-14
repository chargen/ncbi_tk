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
* Author:  Kamen Todorov, NCBI
*
* File Description:
*   Collection of diagonal alignment ranges.
*
* ===========================================================================
*/


#include <ncbi_pch.hpp>

#include <objtools/alnmgr/diag_rng_coll.hpp>
#include <algorithm>


BEGIN_NCBI_SCOPE


CDiagRngColl::CDiagRngColl() :
    TAlnRngColl(TAlnRngColl::fKeepNormalized | TAlnRngColl::fAllowMixedDir),
    m_Extender(*this)
{
};


void CDiagRngColl::Diff(const TAlnRngColl& subtrahend,
                        TAlnRngColl& difference) const
{
    if (empty()) {
        ITERATE (TAlnRngColl, subtrahend_it, subtrahend) {
            difference.insert(*subtrahend_it);
        }
        return;
    }

    TAlnRngColl difference_on_first;
    {
        TAlnRngColl::const_iterator minuend_it = begin();
        ITERATE (TAlnRngColl, subtrahend_it, subtrahend) {
            x_Diff(*subtrahend_it,
                   difference_on_first,
                   minuend_it);
        }
    }

    {
        m_Extender.Init(*this);  /// HACK!
        m_Extender.UpdateIndex();
        TAlnRngCollExt::const_iterator minuend_it = m_Extender.begin();
        TAlnRngCollExt diff_on_first_ext(difference_on_first);
        diff_on_first_ext.UpdateIndex();
        ITERATE (TAlnRngCollExt,
                 subtrahend_it,
                 diff_on_first_ext) {
            x_DiffSecond(*(subtrahend_it->second),
                         difference,
                         minuend_it);
        }
    }
}


void CDiagRngColl::x_Diff(const TAlnRng& rng,
                          TAlnRngColl&   result,
                          TAlnRngColl::const_iterator& r_it) const
{
    TAlnRngColl::PRangeLess<TAlnRng> p;

    r_it = std::lower_bound(r_it, end(), rng.GetFirstFrom(), p); /* NCBI_FAKE_WARNING: WorkShop */

    if (r_it == end()) {
        result.insert(rng);
        return;
    }

    int trim; // whether and how much to trim when truncating

    trim = (r_it->GetFirstFrom() <= rng.GetFirstFrom());

    TAlnRng r = rng;
    TAlnRng tmp_r;

    while (1) {
        if (trim) {
            // x--------)
            //  ...---...
            trim = r_it->GetFirstToOpen() - r.GetFirstFrom();
            TrimFirstFrom(r, trim);
            if ((int) r.GetLength() <= 0) {
                return;
            }
            ++r_it;
            if (r_it == end()) {
                result.insert(r);
                return;
            }
        }

        //      x------)
        // x--...
        trim = r.GetFirstToOpen() - r_it->GetFirstFrom();

        if (trim <= 0) {
            //     x----)
            // x--)
            result.insert(r);
            return;
        } else {
            //     x----)
            // x----...
            tmp_r = r;
            TrimFirstTo(tmp_r, trim);
            result.insert(tmp_r);
        }
    }
}


void CDiagRngColl::x_DiffSecond(const TAlnRng& rng,
                                TAlnRngColl&   result,
                                TAlnRngCollExt::const_iterator& r_it) const
{
    PItLess p;

    m_Extender.UpdateIndex();
    r_it = std::lower_bound(r_it, m_Extender.end(), rng.GetSecondFrom(), p); /* NCBI_FAKE_WARNING: WorkShop */
    if (r_it == m_Extender.end()) {
        result.insert(rng);
        return;
    }

    int trim; // whether and how much to trim when truncating

    trim = (r_it->second->GetSecondFrom() <= rng.GetSecondFrom());

    TAlnRng r = rng;
    TAlnRng tmp_r;

    while (1) {
        if (trim) {
            // x--------)
            //  ...---...
            trim = r_it->second->GetSecondToOpen() - r.GetSecondFrom();
            TrimSecondFrom(r, trim);
            if ((int) r.GetLength() <= 0) {
                return;
            }
            ++r_it;
            if (r_it == m_Extender.end()) {
                result.insert(r);
                return;
            }
        }

        //      x------)
        // x--...
        trim = r.GetSecondToOpen() - r_it->second->GetSecondFrom();

        if (trim <= 0) {
            //     x----)
            // x--)
            result.insert(r);
            return;
        } else {
            //     x----)
            // x----...
            tmp_r = r;
            TrimSecondTo(tmp_r, trim);
            result.insert(tmp_r);
        }
    }
}


END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.4  2006/11/14 20:36:34  todorov
* Diff is const.
*
* Revision 1.3  2006/11/06 19:56:35  todorov
* Eliminated basewidths.  Positions are stored in pseudo coords.
*
* Revision 1.2  2006/10/19 17:17:36  todorov
* Include file fix.
*
* Revision 1.1  2006/10/19 17:07:23  todorov
* Initial revision.
*
* ===========================================================================
*/
