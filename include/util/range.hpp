#ifndef RANGE__HPP
#define RANGE__HPP

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
* Author: Eugene Vasilchenko
*
* File Description:
*   CRange<> class represents interval
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbi_limits.hpp>


/** @addtogroup RangeSupport
 *
 * @{
 */


BEGIN_NCBI_SCOPE

// range
template<class Position>
class COpenRange
{
public:
    typedef Position position_type;
    typedef COpenRange<Position> TThisType;

    // constructors
    COpenRange(void)
        : m_From(GetEmptyFrom()), m_ToOpen(GetEmptyToOpen())
        {
        }
    COpenRange(position_type from, position_type toOpen)
        : m_From(from), m_ToOpen(toOpen)
        {
        }
    
    // parameters
    position_type GetFrom(void) const
        {
            return m_From;
        }
    position_type GetToOpen(void) const
        {
            return m_ToOpen;
        }
    position_type GetTo(void) const
        {
            return GetToOpen()-1;
        }

    // state
    bool Empty(void) const
        {
            return GetToOpen() <= GetFrom();
        }
    bool NotEmpty(void) const
        {
            return GetToOpen() > GetFrom();
        }
    // return length of regular region
    position_type GetLength(void) const
        {
            position_type from = GetFrom(), toOpen = GetToOpen();
            if ( toOpen <= from )
                return 0;
            position_type len = toOpen - from;
            if ( len < 0 )
                len = GetWholeLength();
            return len;
        }

    // modifiers
    TThisType& SetFrom(position_type from)
        {
            m_From = from;
            return *this;
        }
    TThisType& SetToOpen(position_type toOpen)
        {
            m_ToOpen = toOpen;
            return *this;
        }
    TThisType& SetTo(position_type to)
        {
            return SetToOpen(to+1);
        }
    TThisType& SetOpen(position_type from, position_type toOpen)
        {
            return SetFrom(from).SetToOpen(toOpen);
        }
    TThisType& Set(position_type from, position_type to)
        {
            return SetFrom(from).SetTo(to);
        }

    // length must be >= 0
    TThisType& SetLength(position_type length)
        {
            _ASSERT(length >= 0);
            position_type from = GetFrom();
            position_type toOpen = from + length;
            if ( toOpen < from )
                toOpen = GetWholeToOpen();
            return SetToOpen(toOpen);
        }
    // length must be >= 0
    TThisType& SetLengthDown(position_type length)
        {
            _ASSERT(length >= 0);
            position_type toOpen = GetToOpen();
            position_type from = toOpen - length;
            if ( from > toOpen )
                from = GetWholeFrom();
            return SetFrom(from);
        }

    // comparison
    bool operator==(const TThisType& r) const
        {
            return GetFrom() == r.GetFrom() && GetToOpen() == r.GetToOpen();
        }
    bool operator!=(const TThisType& r) const
        {
            return !(*this == r);
        }
    bool operator<(const TThisType& r) const
        {
            return GetFrom() < r.GetFrom() ||
                GetFrom() == r.GetFrom() && GetToOpen() < r.GetToOpen();
        }
    bool operator<=(const TThisType& r) const
        {
            return GetFrom() < r.GetFrom() ||
                GetFrom() == r.GetFrom() && GetToOpen() <= r.GetToOpen();
        }
    bool operator>(const TThisType& r) const
        {
            return GetFrom() > r.GetFrom() ||
                GetFrom() == r.GetFrom() && GetToOpen() > r.GetToOpen();
        }
    bool operator>=(const TThisType& r) const
        {
            return GetFrom() > r.GetFrom() ||
                GetFrom() == r.GetFrom() && GetToOpen() >= r.GetToOpen();
        }


    // special values
    static position_type GetPositionMin(void)
        {
            return numeric_limits<position_type>::min();
        }
    static position_type GetPositionMax(void)
        {
            return numeric_limits<position_type>::max();
        }

    // whole range
    static position_type GetWholeFrom(void)
        {
            return GetPositionMin();
        }
    static position_type GetWholeToOpen(void)
        {
            return GetPositionMax();
        }
    static position_type GetWholeTo(void)
        {
            return GetWholeToOpen()-1;
        }
    static position_type GetWholeLength(void)
        {
            return GetPositionMax();
        }
    static TThisType GetWhole(void)
        {
            return TThisType(GetWholeFrom(), GetWholeToOpen());
        }
    bool IsWholeFrom(void) const
        {
            return GetFrom() == GetWholeFrom();
        }
    bool IsWholeTo(void) const
        {
            return GetToOpen() == GetWholeToOpen();
        }
    bool IsWhole(void) const
        {
            return IsWholeFrom() && IsWholeTo();
        }

    // empty range
    static position_type GetEmptyFrom(void)
        {
            return GetPositionMax();
        }
    static position_type GetEmptyToOpen(void)
        {
            return GetPositionMax();
        }
    static position_type GetEmptyTo(void)
        {
            return GetEmptyToOpen()-1;
        }
    static position_type GetEmptyLength(void)
        {
            return 0;
        }
    static TThisType GetEmpty(void)
        {
            return TThisType(GetEmptyFrom(), GetEmptyToOpen());
        }

    // intersecting ranges
    TThisType IntersectionWith(const TThisType& r) const
        {
            return TThisType(max(GetFrom(), r.GetFrom()),
                             min(GetToOpen(), r.GetToOpen()));
        }
    TThisType IntersectWith(const TThisType& r)
        {
            m_From = max(GetFrom(), r.GetFrom());
            m_ToOpen = min(GetToOpen(), r.GetToOpen());
            return *this;
        }
    TThisType operator&(const TThisType& r) const
        {
            return IntersectionWith(r);
        }
    TThisType& operator&=(const TThisType& r)
        {
            return IntersectWith(r);
        }
    bool IntersectingWith(const TThisType& r) const
        {
            return IntersectionWith(r).NotEmpty();
        }

    // combine ranges
    TThisType& CombineWith(const TThisType& r)
        {
            if ( !r.Empty() ) {
                if ( !Empty() ) {
                    m_From = min(m_From, r.GetFrom());
                    m_ToOpen = max(m_ToOpen, r.GetToOpen());
                }
                else {
                    *this = r;
                }
            }
            return *this;
        }
    TThisType CombinationWith(const TThisType& r) const
        {
            if ( !r.Empty() ) {
                if ( !Empty() ) {
                    return TThisType(min(m_From, r.GetFrom()),
                                     max(m_ToOpen, r.GetToOpen()));
                }
                else {
                    return r;
                }
            }
            return *this;
        }
    TThisType& operator+=(const TThisType& r)
        {
            return CombineWith(r);
        }
    TThisType operator+(const TThisType& r) const
        {
            return CombinationWith(r);
        }

private:
    position_type m_From, m_ToOpen;
};


// range
template<class Position>
class CRange : public COpenRange<Position>
{
    typedef COpenRange<Position> TParent;
public:
    typedef typename TParent::position_type position_type;
    typedef CRange<Position> TThisType;

    // constructors
    CRange(void)
        {
        }
    CRange(position_type from, position_type to)
        : TParent(from, to+1)
        {
        }
    CRange(const TParent& range)
        : TParent(range)
        {
        }
    
    // modifiers
    TThisType& operator=(const TParent& range)
        {
            static_cast<TParent&>(*this) = range;
            return *this;
        }
};

///
/// typedefs for sequence ranges
///

typedef CRange<TSeqPos>       TSeqRange;
typedef CRange<TSignedSeqPos> TSignedSeqRange;

/* @} */


//#include <util/range.inl>

END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.15  2003/10/27 16:57:59  dicuccio
* Added typedefs for T{Signed}SeqRange
*
* Revision 1.14  2003/04/17 17:50:24  siyan
* Added doxygen support
*
* Revision 1.13  2003/02/07 16:54:01  vasilche
* Pass all structures with size > sizeof int by reference.
* Move cvs log to the end of files.
*
* Revision 1.12  2003/01/23 21:24:22  vasilche
* Fixed reference to operator=() on IRIX.
*
* Revision 1.11  2003/01/22 21:14:52  vasilche
* Added missing typename.
*
* Revision 1.10  2003/01/22 20:05:24  vasilche
* Simplified CRange<> implementation.
* Removed special handling of Empty & Whole bounds.
* Added simplier COpenRange<> template.
*
* Revision 1.9  2002/12/20 20:53:28  grichenk
* Removed range normalization
*
* Revision 1.8  2002/12/19 20:24:06  grichenk
* Added normalization of intervals (from <= to).
* Removed SetFrom() and SetTo(), added Set().
*
* Revision 1.7  2002/06/04 19:36:33  ucko
* More fixes for empty ranges; CSeq_loc::GetTotalRange() now works again.
*
* Revision 1.6  2002/05/24 14:56:14  grichenk
* Fixed Empty() for unsigned intervals
*
* Revision 1.5  2002/04/22 20:02:33  grichenk
* Fixed CombineFrom(), CombineTo(), operator+=()
*
* Revision 1.4  2001/09/05 14:50:28  grichenk
* Fixed comparison of "whole" ranges
*
* Revision 1.3  2001/01/05 20:08:53  vasilche
* Added util directory for various algorithms and utility classes.
*
* Revision 1.2  2001/01/03 17:24:52  vasilche
* Fixed typo.
*
* Revision 1.1  2001/01/03 16:39:18  vasilche
* Added CAbstractObjectManager - stub for object manager.
* CRange extracted to separate file.
*
* ===========================================================================
*/

#endif  /* RANGE__HPP */
