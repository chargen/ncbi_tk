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

/// @file Db_Clipboard.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'twebenv.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Db_Clipboard_.hpp


#ifndef DB_CLIPBOARD_HPP
#define DB_CLIPBOARD_HPP


// generated includes
#include "Db_Clipboard_.hpp"

// generated classes

/////////////////////////////////////////////////////////////////////////////
class CDb_Clipboard : public CDb_Clipboard_Base
{
    typedef CDb_Clipboard_Base Tparent;
public:
    // constructor
    CDb_Clipboard(void);
    // destructor
    ~CDb_Clipboard(void);

private:
    // Prohibit copy constructor and assignment operator
    CDb_Clipboard(const CDb_Clipboard& value);
    CDb_Clipboard& operator=(const CDb_Clipboard& value);

};

/////////////////// CDb_Clipboard inline methods

// constructor
inline
CDb_Clipboard::CDb_Clipboard(void)
{
}


/////////////////// end of CDb_Clipboard inline methods



/*
* ===========================================================================
*
* $Log$
* Revision 1.3  2005/04/26 14:18:50  vasilche
* Allow allocation of objects in CObjectMemoryPool.
*
*
* ===========================================================================
*/

#endif // DB_CLIPBOARD_HPP
/* Original file checksum: lines: 86, chars: 2432, CRC32: 4aea2f3b */
