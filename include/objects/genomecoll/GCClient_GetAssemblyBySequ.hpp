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

/// @file GCClient_GetAssemblyBySequ.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'gencoll_client.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: GCClient_GetAssemblyBySequ_.hpp


#ifndef OBJECTS_GENOMECOLL_GCCLIENT_GETASSEMBLYBYSEQU_HPP
#define OBJECTS_GENOMECOLL_GCCLIENT_GETASSEMBLYBYSEQU_HPP


// generated includes
#include <objects/genomecoll/GCClient_GetAssemblyBySequ_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class CGCClient_GetAssemblyBySequenceRequest : public CGCClient_GetAssemblyBySequenceRequest_Base
{
    typedef CGCClient_GetAssemblyBySequenceRequest_Base Tparent;
public:
    // constructor
    CGCClient_GetAssemblyBySequenceRequest(void);
    // destructor
    ~CGCClient_GetAssemblyBySequenceRequest(void);

    static string GetFilterDisplayName(int filter);

private:
    // Prohibit copy constructor and assignment operator
    CGCClient_GetAssemblyBySequenceRequest(const CGCClient_GetAssemblyBySequenceRequest& value);
    CGCClient_GetAssemblyBySequenceRequest& operator=(const CGCClient_GetAssemblyBySequenceRequest& value);

};

/////////////////// CGCClient_GetAssemblyBySequenceRequest inline methods

// constructor
inline
CGCClient_GetAssemblyBySequenceRequest::CGCClient_GetAssemblyBySequenceRequest(void)
{
}


/////////////////// end of CGCClient_GetAssemblyBySequenceRequest inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_GENOMECOLL_GCCLIENT_GETASSEMBLYBYSEQU_HPP
/* Original file checksum: lines: 86, chars: 2881, CRC32: 175ce2d7 */
