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
 * File Description:  DBLib language command
 *
 */
#include <ncbi_pch.hpp>

#include <dbapi/driver/dblib/interfaces.hpp>
#include <dbapi/driver/dblib/interfaces_p.hpp>

#include <stdio.h>

BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
//
//  CDBL_LangCmd::
//

CDBL_LangCmd::CDBL_LangCmd(CDBL_Connection& conn,
                           DBPROCESS* cmd,
                           const string& lang_query,
                           unsigned int nof_params)
: CDBL_Cmd(conn, cmd)
, impl::CBaseCmd(conn, lang_query, nof_params)
, m_Res(0)
, m_Status(0)
{
    SetExecCntxInfo("SQL Command: \"" + lang_query + "\"");
}


bool CDBL_LangCmd::Send()
{
    Cancel();

    SetHasFailed(false);

    if (!x_AssignParams()) {
        dbfreebuf(GetCmd());
        CheckFunctCall();
        SetHasFailed();
        DATABASE_DRIVER_ERROR( "Cannot assign params." + GetDbgInfo(), 220003 );
    }

    if (Check(dbcmd(GetCmd(), (char*)(GetQuery().c_str()))) != SUCCEED) {
        dbfreebuf(GetCmd());
        CheckFunctCall();
        SetHasFailed();
        DATABASE_DRIVER_ERROR( "dbcmd failed." + GetDbgInfo(), 220001 );
    }


    // Timeout is already set by CDBLibContext ...
    // GetConnection().x_SetTimeout();

    SetHasFailed(Check(dbsqlsend(GetCmd())) != SUCCEED);
    CHECK_DRIVER_ERROR(
        HasFailed(),
        "dbsqlsend failed." + GetDbgInfo(),
        220005 );

    SetWasSent();
    m_Status = 0;
    return true;
}


bool CDBL_LangCmd::Cancel()
{
    if (WasSent()) {
        if ( GetResultSet() ) {
#if 1 && defined(FTDS_IN_USE)
            while (GetResultSet()->Fetch())
               continue;
#endif
            ClearResultSet();
        }
        SetWasSent(false);

        dbfreebuf(GetCmd());
        CheckFunctCall();
        // m_Query.erase();

        return (Check(dbcancel(GetCmd())) == SUCCEED);
    }
    return true;
}


CDB_Result* CDBL_LangCmd::Result()
{
    if ( GetResultSet() ) {
        if(m_RowCount < 0) {
            m_RowCount = DBCOUNT(GetCmd());
        }

#if 1 && defined(FTDS_IN_USE)
        // This Fetch is required by FreeTDS v063
        // Useful stuff
        while (GetResultSet()->Fetch())
            continue;
#endif

        ClearResultSet();
    }

    CHECK_DRIVER_ERROR( !WasSent(), "A command has to be sent first." + GetDbgInfo(), 220010 );

    if (m_Status == 0) {
        m_Status = 0x1;
        if (Check(dbsqlok(GetCmd())) != SUCCEED) {
            SetWasSent(false);
            SetHasFailed();
            DATABASE_DRIVER_ERROR( "dbsqlok failed." + GetDbgInfo(), 220011 );
        }
    }

    if ((m_Status & 0x10) != 0) { // we do have a compute result
        SetResultSet( new CDBL_ComputeResult(GetConnection(), GetCmd(), &m_Status) );
        m_RowCount= 1;

        return Create_Result(*GetResultSet());
    }

    while ((m_Status & 0x1) != 0) {
#if !defined(FTDS_LOGIC)
        if ((m_Status & 0x20) != 0) { // check for return parameters from exec
            m_Status ^= 0x20;
            int n = Check(dbnumrets(GetCmd()));
            if (n > 0) {
                SetResultSet( new CDBL_ParamResult(GetConnection(), GetCmd(), n) );
                m_RowCount= 1;

                return Create_Result(*GetResultSet());
            }
        }

        if ((m_Status & 0x40) != 0) { // check for ret status
            m_Status ^= 0x40;
            DBBOOL has_return_status = Check(dbhasretstat(GetCmd()));
            if (has_return_status) {
                SetResultSet( new CDBL_StatusResult(GetConnection(), GetCmd()) );
                m_RowCount= 1;

                return Create_Result(*GetResultSet());
            }
        }
#endif

        RETCODE rc = Check(dbresults(GetCmd()));

        switch (rc) {
        case SUCCEED:
#if !defined(FTDS_LOGIC)
            m_Status |= 0x60;
#endif
            if (DBCMDROW(GetCmd()) == SUCCEED) { // we could get rows in result

// This optimization is currently unavailable for MS dblib...
#ifndef MS_DBLIB_IN_USE /*Text,Image*/
                if (Check(dbnumcols(GetCmd())) == 1) {
                    int ct = Check(dbcoltype(GetCmd(), 1));
                    if ((ct == SYBTEXT) || (ct == SYBIMAGE)) {
                        SetResultSet( new CDBL_BlobResult(GetConnection(), GetCmd()) );
                    }
                }
#endif
                if ( !GetResultSet() ) {
                    SetResultSet( new CDBL_RowResult(GetConnection(), GetCmd(), &m_Status) );
                }

                m_RowCount= -1;

                return Create_Result(*GetResultSet());
            } else {
                m_RowCount = DBCOUNT(GetCmd());
                continue;
            }
        case NO_MORE_RESULTS:
            m_Status = 2;
            break;
        default:
            SetHasFailed();
            CHECK_DRIVER_WARNING( HasFailed(), "an error was encountered by server", 220016 );
        }
        break;
    }

#if defined(FTDS_LOGIC)
    // we've done with the row results at this point
    // let's look at return parameters and ret status
    if (m_Status == 2) {
        m_Status = 4;
        int n = Check(dbnumrets(GetCmd()));
        if (n > 0) {
            SetResultSet( new CTDS_ParamResult(GetConnection(), GetCmd(), n) );
            m_RowCount = 1;

            return Create_Result(*GetResultSet());
        }
    }

    if (m_Status == 4) {
        m_Status = 6;
        DBBOOL has_return_status = Check(dbhasretstat(GetCmd()));
        if (has_return_status) {
            SetResultSet( new CTDS_StatusResult(GetConnection(), GetCmd()) );
            m_RowCount = 1;

            return Create_Result(*GetResultSet());
        }
    }
#endif

    SetWasSent(false);
    return 0;
}


bool CDBL_LangCmd::HasMoreResults() const
{
    return WasSent();
}

int CDBL_LangCmd::RowCount() const
{
    return (m_RowCount < 0)? DBCOUNT(GetCmd()) : m_RowCount;
}


CDBL_LangCmd::~CDBL_LangCmd()
{
    try {
        DetachInterface();

        GetConnection().DropCmd(*this);

        Cancel();
    }
    NCBI_CATCH_ALL( NCBI_CURRENT_FUNCTION )
}


bool CDBL_LangCmd::x_AssignParams()
{
    static const char s_hexnum[] = "0123456789ABCDEF";

    for (unsigned int n = 0; n < GetParams().NofParams(); n++) {
        if(GetParams().GetParamStatus(n) == 0) continue;
        const string& name  =  GetParams().GetParamName(n);
        CDB_Object&   param = *GetParams().GetParam(n);
        char          val_buffer[16*1024];
        const char*   type;
        string        cmd;

        if (name.length() > kDBLibMaxNameLen)
            return false;

        switch (param.GetType()) {
        case eDB_Int:
            type = "int";
            break;
        case eDB_SmallInt:
            type = "smallint";
            break;
        case eDB_TinyInt:
            type = "tinyint";
            break;
        case eDB_BigInt:
            type = "numeric";
            break;
        case eDB_Char:
        case eDB_VarChar:
            type = "varchar(255)";
            break;
        case eDB_LongChar: {
            CDB_LongChar& lc = dynamic_cast<CDB_LongChar&> (param);
            sprintf(val_buffer, "varchar(%d)", int(lc.Size()));
            type= val_buffer;
            break;
        }
        case eDB_Binary:
        case eDB_VarBinary:
            type = "varbinary(255)";
            break;
        case eDB_LongBinary: {
            CDB_LongBinary& lb = dynamic_cast<CDB_LongBinary&> (param);
            if(lb.DataSize()*2 > (sizeof(val_buffer) - 4)) return false;
            sprintf(val_buffer, "varbinary(%d)", int(lb.Size()));
            type= val_buffer;
            break;
        }
        case eDB_Float:
            type = "real";
            break;
        case eDB_Double:
            type = "float";
            break;
        case eDB_SmallDateTime:
            type = "smalldatetime";
            break;
        case eDB_DateTime:
            type = "datetime";
            break;
        default:
            return false;
        }

        cmd += "declare " + name + ' ' + type + "\nselect " + name + " = ";

        if (!param.IsNULL()) {
            switch (param.GetType()) {
            case eDB_Int: {
                CDB_Int& val = dynamic_cast<CDB_Int&> (param);
                sprintf(val_buffer, "%d\n", val.Value());
                break;
            }
            case eDB_SmallInt: {
                CDB_SmallInt& val = dynamic_cast<CDB_SmallInt&> (param);
                sprintf(val_buffer, "%d\n", (int) val.Value());
                break;
            }
            case eDB_TinyInt: {
                CDB_TinyInt& val = dynamic_cast<CDB_TinyInt&> (param);
                sprintf(val_buffer, "%d\n", (int) val.Value());
                break;
            }
            case eDB_BigInt: {
                CDB_BigInt& val = dynamic_cast<CDB_BigInt&> (param);
                sprintf(val_buffer, "%lld\n", val.Value());
                break;
            }
            case eDB_Char: {
                CDB_Char& val = dynamic_cast<CDB_Char&> (param);
                const char* c = val.Value(); // No more than 255 chars
                size_t i = 0;
                val_buffer[i++] = '\'';
                while (*c) {
                    if (*c == '\'')
                        val_buffer[i++] = '\'';
                    val_buffer[i++] = *c++;
                }
                val_buffer[i++] = '\'';
                val_buffer[i++] = '\n';
                val_buffer[i] = '\0';
                break;
            }
            case eDB_VarChar: {
                CDB_VarChar& val = dynamic_cast<CDB_VarChar&> (param);
                const char* c = val.Value();
                size_t i = 0;
                val_buffer[i++] = '\'';
                while (*c) {
                    if (*c == '\'')
                        val_buffer[i++] = '\'';
                    val_buffer[i++] = *c++;
                }
                val_buffer[i++] = '\'';
                val_buffer[i++] = '\n';
                val_buffer[i] = '\0';
                break;
            }
            case eDB_LongChar: {
                CDB_LongChar& val = dynamic_cast<CDB_LongChar&> (param);
                const char* c = val.Value();
                size_t i = 0;
                val_buffer[i++] = '\'';
                while (*c && (i < (sizeof(val_buffer)-3))) {
                    if (*c == '\'')
                        val_buffer[i++] = '\'';
                    val_buffer[i++] = *c++;
                }
                val_buffer[i++] = '\'';
                val_buffer[i++] = '\n';
                val_buffer[i] = '\0';
                if(*c != '\0') return false;
                break;
            }
            case eDB_Binary: {
                CDB_Binary& val = dynamic_cast<CDB_Binary&> (param);
                const unsigned char* c = (const unsigned char*) val.Value();
                size_t i = 0, size = val.Size();
                val_buffer[i++] = '0';
                val_buffer[i++] = 'x';
                for (size_t j = 0; j < size; j++) {
                    val_buffer[i++] = s_hexnum[c[j] >> 4];
                    val_buffer[i++] = s_hexnum[c[j] & 0x0F];
                }
                val_buffer[i++] = '\n';
                val_buffer[i++] = '\0';
                break;
            }
            case eDB_VarBinary: {
                CDB_VarBinary& val = dynamic_cast<CDB_VarBinary&> (param);
                const unsigned char* c = (const unsigned char*) val.Value();
                size_t i = 0, size = val.Size();
                val_buffer[i++] = '0';
                val_buffer[i++] = 'x';
                for (size_t j = 0; j < size; j++) {
                    val_buffer[i++] = s_hexnum[c[j] >> 4];
                    val_buffer[i++] = s_hexnum[c[j] & 0x0F];
                }
                val_buffer[i++] = '\n';
                val_buffer[i++] = '\0';
                break;
            }
            case eDB_LongBinary: {
                CDB_LongBinary& val = dynamic_cast<CDB_LongBinary&> (param);
                const unsigned char* c = (const unsigned char*) val.Value();
                size_t i = 0, size = val.DataSize();
                val_buffer[i++] = '0';
                val_buffer[i++] = 'x';
                for (size_t j = 0; j < size; j++) {
                    val_buffer[i++] = s_hexnum[c[j] >> 4];
                    val_buffer[i++] = s_hexnum[c[j] & 0x0F];
                }
                val_buffer[i++] = '\n';
                val_buffer[i++] = '\0';
                break;
            }
            case eDB_Float: {
                CDB_Float& val = dynamic_cast<CDB_Float&> (param);
                sprintf(val_buffer, "%E\n", (double) val.Value());
                break;
            }
            case eDB_Double: {
                CDB_Double& val = dynamic_cast<CDB_Double&> (param);
                sprintf(val_buffer, "%E\n", val.Value());
                break;
            }
            case eDB_SmallDateTime: {
                CDB_SmallDateTime& val =
                    dynamic_cast<CDB_SmallDateTime&> (param);
                sprintf(val_buffer, "'%s'\n",
            val.Value().AsString("M/D/Y h:m").c_str());
                break;
            }
            case eDB_DateTime: {
                CDB_DateTime& val =
                    dynamic_cast<CDB_DateTime&> (param);
                sprintf(val_buffer, "'%s:%.3d'\n",
            val.Value().AsString("M/D/Y h:m:s").c_str(),
            (int)(val.Value().NanoSecond()/1000000));
                break;
            }
            default:
                return false; // dummy for compiler
            }
            cmd += val_buffer;
        } else
            cmd += "NULL\n";
        if (Check(dbcmd(GetCmd(), (char*) cmd.c_str())) != SUCCEED)
            return false;
    }
    return true;
}


END_NCBI_SCOPE


