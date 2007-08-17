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
 * File Description:  CTLib bcp-in command
 *
 */


#include <ncbi_pch.hpp>
#include <dbapi/driver/ctlib/interfaces.hpp>
#include <dbapi/driver/util/numeric_convert.hpp>
#include <string.h>


BEGIN_NCBI_SCOPE

#ifdef FTDS_IN_USE
namespace ftds64_ctlib
{
#endif


///////////////////////////////////////////////////////////////////////////
//
//  CTL_BCPInCmd::
//

CTL_BCPInCmd::CTL_BCPInCmd(CTL_Connection& conn,
                           const string& table_name,
                           unsigned int nof_columns)
: CTL_CmdBase(conn)
, impl::CBaseCmd(conn, table_name, nof_columns)
, m_Bind(nof_columns)
, m_RowCount(0)
{
    CheckSF(
        blk_alloc(
            GetConnection().GetNativeConnection().GetNativeHandle(),
            GetConnection().GetBLKVersion(),
            &m_Cmd
            ),
        "blk_alloc failed", 110004
        );

    SetExecCntxInfo("BCP table name: " + table_name);
}


CS_RETCODE
CTL_BCPInCmd::CheckSF(CS_RETCODE rc, const char* msg, unsigned int msg_num)
{
    switch (Check(rc)) {
    case CS_SUCCEED:
        break;
    case CS_FAIL:
        SetHasFailed();
        DATABASE_DRIVER_ERROR( msg, msg_num );
    }

    return rc;
}



CS_RETCODE
CTL_BCPInCmd::CheckSFB(CS_RETCODE rc, const char* msg, unsigned int msg_num)
{
    switch (Check(rc)) {
    case CS_SUCCEED:
        break;
    case CS_FAIL:
        SetHasFailed();
        DATABASE_DRIVER_ERROR( msg, msg_num );
#ifdef CS_BUSY
    case CS_BUSY:
        DATABASE_DRIVER_ERROR( "the connection is busy", 122002 );
#endif
    }

    return rc;
}


CS_RETCODE
CTL_BCPInCmd::CheckSentSFB(CS_RETCODE rc, const char* msg, unsigned int msg_num)
{
    switch (Check(rc)) {
    case CS_SUCCEED:
        SetWasSent(false);
        break;
    case CS_FAIL:
        SetHasFailed();
        DATABASE_DRIVER_ERROR( msg, msg_num );
#ifdef CS_BUSY
    case CS_BUSY:
        SetWasSent(false);
        // DATABASE_DRIVER_ERROR( "the connection is busy", 122002 );
#endif
    }

    return rc;
}



bool CTL_BCPInCmd::Bind(unsigned int column_num, CDB_Object* pVal)
{
    return GetParams().BindParam(column_num, kEmptyStr, pVal);
}


bool CTL_BCPInCmd::x_IsUnicodeClientAPI(void) const
{
    switch (GetConnection().GetCTLibContext().GetTDSVersion()) {
    case 70:
    case 80:
        return true;
    };

    return false;
}


CS_VOID*
CTL_BCPInCmd::x_GetValue(const CDB_Char& value) const
{
//--------------------------------------------------
// #if defined(HAVE_WSTRING)
//     if (x_IsUnicodeClientAPI()) {
//         return const_cast<CS_VOID*>(
//             static_cast<const CS_VOID*>(value.AsUnicode(eEncoding_UTF8)));
//     }
// #endif
//--------------------------------------------------

    return const_cast<CS_VOID*>(
        static_cast<const CS_VOID*>(value.Value()));
}


CS_VOID*
CTL_BCPInCmd::x_GetValue(const CDB_VarChar& value) const
{
//--------------------------------------------------
// #if defined(HAVE_WSTRING)
//     if (x_IsUnicodeClientAPI()) {
//         return const_cast<CS_VOID*>(
//             static_cast<const CS_VOID*>(value.AsUnicode(eEncoding_UTF8)));
//     }
// #endif
//--------------------------------------------------

    return const_cast<CS_VOID*>(
        static_cast<const CS_VOID*>(value.Value()));
}


bool CTL_BCPInCmd::x_AssignParams()
{
    CS_DATAFMT param_fmt;
    memset(&param_fmt, 0, sizeof(param_fmt));
    param_fmt.format = CS_FMT_UNUSED;
    param_fmt.count  = 1;

    for (unsigned int i = 0;  i < GetParams().NofParams();  i++) {

        if (GetParams().GetParamStatus(i) == 0)
            continue;

        CDB_Object& param = *GetParams().GetParam(i);
        m_Bind[i].indicator = param.IsNULL() ? -1 : 0;
        m_Bind[i].datalen   = (m_Bind[i].indicator == 0) ? CS_UNUSED : 0;

        CS_RETCODE ret_code;

        switch ( param.GetType() ) {
        case eDB_Int: {
            CDB_Int& par = dynamic_cast<CDB_Int&> (param);
            param_fmt.datatype = CS_INT_TYPE;
            CS_INT value = (CS_INT) par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_INT));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_SmallInt: {
            CDB_SmallInt& par = dynamic_cast<CDB_SmallInt&> (param);
            param_fmt.datatype = CS_SMALLINT_TYPE;
            CS_SMALLINT value = (CS_SMALLINT) par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_SMALLINT));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_TinyInt: {
            CDB_TinyInt& par = dynamic_cast<CDB_TinyInt&> (param);
            param_fmt.datatype = CS_TINYINT_TYPE;
            CS_TINYINT value = (CS_TINYINT) par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_TINYINT));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_BigInt: {
            CDB_BigInt& par = dynamic_cast<CDB_BigInt&> (param);

            /* Old code ...
            param_fmt.datatype = CS_NUMERIC_TYPE;
            CS_NUMERIC value;
            Int8 val8 = par.Value();
            memset(&value, 0, sizeof(value));
            value.precision = 20;
            if (longlong_to_numeric(val8, 20, value.array) == 0)
                return false;
            param_fmt.scale     = 0;
            param_fmt.precision = 20;
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_NUMERIC));
            m_Bind[i].datalen = sizeof(CS_NUMERIC);
            */

            param_fmt.datatype = CS_LONG_TYPE;
            Int8 value = par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(value));

            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Char: {
            CDB_Char& par = dynamic_cast<CDB_Char&> (param);
            param_fmt.datatype  = CS_CHAR_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size() + 1;
            m_Bind[i].datalen   =
                (m_Bind[i].indicator == -1) ? 0 : (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                          x_GetValue(par),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_LongChar: {
            CDB_LongChar& par = dynamic_cast<CDB_LongChar&> (param);
            param_fmt.datatype  = CS_LONGCHAR_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size() + 1;
            m_Bind[i].datalen   =
                (m_Bind[i].indicator == -1) ? 0 : (CS_INT) par.DataSize();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                        (CS_VOID*) par.Value(),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_VarChar: {
            CDB_VarChar& par = dynamic_cast<CDB_VarChar&> (param);
#ifdef FTDS_IN_USE
            // FreeTDS will work fine with CS_CHAR_TYPE either ...
            param_fmt.datatype  = CS_VARCHAR_TYPE;
#else
            // There is a problem with CS_VARCHAR_TYPE on x86_64 ...
            param_fmt.datatype  = CS_CHAR_TYPE;
#endif
            // param_fmt.maxlength = (CS_INT) par.Size() + 1;
            param_fmt.maxlength = (CS_INT) par.Size();
            m_Bind[i].datalen   = (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                          x_GetValue(par),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Binary: {
            CDB_Binary& par = dynamic_cast<CDB_Binary&> (param);
            param_fmt.datatype  = CS_BINARY_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size() + 1;
            m_Bind[i].datalen   =
                (m_Bind[i].indicator == -1) ? 0 : (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                        (CS_VOID*) par.Value(),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_LongBinary: {
            CDB_LongBinary& par = dynamic_cast<CDB_LongBinary&> (param);
            param_fmt.datatype  = CS_LONGBINARY_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size();
            m_Bind[i].datalen   =
                (m_Bind[i].indicator == -1) ? 0 : (CS_INT) par.DataSize();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                        // par.Value() may return NULL. But NULL has a special
                        // meaning for this parameter. So, it is replaced a by
                        // a fake pointer (m_Bind[i].buffer).
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                        (CS_VOID*) par.Value(),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_VarBinary: {
            CDB_VarBinary& par = dynamic_cast<CDB_VarBinary&> (param);
            param_fmt.datatype  = CS_BINARY_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size() + 1;
            m_Bind[i].datalen   = (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      par.IsNULL()? (CS_VOID*)m_Bind[i].buffer :
                                        (CS_VOID*) par.Value(),
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Float: {
            CDB_Float& par = dynamic_cast<CDB_Float&> (param);
            param_fmt.datatype = CS_REAL_TYPE;
            CS_REAL value = (CS_REAL) par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_REAL));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Double: {
            CDB_Double& par = dynamic_cast<CDB_Double&> (param);
            param_fmt.datatype = CS_FLOAT_TYPE;
            CS_FLOAT value = (CS_FLOAT) par.Value();
            memcpy(m_Bind[i].buffer, &value, sizeof(CS_FLOAT));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_SmallDateTime: {
            CDB_SmallDateTime& par = dynamic_cast<CDB_SmallDateTime&> (param);
            param_fmt.datatype = CS_DATETIME4_TYPE;

            CS_DATETIME4 dt;
            if (param.IsNULL()) {
                dt.days    = 0;
                dt.minutes = 0;
            } else {
                dt.days    = par.GetDays();
                dt.minutes = par.GetMinutes();
            }

            memcpy(m_Bind[i].buffer, &dt, sizeof(CS_DATETIME4));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_DateTime: {
            CDB_DateTime& par = dynamic_cast<CDB_DateTime&> (param);
            param_fmt.datatype = CS_DATETIME_TYPE;

            CS_DATETIME dt;
            if (param.IsNULL()) {
                dt.dtdays = 0;
                dt.dttime = 0;
            } else {
                dt.dtdays = par.GetDays();
                dt.dttime = par.Get300Secs();
            }

            _ASSERT(sizeof(CS_NUMERIC) >= sizeof(CS_DATETIME));
            memcpy(m_Bind[i].buffer, &dt, sizeof(CS_DATETIME));
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      (CS_VOID*) m_Bind[i].buffer,
                                      &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Text: {
            CDB_Text& par = dynamic_cast<CDB_Text&> (param);
            param_fmt.datatype  = CS_TEXT_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size();
            m_Bind[i].datalen   = (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      0, &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        case eDB_Image: {
            CDB_Image& par = dynamic_cast<CDB_Image&> (param);
            param_fmt.datatype  = CS_IMAGE_TYPE;
            param_fmt.maxlength = (CS_INT) par.Size();
            m_Bind[i].datalen   = (CS_INT) par.Size();
            ret_code = Check(blk_bind(x_GetSybaseCmd(), i + 1, &param_fmt,
                                      0, &m_Bind[i].datalen,
                                      &m_Bind[i].indicator));
            break;
        }
        default:
            return false;
        }

        if (ret_code != CS_SUCCEED) {
            return false;
        }
    }

    return true;
}


#if defined(HAVE_WSTRING)
static
string MakeUCS2LE(const wstring& str)
{
    string result;

#if defined(WORDS_BIGENDIAN)
    if (!str.empty()) {
        result.resize(str.size() * 2);
        for(wstring::size_type i = 0; i < str.size(); ++i) {
            wchar_t chracter = str.at(i);

            result.at(i * 2) = (chracter & 0x000000FF);
            result.at(i * 2 + 1) = (chracter & 0x0000FF00);
        }
    }
#else
    result.assign((const char*)str.c_str(), str.size() * sizeof(wchar_t));
#endif

    return result;
}
#endif


bool CTL_BCPInCmd::Send(void)
{
    unsigned int i;
    CS_INT       datalen = 0;
    CS_SMALLINT  indicator = -1; // NULL value
    size_t       len = 0;
    char         buff[2048];

    if ( !WasSent() ) {
        // we need to init the bcp
        CheckSFB(blk_init(x_GetSybaseCmd(), CS_BLK_IN, (CS_CHAR*) GetQuery().c_str(), CS_NULLTERM),
                 "blk_init failed", 123001);

        SetWasSent();

        // check what needs to be default
        CS_DATAFMT fmt;

        for (i = 0;  i < GetParams().NofParams();  i++) {
            if (GetParams().GetParamStatus(i) != 0) {
                continue;
            }


            SetHasFailed((Check(blk_describe(x_GetSybaseCmd(),
                                             i + 1,
                                             &fmt)) != CS_SUCCEED));
            CHECK_DRIVER_ERROR(
                HasFailed(),
                "blk_describe failed (check the number of "
                "columns in a table)." + GetDbgInfo(),
                123002 );

            // It is a fake binding ...
            SetHasFailed((Check(blk_bind(x_GetSybaseCmd(),
                                         i + 1,
                                         &fmt,
                                         // This adderess is semantically
                                         // invalid, but we have to pass
                                         // something except of NULL.
                                         (void*) &GetParams(),
                                         &datalen,
                                         &indicator)) != CS_SUCCEED));
            CHECK_DRIVER_ERROR(
                HasFailed(),
                "blk_bind failed for default value." + GetDbgInfo(),
                123003 );

            // GetParams().SetParamStatus(i, CDB_Params::fSet);
        }
    }


    SetHasFailed(!x_AssignParams());
    CHECK_DRIVER_ERROR( HasFailed(), "Cannot assign the params." + GetDbgInfo(), 123004 );

    switch ( Check(blk_rowxfer(x_GetSybaseCmd())) ) {
    case CS_BLK_HAS_TEXT:
        for (i = 0;  i < GetParams().NofParams();  i++) {
            if (GetParams().GetParamStatus(i) == 0)
                continue;

            CDB_Object& param = *GetParams().GetParam(i);

            if (param.GetType() == eDB_Text) {
                size_t valid_len = 0;
                size_t invalid_len = 0;
                CDB_Stream& par = dynamic_cast<CDB_Stream&> (param);

                for (datalen = (CS_INT) par.Size();  datalen > 0;
                     datalen -= (CS_INT) len)
                {
                    len = par.Read(buff + invalid_len, sizeof(buff) - invalid_len);

                    valid_len = CStringUTF8::GetValidBytesCount(buff, len);
                    invalid_len = len - valid_len;

#if defined(HAVE_WSTRING)
                    if (x_IsUnicodeClientAPI()) {
                        CWString unicode_str(buff, valid_len, eEncoding_UTF8);

                        string wcharle_str = MakeUCS2LE(unicode_str.AsUnicode(eEncoding_UTF8));

                        SetHasFailed((Check(
                            blk_textxfer(
                                x_GetSybaseCmd(),
                                (CS_BYTE*) wcharle_str.c_str(),
                                (CS_INT) wcharle_str.size(),
                                0)
                            ) == CS_FAIL));
                    } else {
                        SetHasFailed((Check(blk_textxfer(x_GetSybaseCmd(),
                                                         (CS_BYTE*) buff,
                                                         (CS_INT) valid_len,
                                                         0)
                                             ) == CS_FAIL));
                    }
#else
                    SetHasFailed((Check(blk_textxfer(x_GetSybaseCmd(),
                                                     (CS_BYTE*) buff,
                                                     (CS_INT) valid_len,
                                                     0)
                                         ) == CS_FAIL));
#endif

                    CHECK_DRIVER_ERROR(
                        HasFailed(),
                        "blk_textxfer failed for the text/image field." + GetDbgInfo(), 123005
                        );

                    if (valid_len < len) {
                        memmove(buff, buff + valid_len, invalid_len);
                    }
                }
            } else if (param.GetType() == eDB_Image) {
                CDB_Stream& par = dynamic_cast<CDB_Stream&> (param);

                for (datalen = (CS_INT) par.Size();  datalen > 0; datalen -= (CS_INT) len) {
                    len = par.Read(buff, sizeof(buff));

                    SetHasFailed((Check(blk_textxfer(x_GetSybaseCmd(), (CS_BYTE*) buff, (CS_INT) len, 0)) == CS_FAIL));

                    CHECK_DRIVER_ERROR(
                        HasFailed(),
                        "blk_textxfer failed for the text/image field." + GetDbgInfo(), 123005
                        );
                }
            } else {
                continue;
            }
        }
    case CS_SUCCEED:
        ++m_RowCount;
        return true;
    default:
        SetHasFailed();
        CHECK_DRIVER_ERROR( HasFailed(), "blk_rowxfer failed." + GetDbgInfo(), 123007 );
    }

    return false;
}


bool CTL_BCPInCmd::Cancel()
{
    if(WasSent()) {
        CS_INT outrow = 0;

        return (CheckSentSFB(blk_done(x_GetSybaseCmd(), CS_BLK_CANCEL, &outrow),
                             "blk_done failed", 123020) == CS_SUCCEED);
    }

    return true;
}

bool CTL_BCPInCmd::CommitBCPTrans(void)
{
    if(!WasSent()) return false;

    CS_INT outrow = 0;

    switch( Check(blk_done(x_GetSybaseCmd(), CS_BLK_BATCH, &outrow)) ) {
    case CS_SUCCEED:
        return (outrow > 0);
    case CS_FAIL:
        SetHasFailed();
        DATABASE_DRIVER_ERROR( "blk_done failed." + GetDbgInfo(), 123020 );
    default:
        return false;
    }
}


bool CTL_BCPInCmd::EndBCP(void)
{
    if(!WasSent()) return false;

    CS_INT outrow = 0;

    if (CheckSentSFB(blk_done(x_GetSybaseCmd(), CS_BLK_ALL, &outrow),
                     "blk_done failed", 123020) == CS_SUCCEED) {
        return (outrow > 0);
    }

    return false;
}


int CTL_BCPInCmd::RowCount(void) const
{
    return m_RowCount;
}


CTL_BCPInCmd::~CTL_BCPInCmd()
{
    try {
        DetachInterface();

        DropCmd(*this);

        Close();

        if (!IsDead()) {
            Check(blk_drop(x_GetSybaseCmd()));
        }
    }
    NCBI_CATCH_ALL( NCBI_CURRENT_FUNCTION )
}


void
CTL_BCPInCmd::Close(void)
{
    if (x_GetSybaseCmd()) {
        // ????
        DetachInterface();

        try {
            SetDead(!Cancel());
        } catch (...) {
            SetDead();
            throw;
        }
    }
}


#ifdef FTDS_IN_USE
} // namespace ftds64_ctlib
#endif

END_NCBI_SCOPE

