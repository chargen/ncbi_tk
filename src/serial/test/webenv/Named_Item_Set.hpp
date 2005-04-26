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

/// @file Named_Item_Set.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'twebenv.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Named_Item_Set_.hpp


#ifndef NAMED_ITEM_SET_HPP
#define NAMED_ITEM_SET_HPP


// generated includes
#include "Named_Item_Set_.hpp"

// generated classes

/////////////////////////////////////////////////////////////////////////////
class CNamed_Item_Set : public CNamed_Item_Set_Base
{
    typedef CNamed_Item_Set_Base Tparent;
public:
    // constructor
    CNamed_Item_Set(void);
    // destructor
    ~CNamed_Item_Set(void);

private:
    // Prohibit copy constructor and assignment operator
    CNamed_Item_Set(const CNamed_Item_Set& value);
    CNamed_Item_Set& operator=(const CNamed_Item_Set& value);

};

/////////////////// CNamed_Item_Set inline methods

// constructor
inline
CNamed_Item_Set::CNamed_Item_Set(void)
{
}


/////////////////// end of CNamed_Item_Set inline methods



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

#endif // NAMED_ITEM_SET_HPP
/* Original file checksum: lines: 86, chars: 2470, CRC32: eac49b84 */
