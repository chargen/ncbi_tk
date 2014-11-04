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
 * Author:  Sergey Satskiy
 *
 */

#include <ncbi_pch.hpp>

#include <corelib/ncbistd.hpp>
#include <connect/services/json_over_uttp.hpp>
#include "nst_database.hpp"
#include "nst_exception.hpp"
#include "nst_application.hpp"
#include "nst_server.hpp"


BEGIN_NCBI_SCOPE


CNSTDatabase::CNSTDatabase(CNetStorageServer *  server)
    : m_Server(server), m_Db(NULL), m_Connected(false),
      m_RestoreConnectionThread(NULL)
{
    x_CreateDatabase();
    try {
        m_RestoreConnectionThread.Reset(new CNSTDBConnectionThread(
                                                m_Connected, m_Db));
        m_RestoreConnectionThread->Run();
    } catch (...) {
        delete m_Db;
        throw;
    }
}


void CNSTDatabase::InitialConnect(void)
{
    if (!m_Connected) {
        try {
            m_Db->Connect();
            m_Connected = true;
        } catch (...) {
            m_RestoreConnectionThread->Wakeup();
            throw;
        }
    }
}


CNSTDatabase::~CNSTDatabase(void)
{
    m_RestoreConnectionThread->Stop();
    m_RestoreConnectionThread->Join();
    m_RestoreConnectionThread.Reset(0);

    if (m_Connected)
        m_Db->Close();
    delete m_Db;
}


int
CNSTDatabase::ExecSP_GetNextObjectID(Int8 &  object_id)
{
    x_PreCheckConnection();

    int     status;
    try {
        CQuery      query = x_NewQuery();

        object_id = 0;
        query.SetParameter("@next_id", 0, eSDB_Int8, eSP_InOut);
        query.ExecuteSP("GetNextObjectID");
        query.VerifyDone();
        status = x_CheckStatus(query, "GetNextObjectID");

        if (status == 0)
            object_id = query.GetParameter("@next_id").AsInt8();
        else
            object_id = -1;
        return status;
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while getting next object ID");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_CreateClient(
            const string &  client, Int8 &  client_id)
{
    x_PreCheckConnection();

    int     status;
    try {
        CQuery      query = x_NewQuery();

        client_id = -1;
        query.SetParameter("@client_name", client);
        query.SetParameter("@client_id", client_id, eSDB_Int8, eSP_InOut);

        query.ExecuteSP("CreateClient");
        query.VerifyDone();
        status = x_CheckStatus(query, "CreateClient");

        if (status == 0)
            client_id = query.GetParameter("@client_id").AsInt8();
        else
            client_id = -1;
        return status;
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while creating a client");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_CreateObject(
            Int8  object_id, const string &  object_key,
            const string &  object_loc, Int8  size,
            const string &  client_name)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_id", object_id);
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@object_size", size);
        query.SetParameter("@client_name", client_name);

        query.ExecuteSP("CreateObject");
        query.VerifyDone();
        return x_CheckStatus(query, "CreateObject");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while creating an object");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_CreateObjectWithClientID(
            Int8  object_id, const string &  object_key,
            const string &  object_loc, Int8  size,
            Int8  client_id)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_id", object_id);
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@object_size", size);
        query.SetParameter("@client_id", client_id);

        query.ExecuteSP("CreateObjectWithClientID");
        query.VerifyDone();
        return x_CheckStatus(query, "CreateObjectWithClientID");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while creating "
                                     "an object with client ID");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_UpdateObjectOnWrite(
            const string &  object_key,
            const string &  object_loc, Int8  size, Int8  client_id)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@object_size", size);
        query.SetParameter("@client_id", client_id);

        query.ExecuteSP("UpdateObjectOnWrite");
        query.VerifyDone();
        return x_CheckStatus(query, "UpdateObjectOnWrite");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while updating an "
                                     "object on write");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_UpdateObjectOnRead(
            const string &  object_key,
            const string &  object_loc, Int8  size, Int8  client_id)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@object_size", size);
        query.SetParameter("@client_id", client_id);

        query.ExecuteSP("UpdateObjectOnRead");
        query.VerifyDone();
        return x_CheckStatus(query, "UpdateObjectOnRead");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while updating an "
                                     "object on read");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_UpdateObjectOnRelocate(
            const string &  object_key,
            const string &  object_loc, Int8  client_id)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@client_id", client_id);

        query.ExecuteSP("UpdateObjectOnRelocate");
        query.VerifyDone();
        return x_CheckStatus(query, "UpdateObjectOnRelocate");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while updating an "
                                     "object on relocate");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_RemoveObject(const string &  object_key)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);

        query.ExecuteSP("RemoveObject");
        query.VerifyDone();
        return x_CheckStatus(query, "RemoveObject");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while removing an object");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_AddAttribute(const string &  object_key,
                                  const string &  object_loc,
                                  const string &  attr_name,
                                  const string &  attr_value,
                                  Int8  client_id)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@object_loc", object_loc);
        query.SetParameter("@attr_name", attr_name);
        query.SetParameter("@attr_value", attr_value);
        query.SetParameter("@client_id", client_id);

        query.ExecuteSP("AddAttribute");
        query.VerifyDone();
        return x_CheckStatus(query, "AddAttribute");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while adding an attribute");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_GetAttribute(const string &  object_key,
                                  const string &  attr_name,
                                  string &        value)
{
    x_PreCheckConnection();

    int     status;
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@attr_name", attr_name);
        query.SetParameter("@attr_value", "", eSDB_String, eSP_InOut);

        query.ExecuteSP("GetAttribute");
        query.VerifyDone();
        status = x_CheckStatus(query, "GetAttribute");

        if (status == 0)
            value = query.GetParameter("@attr_value").AsString();
        else
            value = "";
        return status;
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while getting an attribute");
        x_PostCheckConnection();
        throw;
    }
}


int
CNSTDatabase::ExecSP_DelAttribute(const string &  object_key,
                                  const string &  attr_name)
{
    x_PreCheckConnection();
    try {
        CQuery      query = x_NewQuery();
        query.SetParameter("@object_key", object_key);
        query.SetParameter("@attr_name", attr_name);

        query.ExecuteSP("DelAttribute");
        query.VerifyDone();
        return x_CheckStatus(query, "DelAttribute");
    } catch (...) {
        m_Server->RegisterAlert(eDB, "DB error while deleting an attribute");
        x_PostCheckConnection();
        throw;
    }
}


void CNSTDatabase::x_PreCheckConnection(void)
{
    if (!m_Connected)
        NCBI_THROW(CNetStorageServerException, eDatabaseError,
                   "There is no connection to metadata information database");

    // It is possible that a connection has been lost and restored while there
    // were no activities
    if (!m_Db->IsConnected(CDatabase::eFastCheck)) {
        try {
            m_Db->Close();
            m_Db->Connect();
        } catch (...) {
            // To avoid interfering the connection restoring thread nothing is
            // done here. Basically the fact that we are here means there is no
            // connection anymore. The exception is suppressed however a stored
            // procedure execution exception will be generated a few moments
            // later and a normal procedure of the connection restoration will
            // be activated.
        }
    }
}


void CNSTDatabase::x_PostCheckConnection(void)
{
    if (!m_Db->IsConnected(CDatabase::eFastCheck)) {
        m_Connected = false;
        ERR_POST(Critical << "Database connection has been lost");
        m_RestoreConnectionThread->Wakeup();
    }
}


int  CNSTDatabase::x_CheckStatus(CQuery &  query,
                                 const string &  procedure)
{
    int     status = query.GetStatus();
    if (status > 0)
        NCBI_THROW(CNetStorageServerException, eDatabaseError,
                   "Error executing " + procedure + " stored "
                   "procedure (return code " + NStr::NumericToString(status) +
                   "). See MS SQL log for details.");
    return status;
}


CQuery CNSTDatabase::x_NewQuery(void)
{
    x_PreCheckConnection();
    return m_Db->NewQuery();
}


void  CNSTDatabase::x_ReadDbAccessInfo(void)
{
    CNetStorageDApp *       app = dynamic_cast<CNetStorageDApp*>
                                        (CNcbiApplication::Instance());
    const CNcbiRegistry &   reg  = app->GetConfig();

    m_DbAccessInfo.m_Service = reg.GetString("database", "service", "");
    m_DbAccessInfo.m_UserName = reg.GetString("database", "user_name", "");
    m_DbAccessInfo.m_Password = reg.GetString("database", "password", "");
    m_DbAccessInfo.m_Database = reg.GetString("database", "database", "");
}


void  CNSTDatabase::x_CreateDatabase(void)
{
    x_ReadDbAccessInfo();

    string  uri = "dbapi://" + m_DbAccessInfo.m_UserName +
                  ":" + m_DbAccessInfo.m_Password +
                  "@" + m_DbAccessInfo.m_Service +
                  "/" + m_DbAccessInfo.m_Database;

    CSDB_ConnectionParam    db_params(uri);
    m_Db = new CDatabase(db_params);
}

END_NCBI_SCOPE

