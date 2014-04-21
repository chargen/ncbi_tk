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

#ifndef OBJECTS_SEQFEAT_PROT_REF_HPP
#define OBJECTS_SEQFEAT_PROT_REF_HPP


// generated includes
#include <objects/seqfeat/Prot_ref_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class NCBI_SEQFEAT_EXPORT CProt_ref : public CProt_ref_Base
{
    typedef CProt_ref_Base Tparent;
public:
    // constructor
    CProt_ref(void);
    // destructor
    ~CProt_ref(void);
    
    // Appends a label to "label" based on content
    void GetLabel(string* label) const;

    /// Enzyme Commission number status
    enum EECNumberStatus {
        eEC_specific,  ///< Specifically identifies a valid classification.
        eEC_ambiguous, ///< Valid wildcard for a broader family.
        eEC_replaced,  ///< Obsolete synonym for some other EC number.
        eEC_deleted,   ///< Withdrawn, with no (single?) replacement.
        eEC_unknown    ///< Unrecognized; possibly malformed.
    };

    /// Enzyme Commission file status
    enum EECNumberFileStatus {
        eECFile_not_attempted, ///< No attempt has been made to read the file
        eECFile_not_found,     ///< File was not found in expected directory
        eECFile_not_read,      ///< File was found but could not be read
        eECFile_read           ///< File was read successfully (and is being
                               ///  instead of the compiled fallback data
    };

    /// Determine an EC number's validity and specificity.
    static EECNumberStatus GetECNumberStatus(const string& ecno);

    /// Return a replaced EC number's replacement.
    static const string& GetECNumberReplacement(const string& old_ecno);

    /// Verify correct form of EC number.
    static bool IsValidECNumberFormat (const string&  ecno);

    static EECNumberFileStatus GetECNumAmbiguousStatus();
    static EECNumberFileStatus GetECNumDeletedStatus();
    static EECNumberFileStatus GetECNumReplacedStatus();
    static EECNumberFileStatus GetECNumSpecificStatus();


private:
    // Prohibit copy constructor and assignment operator
    CProt_ref(const CProt_ref& value);
    CProt_ref& operator=(const CProt_ref& value);

};



/////////////////// CProt_ref inline methods

// constructor
inline
CProt_ref::CProt_ref(void)
{
}


/////////////////// end of CProt_ref inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_SEQFEAT_PROT_REF_HPP
/* Original file checksum: lines: 90, chars: 2388, CRC32: 683107f2 */
