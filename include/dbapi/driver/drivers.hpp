#ifndef DBAPI_DRIVER___DRIVERS__HPP
#define DBAPI_DRIVER___DRIVERS__HPP

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
 * Author:  Vladimir Soussov
 *   
 * File Description:  Drivers' registration
 *
 */


BEGIN_NCBI_SCOPE

class I_DriverMgr;

void DBAPI_RegisterDriver_CTLIB (I_DriverMgr& mgr);
void DBAPI_RegisterDriver_DBLIB (I_DriverMgr& mgr);
void DBAPI_RegisterDriver_FTDS  (I_DriverMgr& mgr);
void DBAPI_RegisterDriver_ODBC  (I_DriverMgr& mgr);


END_NCBI_SCOPE



/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2002/07/02 20:52:31  soussov
 * adds RegisterDriver for ODBC
 *
 * Revision 1.1  2002/01/17 22:05:56  soussov
 * adds driver manager
 *
 * ===========================================================================
 */

#endif  /* DBAPI_DRIVER___DRIVERS__HPP */
