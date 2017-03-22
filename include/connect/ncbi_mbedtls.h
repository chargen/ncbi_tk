#ifndef CONNECT___NCBI_MBEDTLS__H
#define CONNECT___NCBI_MBEDTLS__H

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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   MBEDTLS support for SSL in connection library
 *
 */

#include <connect/ncbi_socket.h>

#ifdef HAVE_LIBMBEDTLS  /* external */
#  define NCBI_MBEDTLS_HEADER(x)  <../include/x>
#elif defined(NCBI_CXX_TOOLKIT)
#  define NCBI_MBEDTLS_HEADER(x)  <connect/x>
#endif


/** @addtogroup Sockets
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif


extern NCBI_XCONNECT_EXPORT
SOCKSSL NcbiSetupMbedTls(void);


extern NCBI_XCONNECT_EXPORT
NCBI_CRED NcbiCredMbedTls(void* xcred);


#ifdef __cplusplus
} /* extern "C" */
#endif


/* @} */

#endif /* CONNECT___NCBI_MBEDTLS_H */
