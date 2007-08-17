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
 * File Description:  ODBC connection
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <dbapi/driver/odbc/interfaces.hpp>
#include <dbapi/driver/types.hpp>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ODBCSS_H
#include <odbcss.h>
#endif

#include "odbc_utils.hpp"

BEGIN_NCBI_SCOPE

static bool ODBC_xSendDataPrepare(CStatementBase& stmt,
                                  CDB_ITDescriptor& descr_in,
                                  SQLLEN size,
                                  bool is_text,
                                  bool logit,
                                  SQLPOINTER id,
                                  SQLLEN* ph);

static bool ODBC_xSendDataGetId(CStatementBase& stmt,
                                SQLPOINTER* id);

CODBC_Connection::CODBC_Connection(CODBCContext& cntx,
                                   const I_DriverContext::SConnAttr& conn_attr) :
    impl::CConnection(cntx, false, conn_attr.reusable, conn_attr.pool_name),
    m_Link(NULL),
    m_Reporter(0, SQL_HANDLE_DBC, NULL, &cntx.GetReporter()),
    m_query_timeout(cntx.GetTimeout())
{
    SQLRETURN rc;

    rc = SQLAllocHandle(SQL_HANDLE_DBC,
                        cntx.GetODBCContext(),
                        const_cast<SQLHDBC*>(&m_Link));

    if((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
        DATABASE_DRIVER_ERROR( "Cannot allocate a connection handle.", 100011 );
    }

    // This might look strange, but in current design all errors related to
    // opening of a connection to a database are reported by a DriverContext.
    // Have fun.
    cntx.SetupErrorReporter(conn_attr);

    x_SetupErrorReporter(conn_attr);

    x_SetConnAttributesBefore(cntx, conn_attr);

    x_Connect(cntx, conn_attr);

    x_SetConnAttributesAfter(conn_attr);
}


void
CODBC_Connection::x_SetupErrorReporter(const I_DriverContext::SConnAttr& conn_attr)
{
    string extra_msg = " SERVER: " + conn_attr.srv_name + "; USER: " + conn_attr.user_name;

    _ASSERT(m_Link);

    m_Reporter.SetHandle(m_Link);
    m_Reporter.SetHandlerStack(GetMsgHandlers());
    m_Reporter.SetExtraMsg( extra_msg );
}

void CODBC_Connection::x_Connect(
    CODBCContext& cntx,
    const I_DriverContext::SConnAttr& conn_attr) const
{
    SQLRETURN rc;

    if(!cntx.GetUseDSN()) {
        string connect_str;
        const string conn_str_suffix(";SERVER=" +
                                     conn_attr.srv_name +
                                     ";UID=" +
                                     conn_attr.user_name +
                                     ";PWD=" +
                                     conn_attr.passwd);

#ifndef FTDS_IN_USE
        CNcbiApplication* app = CNcbiApplication::Instance();

        // Connection strings for SQL Native Client 2005.
        // string connect_str("DRIVER={SQL Native Client};MultipleActiveResultSets=true;SERVER=");
        // string connect_str("DRIVER={SQL Native Client};SERVER=");

        if (app) {
            const string driver_name = x_GetDriverName(app->GetConfig());

            connect_str = "DRIVER={" + driver_name + "}";
        } else {
            connect_str = "DRIVER={SQL Server}";
        }

        connect_str += conn_str_suffix;
#else
        connect_str = "DRIVER={FreeTDS}";
        connect_str += conn_str_suffix;

        connect_str += ";" + x_MakeFreeTDSVersion(cntx.GetTDSVersion());

        if (!GetCDriverContext().GetClientCharset().empty()) {
            connect_str += ";client_charset=" + GetCDriverContext().GetClientCharset();
        }
#endif

        rc = SQLDriverConnect(m_Link,
                              0,
                              CODBCString(connect_str, GetClientEncoding()),
                              SQL_NTS,
                              0,
                              0,
                              0,
                              SQL_DRIVER_NOPROMPT);
    }
    else {
        rc = SQLConnect(m_Link,
                        CODBCString(conn_attr.srv_name, GetClientEncoding()),
                        SQL_NTS,
                        CODBCString(conn_attr.user_name, GetClientEncoding()),
                        SQL_NTS,
                        CODBCString(conn_attr.passwd, GetClientEncoding()),
                        SQL_NTS);
    }

    if (!cntx.CheckSIE(rc, m_Link)) {
        string err;

        err += "Cannot connect to the server '" + conn_attr.srv_name;
        err += "' as user '" + conn_attr.user_name + "'" + m_Reporter.GetExtraMsg();
        DATABASE_DRIVER_ERROR( err, 100011 );
    }
}


void
CODBC_Connection::x_SetConnAttributesBefore(
    const CODBCContext& cntx,
    const I_DriverContext::SConnAttr& conn_attr)
{
    if(GetCDriverContext().GetTimeout()) {
        SQLSetConnectAttr(m_Link,
                          SQL_ATTR_CONNECTION_TIMEOUT,
                          (SQLPOINTER)SQLULEN(GetCDriverContext().GetLoginTimeout()),
                          0);
    }

    if(GetCDriverContext().GetLoginTimeout()) {
        SQLSetConnectAttr(m_Link,
                          SQL_ATTR_LOGIN_TIMEOUT,
                          (SQLPOINTER)SQLULEN(GetCDriverContext().GetLoginTimeout()),
                          0);
    }

    if(cntx.GetPacketSize()) {
        SQLSetConnectAttr(m_Link,
                          SQL_ATTR_PACKET_SIZE,
                          (SQLPOINTER)SQLULEN(cntx.GetPacketSize()),
                          0);
    }

#ifdef SQL_COPT_SS_BCP
//     if((conn_attr.mode & I_DriverContext::fBcpIn) != 0) {
        // Always enable BCP ...
        SQLSetConnectAttr(m_Link,
                          SQL_COPT_SS_BCP,
                          (SQLPOINTER) SQL_BCP_ON,
                          SQL_IS_INTEGER);
//     }
#endif
}


void
CODBC_Connection::x_SetConnAttributesAfter(const I_DriverContext::SConnAttr& conn_attr)
{
    SetServerName(conn_attr.srv_name);
    SetUserName(conn_attr.user_name);
    SetPassword(conn_attr.passwd);
    SetBCPable((conn_attr.mode & I_DriverContext::fBcpIn) != 0);
    SetSecureLogin((conn_attr.mode & I_DriverContext::fPasswordEncrypted) != 0);
}

string
CODBC_Connection::x_GetDriverName(const IRegistry& registry)
{
    enum EState {eStInitial, eStSingleQuote};
    vector<string> driver_names;
    const string odbc_driver_name =
        registry.GetString("ODBC", "DRIVER_NAME", "'SQL Server'");

    NStr::Tokenize(odbc_driver_name, " ", driver_names);
    EState state = eStInitial;
    string driver_name;

    ITERATE(vector<string>, it, driver_names) {
        bool complete_deriver_name = false;
        const string cur_str(*it);

        // Check for quotes ...
        if (state == eStInitial) {
            if (cur_str[0] == '\'') {
                if (cur_str[cur_str.size() - 1] == '\'') {
                    // Skip quote ...
                    driver_name = it->substr(1, cur_str.size() - 2);
                    complete_deriver_name = true;
                } else {
                    // Skip quote ...
                    driver_name = it->substr(1);
                    state = eStSingleQuote;
                }
            } else {
                driver_name = cur_str;
                complete_deriver_name = true;
            }
        } else if (state == eStSingleQuote) {
            if (cur_str[cur_str.size() - 1] == '\'') {
                // Final quote ...
                driver_name += " " + cur_str.substr(0, cur_str.size() - 1);
                state = eStInitial;
                complete_deriver_name = true;
            } else {
                driver_name += " " + cur_str;
            }
        }

        if (complete_deriver_name) {
            return driver_name;
        }
    }

    return driver_name;
}


string CODBC_Connection::x_MakeFreeTDSVersion(int version)
{
    string str_version = "TDS_Version=";

    switch ( version )
    {
    case 42:
        str_version += "4.2;Port=2158";
        break;
    case 46:
        str_version += "4.6";
        break;
    case 50:
        str_version += "5.0;Port=2158";
        break;
    case 70:
        str_version += "7.0";
        break;
    case 80:
        str_version += "8.0";
        break;
    default:
        // DATABASE_DRIVER_ERROR( "Invalid TDS version with the FreeTDS driver.", 100000 );
        str_version += "8.0";
        break;
    }

    return str_version;
}


bool CODBC_Connection::IsAlive(void)
{
    // FreeTDS odbc driver does not support SQL_ATTR_CONNECTION_DEAD attribute.

#if !defined(FTDS_IN_USE)
    if (m_Link) {
        SQLINTEGER status;
        SQLRETURN r= SQLGetConnectAttr(m_Link, SQL_ATTR_CONNECTION_DEAD, &status, SQL_IS_INTEGER, 0);

        return ((r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) && (status == SQL_CD_FALSE));
    }
#endif

    return (m_Link != NULL);
}


CODBC_LangCmd* CODBC_Connection::xLangCmd(const string& lang_query,
                                          unsigned int  nof_params)
{
    string extra_msg = "SQL Command: \"" + lang_query + "\"";
    m_Reporter.SetExtraMsg( extra_msg );

    CODBC_LangCmd* lcmd = new CODBC_LangCmd(*this, lang_query, nof_params);
    return lcmd;
}

CDB_LangCmd* CODBC_Connection::LangCmd(const string& lang_query,
                                     unsigned int  nof_params)
{
    return Create_LangCmd(*(xLangCmd(lang_query, nof_params)));
}


CDB_RPCCmd* CODBC_Connection::RPC(const string& rpc_name,
                                unsigned int  nof_args)
{
    string extra_msg = "RPC Command: " + rpc_name;
    m_Reporter.SetExtraMsg( extra_msg );

    CODBC_RPCCmd* rcmd = new CODBC_RPCCmd(*this, rpc_name, nof_args);
    return Create_RPCCmd(*rcmd);
}


CDB_BCPInCmd* CODBC_Connection::BCPIn(const string& table_name,
                                    unsigned int  nof_columns)
{
#ifdef FTDS_IN_USE
    return NULL; // not implemented yet
#else
    if ( !IsBCPable() ) {
        string err_message = "No bcp on this connection." + GetDbgInfo();
        DATABASE_DRIVER_ERROR( err_message, 410003 );
    }

    string extra_msg = "BCP Table: " + table_name;
    m_Reporter.SetExtraMsg( extra_msg );

    CODBC_BCPInCmd* bcmd = new CODBC_BCPInCmd(*this, m_Link, table_name, nof_columns);
    return Create_BCPInCmd(*bcmd);
#endif
}


CDB_CursorCmd* CODBC_Connection::Cursor(const string& cursor_name,
                                      const string& query,
                                      unsigned int  nof_params,
                                      unsigned int  batch_size)
{
    string extra_msg = "Cursor Name: \"" + cursor_name + "\"; SQL Command: \""+
        query + "\"";
    m_Reporter.SetExtraMsg( extra_msg );

#if 1 // defined(FTDS_IN_USE)
    CODBC_CursorCmdExpl* ccmd = new CODBC_CursorCmdExpl(*this,
                                                        cursor_name,
                                                        query,
                                                        nof_params);
#else
    CODBC_CursorCmd* ccmd = new CODBC_CursorCmd(*this,
                                                cursor_name,
                                                query,
                                                nof_params);
#endif

    return Create_CursorCmd(*ccmd);
}


CDB_SendDataCmd* CODBC_Connection::SendDataCmd(I_ITDescriptor& descr_in,
                                               size_t data_size, bool log_it)
{
    CODBC_SendDataCmd* sd_cmd =
        new CODBC_SendDataCmd(*this,
                              (CDB_ITDescriptor&)descr_in,
                              data_size,
                              log_it);
    return Create_SendDataCmd(*sd_cmd);
}


bool CODBC_Connection::SendData(I_ITDescriptor& desc, CDB_Image& img, bool log_it)
{
    CStatementBase stmt(*this);

    SQLPOINTER  p = (SQLPOINTER)2;
    SQLLEN      s = img.Size();
    SQLLEN      ph;

    if((!ODBC_xSendDataPrepare(stmt, (CDB_ITDescriptor&)desc, s, false, log_it, p, &ph)) ||
       (!ODBC_xSendDataGetId(stmt, &p ))) {
        string err_message = "Cannot prepare a command." + GetDbgInfo();
        DATABASE_DRIVER_ERROR( err_message, 410035 );
    }

    return x_SendData(CDB_ITDescriptor::eBinary, stmt, img);

}


bool CODBC_Connection::SendData(I_ITDescriptor& desc, CDB_Text& txt, bool log_it)
{
    CStatementBase stmt(*this);

    SQLPOINTER  p = (SQLPOINTER)2;
    SQLLEN      s = txt.Size();
    SQLLEN      ph;

    if((!ODBC_xSendDataPrepare(stmt, (CDB_ITDescriptor&)desc, s, true, log_it, p, &ph)) ||
       (!ODBC_xSendDataGetId(stmt, &p))) {
        string err_message = "Cannot prepare a command." + GetDbgInfo();
        DATABASE_DRIVER_ERROR( err_message, 410035 );
    }

    return x_SendData(CDB_ITDescriptor::eText, stmt, txt);
}


bool CODBC_Connection::Refresh()
{
    // close all commands first
    DeleteAllCommands();

    return IsAlive();
}


I_DriverContext::TConnectionMode CODBC_Connection::ConnectMode() const
{
    I_DriverContext::TConnectionMode mode = 0;
    if ( IsBCPable() ) {
        mode |= I_DriverContext::fBcpIn;
    }
    if ( HasSecureLogin() ) {
        mode |= I_DriverContext::fPasswordEncrypted;
    }
    return mode;
}


CODBC_Connection::~CODBC_Connection()
{
    try {
        Close();

        if(SQLFreeHandle(SQL_HANDLE_DBC, m_Link) == SQL_ERROR) {
            ReportErrors();
        }
    }
    NCBI_CATCH_ALL( NCBI_CURRENT_FUNCTION )
}


bool CODBC_Connection::Abort()
{
    SQLDisconnect(m_Link);
    return false;
}

bool CODBC_Connection::Close(void)
{
    if (Refresh()) {
        switch(SQLDisconnect(m_Link)) {
        case SQL_SUCCESS_WITH_INFO:
        case SQL_ERROR:
            ReportErrors();
        case SQL_SUCCESS:
            break;
        default:
            {
                string err_message = "SQLDisconnect failed (memory corruption suspected)." + GetDbgInfo();
                DATABASE_DRIVER_ERROR( err_message, 410009 );
            }
        }

        return true;
    }

    return false;
}

void CODBC_Connection::SetTimeout(size_t nof_secs)
{
    m_query_timeout = nof_secs;
}

static
bool
ODBC_xCheckSIE(int rc, CStatementBase& stmt)
{
    switch(rc) {
    case SQL_SUCCESS_WITH_INFO:
        stmt.ReportErrors();
    case SQL_SUCCESS: break;
    case SQL_ERROR:
    default:
        stmt.ReportErrors();
        return false;
    }

    return true;
}

static bool ODBC_xSendDataPrepare(CStatementBase& stmt,
                                  CDB_ITDescriptor& descr_in,
                                  SQLLEN size,
                                  bool is_text,
                                  bool logit,
                                  SQLPOINTER id,
                                  SQLLEN* ph)
{
    string q = "update ";
    q += descr_in.TableName();
    q += " set ";
    q += descr_in.ColumnName();
    q += "= ? where ";
    q += descr_in.SearchConditions();
    //q+= " ;\nset rowcount 0";

#if defined(SQL_TEXTPTR_LOGGING) && !defined(FTDS_IN_USE)
    if(!logit) {
        switch(SQLSetStmtAttr(stmt.GetHandle(), SQL_TEXTPTR_LOGGING, /*SQL_SOPT_SS_TEXTPTR_LOGGING,*/
            (SQLPOINTER)SQL_TL_OFF, SQL_IS_INTEGER)) {
        case SQL_SUCCESS_WITH_INFO:
        case SQL_ERROR:
            stmt.ReportErrors();
        default:
            break;
        }
    }
#endif


    CDB_ITDescriptor::ETDescriptorType descr_type = descr_in.GetColumnType();
    if (descr_type == CDB_ITDescriptor::eUnknown) {
        if (is_text) {
            descr_type = CDB_ITDescriptor::eText;
        } else {
            descr_type = CDB_ITDescriptor::eBinary;
        }
    }

    *ph = SQL_LEN_DATA_AT_EXEC(size);

    int c_type = 0;
    int sql_type = 0;

    if (descr_type == CDB_ITDescriptor::eText) {
#if defined(UNICODE)
        c_type = SQL_C_WCHAR;
        sql_type = SQL_WLONGVARCHAR;
#else
        c_type = SQL_C_CHAR;
        sql_type = SQL_LONGVARCHAR;
#endif
    } else {
        c_type = SQL_C_BINARY;
        sql_type = SQL_LONGVARBINARY;
    }

    // Do not use SQLDescribeParam. It is not implemented with the odbc driver
    // from FreeTDS.

    if (!ODBC_xCheckSIE(SQLBindParameter(
            stmt.GetHandle(),
            1,
            SQL_PARAM_INPUT,
            c_type,
            sql_type,
            size,
            0,
            id,
            0,
            ph),
        stmt)) {
        return false;
    }

    if (!ODBC_xCheckSIE(SQLPrepare(stmt.GetHandle(),
                                   CODBCString(q, stmt.GetClientEncoding()),
                                   SQL_NTS),
                        stmt)) {
        return false;
    }

    switch(SQLExecute(stmt.GetHandle())) {
    case SQL_NEED_DATA:
        return true;
    case SQL_SUCCESS_WITH_INFO:
    case SQL_ERROR:
        stmt.ReportErrors();
    default:
        return false;
    }
}

static bool ODBC_xSendDataGetId(CStatementBase& stmt,
                                SQLPOINTER* id)
{
    switch(SQLParamData(stmt.GetHandle(), id)) {
    case SQL_NEED_DATA:
        return true;
    case SQL_SUCCESS_WITH_INFO:
    case SQL_ERROR:
        stmt.ReportErrors();
    default:
        return false;
    }
}

bool CODBC_Connection::x_SendData(CDB_ITDescriptor::ETDescriptorType descr_type,
                                  CStatementBase& stmt,
                                  CDB_Stream& stream)
{
    char buff[1800];

    int rc;

    size_t len = 0;
    size_t invalid_len = 0;

    while(( len = stream.Read(buff + invalid_len, sizeof(buff) - invalid_len - 1)) != 0 ) {
        if (stmt.GetClientEncoding() == eEncoding_UTF8 &&
            descr_type == CDB_ITDescriptor::eText) {

            size_t valid_len = CStringUTF8::GetValidBytesCount(buff, len);
            invalid_len = len - valid_len;

            // Encoding is always eEncoding_UTF8 in here.
            CODBCString odbc_str(buff, valid_len, eEncoding_UTF8);
            // Force odbc_str to make conversion to odbc::TChar*.
            odbc::TChar* tchar_str = odbc_str;

            rc = SQLPutData(stmt.GetHandle(),
                            static_cast<SQLPOINTER>(tchar_str),
                            static_cast<SQLINTEGER>(odbc_str.GetSymbolNum() *
                                                    sizeof(odbc::TChar)) // Number of bytes ...
                            );

            if (valid_len < len) {
                memmove(buff, buff + valid_len, invalid_len);
            }
        } else {
            rc = SQLPutData(stmt.GetHandle(),
                            static_cast<SQLPOINTER>(buff),
                            static_cast<SQLINTEGER>(len) // Number of bytes ...
                            );
        }

        switch( rc ) {
        case SQL_SUCCESS_WITH_INFO:
            stmt.ReportErrors();
        case SQL_NEED_DATA:
            continue;
        case SQL_NO_DATA:
            return true;
        case SQL_SUCCESS:
            break;
        case SQL_ERROR:
            stmt.ReportErrors();
        default:
            return false;
        }
    }

    if (invalid_len > 0) {
        DATABASE_DRIVER_ERROR( "Invalid encoding of a text string." + GetDbgInfo(), 410055 );
    }

    switch(SQLParamData(stmt.GetHandle(), (SQLPOINTER*)&len)) {
    case SQL_SUCCESS_WITH_INFO: stmt.ReportErrors();
    case SQL_SUCCESS:           break;
    case SQL_NO_DATA:           return true;
    case SQL_NEED_DATA:
        {
            string err_message = "Not all the data were sent." + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, 410044 );
        }
    case SQL_ERROR:             stmt.ReportErrors();
    default:
        {
            string err_message = "SQLParamData failed." + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, 410045 );
        }
    }

    for(;;) {
        switch(SQLMoreResults( stmt.GetHandle() )) {
        case SQL_SUCCESS_WITH_INFO: stmt.ReportErrors();
        case SQL_SUCCESS:           continue;
        case SQL_NO_DATA:           break;
        case SQL_ERROR:
            {
                stmt.ReportErrors();
                string err_message = "SQLMoreResults failed." + GetDbgInfo();
                DATABASE_DRIVER_ERROR( err_message, 410014 );
            }
        default:
            {
                string err_message = "SQLMoreResults failed (memory corruption suspected)." + GetDbgInfo();
                DATABASE_DRIVER_ERROR( err_message, 410015 );
            }
        }
        break;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
CStatementBase::CStatementBase(CODBC_Connection& conn) :
    m_RowCount(-1),
    m_ConnectPtr(&conn),
    m_Reporter(&conn.GetMsgHandlers(), SQL_HANDLE_STMT, NULL, &conn.m_Reporter)
{
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn.m_Link, &m_Cmd);

    if(rc == SQL_ERROR) {
        conn.ReportErrors();
    }
    m_Reporter.SetHandle(m_Cmd);

    SQLUINTEGER query_timeout = static_cast<SQLUINTEGER>(conn.GetTimeout());
    switch(SQLSetStmtAttr(GetHandle(),
                       SQL_ATTR_QUERY_TIMEOUT,
                       (SQLPOINTER)static_cast<uintptr_t>(query_timeout),
                       0))
    {
    case SQL_SUCCESS_WITH_INFO:
    case SQL_ERROR:
    case SQL_INVALID_HANDLE:
        ReportErrors();
        break;
    default: // SQL_SUCCESS
        break;
    };
}

CStatementBase::~CStatementBase(void)
{
    try {
        SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, m_Cmd);
        if(rc != SQL_SUCCESS) {
            ReportErrors();
        }
    }
    NCBI_CATCH_ALL( NCBI_CURRENT_FUNCTION )
}

bool
CStatementBase::CheckRC(int rc) const
{
    switch (rc)
    {
    case SQL_SUCCESS:
        return true;
    case SQL_SUCCESS_WITH_INFO:
    case SQL_ERROR:
        ReportErrors();
        break;
    case SQL_INVALID_HANDLE:
        {
            string err_message = "Invalid handle." + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, 0 );
        }
        break;
    }

    return false;
}

int
CStatementBase::CheckSIE(int rc, const char* msg, unsigned int msg_num) const
{
    switch( rc ) {
    case SQL_SUCCESS_WITH_INFO:
        ReportErrors();

    case SQL_SUCCESS:
        break;

    case SQL_ERROR:
        ReportErrors();
        {
            string err_message = msg + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, msg_num );
        }
    default:
        {
            string err_message;

            err_message.append(msg);
            err_message.append(" (memory corruption suspected).");
            err_message.append(GetDbgInfo());

            DATABASE_DRIVER_ERROR( err_message, 420001 );
        }
    }

    return rc;
}

int
CStatementBase::CheckSIENd(int rc, const char* msg, unsigned int msg_num) const
{
    switch( rc ) {
    case SQL_SUCCESS_WITH_INFO:
        ReportErrors();

    case SQL_SUCCESS:
        break;

    case SQL_ERROR:
        ReportErrors();
        {
            string err_message = msg + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, msg_num );
        }
    default:
        {
            string err_message;

            err_message.append(msg);
            err_message.append(" (memory corruption suspected).");
            err_message.append(GetDbgInfo());

            DATABASE_DRIVER_ERROR( err_message, 420001 );
        }
    }

    return rc;
}


string
CStatementBase::Type2String(const CDB_Object& param) const
{
    string type_str;

    switch (param.GetType()) {
    case eDB_Int:
        type_str = "int";
        break;
    case eDB_SmallInt:
        type_str = "smallint";
        break;
    case eDB_TinyInt:
        type_str = "tinyint";
        break;
    case eDB_BigInt:
        type_str = "numeric";
        break;
    case eDB_Char:
    case eDB_VarChar:
        if (IsMultibyteClientEncoding()) {
            type_str = "nvarchar(255)";
        } else {
            type_str = "varchar(255)";
        }
        break;
    case eDB_LongChar:
        if (IsMultibyteClientEncoding()) {
            type_str = "nvarchar(4000)";
        } else {
            type_str = "varchar(8000)";
        }
        break;
    case eDB_Binary:
    case eDB_VarBinary:
        type_str = "varbinary(255)";
        break;
    case eDB_LongBinary:
        type_str = "varbinary(8000)";
        break;
    case eDB_Float:
        type_str = "real";
        break;
    case eDB_Double:
        type_str = "float";
        break;
    case eDB_SmallDateTime:
        type_str = "smalldatetime";
        break;
    case eDB_DateTime:
        type_str = "datetime";
        break;
    case eDB_Text:
        if (IsMultibyteClientEncoding()) {
            type_str = "ntext";
        } else {
            type_str = "text";
        }
    case eDB_Image:
        type_str = "image";
        break;
    default:
        break;
    }

    return type_str;
}

bool
CStatementBase::x_BindParam_ODBC(const CDB_Object& param,
                               CMemPot& bind_guard,
                               SQLLEN* indicator_base,
                               unsigned int pos) const
{
    SQLRETURN rc = 0;

    EDB_Type data_type = param.GetType();

    switch (data_type) {
    case eDB_Text:
    case eDB_Image:
    case eDB_Bit:
    case eDB_Numeric:
    case eDB_UnsupportedType:
        return false;
    default:
        break;
    }

    indicator_base[pos] = x_GetIndicator(param);

    rc = SQLBindParameter(GetHandle(),
                          pos + 1,
                          SQL_PARAM_INPUT,
                          x_GetCType(param),
                          x_GetSQLType(param),
                          x_GetMaxDataSize(param),
                          (data_type == eDB_DateTime ? 3 : 0),
                          x_GetData(param, bind_guard),
                          x_GetCurDataSize(param),
                          indicator_base + pos);

    CheckSIE(rc, "SQLBindParameter failed", 420066);

    return true;
}


SQLSMALLINT
CStatementBase::x_GetCType(const CDB_Object& param)
{
    SQLSMALLINT type = 0;

    switch (param.GetType()) {
    case eDB_Int:
        type = SQL_C_SLONG;
        break;
    case eDB_SmallInt:
        type = SQL_C_SSHORT;
        break;
    case eDB_TinyInt:
        type = SQL_C_UTINYINT;
        break;
    case eDB_BigInt:
        type = SQL_C_SBIGINT;
        break;
    case eDB_Char:
    case eDB_VarChar:
    case eDB_LongChar:
#ifdef UNICODE
        type = SQL_C_WCHAR;
#else
        type = SQL_C_CHAR;
#endif
        break;
    case eDB_Binary:
    case eDB_VarBinary:
    case eDB_LongBinary:
        type = SQL_C_BINARY;
        break;
    case eDB_Float:
        type = SQL_C_FLOAT;
        break;
    case eDB_Double:
        type = SQL_C_DOUBLE;
        break;
    case eDB_SmallDateTime:
    case eDB_DateTime:
        type = SQL_C_TYPE_TIMESTAMP;
        break;
    default:
        break;
    }

    return type;
}


SQLSMALLINT
CStatementBase::x_GetSQLType(const CDB_Object& param)
{
    SQLSMALLINT type = SQL_UNKNOWN_TYPE;

    switch (param.GetType()) {
    case eDB_Int:
        type = SQL_INTEGER;
        break;
    case eDB_SmallInt:
        type = SQL_SMALLINT;
        break;
    case eDB_TinyInt:
        type = SQL_TINYINT;
        break;
    case eDB_BigInt:
        type = SQL_NUMERIC;
        break;
    case eDB_Char:
    case eDB_VarChar:
#ifdef UNICODE
        type = SQL_WVARCHAR;
#else
        type = SQL_VARCHAR;
#endif
        break;
    case eDB_LongChar:
#ifdef UNICODE
        type = SQL_WLONGVARCHAR;
#else
        type = SQL_LONGVARCHAR;
#endif
        break;
    case eDB_Binary:
    case eDB_VarBinary:
    case eDB_LongBinary:
        type = SQL_VARBINARY;
        break;
    case eDB_Float:
        type = SQL_REAL;
        break;
    case eDB_Double:
        type = SQL_FLOAT;
        break;
    case eDB_SmallDateTime:
    case eDB_DateTime:
        type = SQL_TYPE_TIMESTAMP;
        break;
    default:
        break;
    }

    return type;
}


SQLULEN
CStatementBase::x_GetMaxDataSize(const CDB_Object& param)
{
    SQLULEN size = 0;

    switch (param.GetType()) {
    case eDB_Int:
        size = 4;
        break;
    case eDB_SmallInt:
        size = 2;
        break;
    case eDB_TinyInt:
        size = 1;
        break;
    case eDB_BigInt:
        size = 18;
        break;
    case eDB_Char:
    case eDB_VarChar:
        size = 255;
        break;
    case eDB_LongChar:
        size = 8000 / sizeof(odbc::TChar);
        break;
    case eDB_Binary:
    case eDB_VarBinary:
        size = 255;
        break;
    case eDB_LongBinary:
        size = 8000;
        break;
    case eDB_Float:
        size = 4;
        break;
    case eDB_Double:
        size = 8;
        break;
    case eDB_SmallDateTime:
        size = 16;
        break;
    case eDB_DateTime:
        size = 23;
        break;
    default:
        break;
    }

    return size;
}


SQLLEN
CStatementBase::x_GetCurDataSize(const CDB_Object& param)
{
    SQLLEN size = 0;

    switch (param.GetType()) {
    case eDB_Int:
    case eDB_SmallInt:
    case eDB_TinyInt:
    case eDB_BigInt:
    case eDB_Binary:
    case eDB_VarBinary:
    case eDB_Float:
    case eDB_Double:
        size = x_GetMaxDataSize(param);
        break;
    case eDB_Char:
    case eDB_VarChar:
        size = 256;
        break;
    case eDB_LongChar:
        size = dynamic_cast<const CDB_LongChar&>(param).DataSize() * sizeof(odbc::TChar);
        break;
    case eDB_LongBinary:
        size = dynamic_cast<const CDB_LongBinary&>(param).Size();
        break;
    case eDB_SmallDateTime:
    case eDB_DateTime:
        size = sizeof(SQL_TIMESTAMP_STRUCT);
        break;
    default:
        break;
    }

    return size;
}


SQLLEN
CStatementBase::x_GetIndicator(const CDB_Object& param)
{
    if (param.IsNULL()) {
        return SQL_NULL_DATA;
    }

    switch (param.GetType()) {
    case eDB_Char:
    case eDB_VarChar:
    case eDB_LongChar:
        return SQL_NTS;
    case eDB_Binary:
        return dynamic_cast<const CDB_Binary&>(param).Size();
    case eDB_VarBinary:
        return dynamic_cast<const CDB_VarBinary&>(param).Size();
    case eDB_LongBinary:
        return dynamic_cast<const CDB_LongBinary&>(param).DataSize();
    case eDB_SmallDateTime:
    case eDB_DateTime:
        return sizeof(SQL_TIMESTAMP_STRUCT);
        break;
    default:
        break;
    }

    return x_GetMaxDataSize(param);
}


SQLPOINTER
CStatementBase::x_GetData(const CDB_Object& param,
                          CMemPot& bind_guard) const
{
    SQLPOINTER data = NULL;

    switch (param.GetType()) {
    case eDB_Int:
        data = dynamic_cast<const CDB_Int&>(param).BindVal();
        break;
    case eDB_SmallInt:
        data = dynamic_cast<const CDB_SmallInt&>(param).BindVal();
        break;
    case eDB_TinyInt:
        data = dynamic_cast<const CDB_TinyInt&>(param).BindVal();
        break;
    case eDB_BigInt:
        data = dynamic_cast<const CDB_BigInt&>(param).BindVal();
        break;
    case eDB_Char:
    case eDB_VarChar:
    case eDB_LongChar:
#ifdef UNICODE
        data = const_cast<wchar_t *>(dynamic_cast<const CDB_String&>(param).AsUnicode(GetClientEncoding()));
#else
        data = const_cast<char *>(dynamic_cast<const CDB_String&>(param).Value());
#endif
        break;
    case eDB_Binary:
        data = const_cast<void *>(dynamic_cast<const CDB_Binary&>(param).Value());
        break;
    case eDB_VarBinary:
        data = const_cast<void *>(dynamic_cast<const CDB_VarBinary&>(param).Value());
        break;
    case eDB_LongBinary:
        data = const_cast<void *>(dynamic_cast<const CDB_LongBinary&>(param).Value());
        break;
    case eDB_Float:
        data = dynamic_cast<const CDB_Float&>(param).BindVal();
        break;
    case eDB_Double:
        data = dynamic_cast<const CDB_Double&>(param).BindVal();
        break;
    case eDB_SmallDateTime:
        if(!param.IsNULL()) {
            SQL_TIMESTAMP_STRUCT* ts = NULL;

            ts = (SQL_TIMESTAMP_STRUCT*)bind_guard.Alloc(sizeof(SQL_TIMESTAMP_STRUCT));
            const CTime& t = dynamic_cast<const CDB_SmallDateTime&>(param).Value();
            ts->year = t.Year();
            ts->month = t.Month();
            ts->day = t.Day();
            ts->hour = t.Hour();
            ts->minute = t.Minute();

            ts->second = 0;
            ts->fraction = 0;

            data = ts;
        }
        break;
    case eDB_DateTime:
        if(!param.IsNULL()) {
            SQL_TIMESTAMP_STRUCT* ts = NULL;

            ts = (SQL_TIMESTAMP_STRUCT*)bind_guard.Alloc(sizeof(SQL_TIMESTAMP_STRUCT));
            const CTime& t = dynamic_cast<const CDB_DateTime&>(param).Value();
            ts->year = t.Year();
            ts->month = t.Month();
            ts->day = t.Day();
            ts->hour = t.Hour();
            ts->minute = t.Minute();

            ts->second = t.Second();
            ts->fraction = t.NanoSecond()/1000000;
            ts->fraction *= 1000000; /* MSSQL has a bug - it cannot handle fraction of msecs */

            data = ts;
        }
        break;
    default:
        break;
    }

    return data;
}


/////////////////////////////////////////////////////////////////////////////
//
//  CODBC_SendDataCmd::
//

CODBC_SendDataCmd::CODBC_SendDataCmd(CODBC_Connection& conn,
                                     CDB_ITDescriptor& descr,
                                     size_t nof_bytes,
                                     bool logit) :
    CStatementBase(conn),
    impl::CSendDataCmd(conn, nof_bytes),
    m_DescrType(descr.GetColumnType() == CDB_ITDescriptor::eText ?
                CDB_ITDescriptor::eText : CDB_ITDescriptor::eBinary)
{
    SQLPOINTER p = (SQLPOINTER)1;
    if((!ODBC_xSendDataPrepare(*this, descr, (SQLINTEGER)nof_bytes,
                              false, logit, p, &m_ParamPH)) ||
       (!ODBC_xSendDataGetId(*this, &p))) {

        string err_message = "Cannot prepare a command." + GetDbgInfo();
        DATABASE_DRIVER_ERROR( err_message, 410035 );
    }
}

size_t CODBC_SendDataCmd::SendChunk(const void* chunk_ptr, size_t nof_bytes)
{
    if(nof_bytes > GetBytes2Go()) nof_bytes= GetBytes2Go();
    if(nof_bytes < 1) return 0;

    int rc;

    if (GetClientEncoding() == eEncoding_UTF8 &&
        m_DescrType == CDB_ITDescriptor::eText) {
        size_t valid_len = 0;

        valid_len = CStringUTF8::GetValidBytesCount(static_cast<const char*>(chunk_ptr),
                                                    nof_bytes);

        if (valid_len == 0) {
            DATABASE_DRIVER_ERROR( "Invalid encoding of a text string." + GetDbgInfo(), 410055 );
        }

        // Encoding is always eEncoding_UTF8 in here.
        CODBCString odbc_str(static_cast<const char*>(chunk_ptr),
                             valid_len,
                             eEncoding_UTF8);
        // Force odbc_str to make conversion to odbc::TChar*.
        odbc::TChar* tchar_str = odbc_str;
        nof_bytes = valid_len;

        rc = SQLPutData(GetHandle(),
                        static_cast<SQLPOINTER>(tchar_str),
                        static_cast<SQLINTEGER>(odbc_str.GetSymbolNum() *
                                                sizeof(odbc::TChar))
                        );
    } else {
        rc = SQLPutData(GetHandle(),
                        const_cast<SQLPOINTER>(chunk_ptr),
                        static_cast<SQLINTEGER>(nof_bytes)
                        );
    }


    switch( rc ) {
    case SQL_SUCCESS_WITH_INFO:
        ReportErrors();
    case SQL_NEED_DATA:
    case SQL_NO_DATA:
    case SQL_SUCCESS:
        SetBytes2Go(GetBytes2Go() - nof_bytes);
        if(GetBytes2Go() == 0) break;
        return nof_bytes;
    case SQL_ERROR:
        ReportErrors();
    default:
        return 0;
    }

    SQLPOINTER s= (SQLPOINTER)1;
    switch(SQLParamData(GetHandle(), (SQLPOINTER*)&s)) {
    case SQL_SUCCESS_WITH_INFO: ReportErrors();
    case SQL_SUCCESS:           break;
    case SQL_NO_DATA:           break;
    case SQL_NEED_DATA:
        {
            string err_message = "Not all the data were sent." + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, 410044 );
        }
    case SQL_ERROR:             ReportErrors();
    default:
        {
            string err_message = "SQLParamData failed." + GetDbgInfo();
            DATABASE_DRIVER_ERROR( err_message, 410045 );
        }
    }

    for(;;) {
        switch(SQLMoreResults(GetHandle())) {
        case SQL_SUCCESS_WITH_INFO: ReportErrors();
        case SQL_SUCCESS:           continue;
        case SQL_NO_DATA:           break;
        case SQL_ERROR:
            {
                ReportErrors();
                string err_message = "SQLMoreResults failed." + GetDbgInfo();
                DATABASE_DRIVER_ERROR( err_message, 410014 );
            }
        default:
            {
                string err_message = "SQLMoreResults failed (memory corruption suspected)." + GetDbgInfo();
                DATABASE_DRIVER_ERROR( err_message, 410015 );
            }
        }
        break;
    }

    return nof_bytes;
}

bool CODBC_SendDataCmd::Cancel(void)
{
    if (GetBytes2Go() > 0) {
        xCancel();
        SetBytes2Go(0);
        return true;
    }

    return false;
}

CODBC_SendDataCmd::~CODBC_SendDataCmd()
{
    try {
        DetachInterface();

        GetConnection().DropCmd(*this);

        Cancel();
    }
    NCBI_CATCH_ALL( NCBI_CURRENT_FUNCTION )
}

void CODBC_SendDataCmd::xCancel()
{
    if ( !Close() ) {
        return;
    }
    ResetParams();
}

END_NCBI_SCOPE



