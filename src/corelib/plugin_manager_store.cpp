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
 * Author: Anatoliy Kuznetsov
 *
 * File Description:  Object Store implementations
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/plugin_manager_store.hpp>

BEGIN_NCBI_SCOPE


CPluginManagerStore::CPluginManagerStore()
{
}


CPluginManagerStore::~CPluginManagerStore()
{
}


END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.3  2004/12/27 16:10:41  vasilche
* Force linking in CSingletonObjectStore::s_obj_store on GCC.
*
* Revision 1.2  2004/12/21 21:40:18  grichenk
* Moved obj_store and plugin_manager_store to corelib
*
* Revision 1.1  2004/08/02 13:44:10  kuznets
* Initial revision
*
*
* ===========================================================================
*/
