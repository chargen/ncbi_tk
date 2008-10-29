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
 */

/// @file Spliced_exon.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'seqalign.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Spliced_exon_.hpp


#ifndef OBJECTS_SEQALIGN_SPLICED_EXON_HPP
#define OBJECTS_SEQALIGN_SPLICED_EXON_HPP


// generated includes
#include <objects/seqalign/Spliced_exon_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class NCBI_SEQALIGN_EXPORT CSpliced_exon : public CSpliced_exon_Base
{
    typedef CSpliced_exon_Base Tparent;
public:
    // constructor
    CSpliced_exon(void);
    // destructor
    ~CSpliced_exon(void);

    /// @name Deprecated APIs
    /// These APIs map to an older version of the ASN spec and are designed
    /// to help migrate code to the newer spec
    /// @{

    typedef TAcceptor_before_exon TSplice_5_prime;
    typedef TDonor_after_exon     TSplice_3_prime;

    NCBI_DEPRECATED const TAcceptor_before_exon& GetSplice_5_prime() const;
    NCBI_DEPRECATED TAcceptor_before_exon& SetSplice_5_prime();
    NCBI_DEPRECATED void SetSplice_5_prime(TAcceptor_before_exon& splice);
    NCBI_DEPRECATED bool IsSetSplice_5_prime() const;
    NCBI_DEPRECATED bool CanGetSplice_5_prime() const;
    NCBI_DEPRECATED void ResetSplice_5_prime();

    NCBI_DEPRECATED const TDonor_after_exon& GetSplice_3_prime() const;
    NCBI_DEPRECATED TDonor_after_exon& SetSplice_3_prime();
    NCBI_DEPRECATED void SetSplice_3_prime(TDonor_after_exon& splice);
    NCBI_DEPRECATED bool IsSetSplice_3_prime() const;
    NCBI_DEPRECATED bool CanGetSplice_3_prime() const;
    NCBI_DEPRECATED void ResetSplice_3_prime();

    /// @}

private:
    // Prohibit copy constructor and assignment operator
    CSpliced_exon(const CSpliced_exon& value);
    CSpliced_exon& operator=(const CSpliced_exon& value);

};

/////////////////// CSpliced_exon inline methods

// constructor
inline
CSpliced_exon::CSpliced_exon(void)
{
}


/////////////////// end of CSpliced_exon inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_SEQALIGN_SPLICED_EXON_HPP
/* Original file checksum: lines: 86, chars: 2479, CRC32: e6b79c71 */
