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

/// @file Rna_feat_type.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'macro.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Rna_feat_type_.hpp


#ifndef OBJECTS_MACRO_RNA_FEAT_TYPE_HPP
#define OBJECTS_MACRO_RNA_FEAT_TYPE_HPP


// generated includes
#include <objects/macro/Rna_feat_type_.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/RNA_gen.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class CRna_feat_type : public CRna_feat_type_Base
{
    typedef CRna_feat_type_Base Tparent;
public:
    // constructor
    CRna_feat_type(void);
    // destructor
    ~CRna_feat_type(void);

    bool Match (const CSeq_feat& feat) const;
    CRNA_ref :: EType GetRnaRefType(E_Choice choice) const;

private:
    // Prohibit copy constructor and assignment operator
    CRna_feat_type(const CRna_feat_type& value);
    CRna_feat_type& operator=(const CRna_feat_type& value);

};

/////////////////// CRna_feat_type inline methods

// constructor
inline
CRna_feat_type::CRna_feat_type(void)
{
}


/////////////////// end of CRna_feat_type inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_MACRO_RNA_FEAT_TYPE_HPP
/* Original file checksum: lines: 86, chars: 2462, CRC32: d4a96a8e */
