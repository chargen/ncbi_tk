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

/// @VersionResponse_.hpp
/// Data storage class.
///
/// This file was generated by application DATATOOL
/// using the following specifications:
/// 'samplesoap.dtd'.
///
/// ATTENTION:
///   Don't edit or commit this file into CVS as this file will
///   be overridden (by DATATOOL) without warning!

#ifndef VERSIONRESPONSE_BASE_HPP
#define VERSIONRESPONSE_BASE_HPP

// standard includes
#include <serial/serialbase.hpp>

// forward declarations
class CVersionStruct;


// generated classes

/////////////////////////////////////////////////////////////////////////////
class CVersionResponse_Base : public ncbi::CSerialObject
{
    typedef ncbi::CSerialObject Tparent;
public:
    // constructor
    CVersionResponse_Base(void);
    // destructor
    virtual ~CVersionResponse_Base(void);

    // type info
    DECLARE_INTERNAL_TYPE_INFO();

    // types
    typedef CVersionStruct TVersionStruct;

    // getters
    // setters

    /// mandatory
    /// typedef CVersionStruct TVersionStruct
    bool IsSetVersionStruct(void) const;
    bool CanGetVersionStruct(void) const;
    void ResetVersionStruct(void);
    const TVersionStruct& GetVersionStruct(void) const;
    void SetVersionStruct(TVersionStruct& value);
    TVersionStruct& SetVersionStruct(void);

    /// Reset the whole object
    virtual void Reset(void);


private:
    // Prohibit copy constructor and assignment operator
    CVersionResponse_Base(const CVersionResponse_Base&);
    CVersionResponse_Base& operator=(const CVersionResponse_Base&);

    // data
    Uint4 m_set_State[1];
    ncbi::CRef< TVersionStruct > m_VersionStruct;
};






///////////////////////////////////////////////////////////
///////////////////// inline methods //////////////////////
///////////////////////////////////////////////////////////
inline
bool CVersionResponse_Base::IsSetVersionStruct(void) const
{
    return m_VersionStruct;
}

inline
bool CVersionResponse_Base::CanGetVersionStruct(void) const
{
    return IsSetVersionStruct();
}

inline
const CVersionStruct& CVersionResponse_Base::GetVersionStruct(void) const
{
    if (!CanGetVersionStruct()) {
        ThrowUnassigned(0);
    }
    return (*m_VersionStruct);
}

inline
CVersionStruct& CVersionResponse_Base::SetVersionStruct(void)
{
    return (*m_VersionStruct);
}

///////////////////////////////////////////////////////////
////////////////// end of inline methods //////////////////
///////////////////////////////////////////////////////////






#endif // VERSIONRESPONSE_BASE_HPP
