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
 *   'seqloc.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.6  2002/06/07 11:24:24  clausen
 * Added related function comment
 *
 * Revision 1.5  2002/06/06 20:48:38  clausen
 * Moved IsValid to objects/util/sequence.hpp
 *
 * Revision 1.4  2002/05/03 21:28:04  ucko
 * Introduce T(Signed)SeqPos.
 *
 * Revision 1.3  2002/01/10 18:22:59  clausen
 * Added IsValid
 *
 * Revision 1.2  2001/06/25 18:52:02  grichenk
 * Prohibited copy constructor and assignment operator
 *
 * Revision 1.1  2000/11/17 21:35:02  vasilche
 * Added GetLength() method to CSeq_loc class.
 *
 *
 * ===========================================================================
 */

#ifndef OBJECTS_SEQLOC_SEQ_INTERVAL_HPP
#define OBJECTS_SEQLOC_SEQ_INTERVAL_HPP


// generated includes
#include <objects/seqloc/Seq_interval_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CSeq_interval : public CSeq_interval_Base
{
    typedef CSeq_interval_Base Tparent;
public:

    // See also related function in util/sequence.hpp:
    //
    //   bool IsValid(const CSeq_interval&, CScope*)
    //

    // constructor
    CSeq_interval(void);
    // destructor
    ~CSeq_interval(void);

    // Get the length of the interval
    TSeqPos GetLength(void) const;
        
private:
    // Prohibit copy constructor & assignment operator
    CSeq_interval(const CSeq_interval&);
    CSeq_interval& operator= (const CSeq_interval&);
};



/////////////////// CSeq_interval inline methods

// constructor
inline
CSeq_interval::CSeq_interval(void)
{
}

// length return
inline
TSeqPos CSeq_interval::GetLength(void) const
{
	return ((GetTo() - GetFrom()) + 1);
}

/////////////////// end of CSeq_interval inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_SEQLOC_SEQ_INTERVAL_HPP
