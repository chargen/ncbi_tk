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
*/

#include <ncbi_pch.hpp>
#include <map>
#include "dbapi_cursor.hpp"
#include "../dbapi_sample_base.hpp"

map<string, string> dblib_version;


USING_NCBI_SCOPE;

// This program will CREATE a table with 5 rows , UPDATE text field,
// PRINT table on screen (each row will begin with <ROW> and ended by </ROW>)
// and DELETE table from the database.

/////////////////////////////////////////////////////////////////////////////
//  CDbapiCursorApp::
//

class CDbapiCursorApp : public CDbapiSampleApp
{
public:
    CDbapiCursorApp(void);
    virtual ~CDbapiCursorApp(void);

protected:
    virtual int  RunSample(void);

protected:
    string GetTableName(void) const;
    /// function CreateTable is creating table in the database
    void CreateTable (const string& table_name);
};

CDbapiCursorApp::CDbapiCursorApp(void)
{
}

CDbapiCursorApp::~CDbapiCursorApp(void)
{
}

void
CDbapiCursorApp::CreateTable (const string& table_name)
{
    string sql;

    // Drop a table with same name.
    sql  = string(" IF EXISTS (select * from sysobjects WHERE name = '");
    sql += table_name + "' AND type = 'U') begin ";
    sql += " DROP TABLE " + table_name + " end ";

    auto_ptr<CDB_LangCmd> lcmd(GetConnection().LangCmd (sql));
    lcmd->Send();

    while ( lcmd->HasMoreResults() ) {
        auto_ptr<CDB_Result> r(lcmd->Result());
    }

    // Create a new table.
    sql  = " create table " + table_name + "( \n";
    sql += "    int_val int not null, \n";
    sql += "    fl_val real not null, \n";
    sql += "    date_val datetime not null, \n";
    sql += "    str_val varchar(255) null, \n";
    sql += "    txt_val text null, \n";
    sql += "    primary key clustered(int_val) \n";
    sql += ")";

    lcmd.reset(GetConnection().LangCmd ( sql ));
    lcmd->Send();

    while ( lcmd->HasMoreResults() ) {
        auto_ptr<CDB_Result> r(lcmd->Result());
    }

    lcmd.release();

    auto_ptr<CDB_BCPInCmd> bcp(GetConnection().BCPIn(table_name, 5));

    CDB_Int int_val;
    CDB_Float fl_val;
    CDB_DateTime date_val(CTime::eCurrent);
    CDB_VarChar str_val;
    CDB_Text pTxt;
    int i;
    pTxt.Append("This is a test string.");

    // Bind data from a program variables
    bcp->Bind(0, &int_val);
    bcp->Bind(1, &fl_val);
    bcp->Bind(2, &date_val);
    bcp->Bind(3, &str_val);
    bcp->Bind(4, &pTxt);

    for ( i = 0; *file_name[i] != '\0'; ++i ) {
        int_val = i;
        fl_val = i + 0.999;
        date_val = date_val.Value();

        str_val = file_name[i];

        pTxt.MoveTo(0);
        bcp->SendRow();
    }
    bcp->CompleteBCP();
}


inline
string
CDbapiCursorApp::GetTableName(void) const
{
    return "crs" + GetTableUID();
}

// The following function illustrates a usage of dbapi cursor
int
CDbapiCursorApp::RunSample(void)
{
    try {
        auto_ptr<CDB_LangCmd> set_cmd;

        // Change a default size of text(image)
        GetDriverContext().SetMaxTextImageSize(1000000);

        // Create table in database for the test
        CreateTable(GetTableName());

        CDB_Text txt;

        txt.Append ("This text will replace a text in the table.");

        // Example : update text field in the table CursorSample where int_val = 2,
        // by using cursor

        auto_ptr<CDB_CursorCmd> upd(GetConnection().Cursor("upd",
           "select int_val, txt_val from " + GetTableName() +
           " for update of txt_val", 0));

        //Open cursor
        auto_ptr<CDB_Result> crres(upd->Open());
        //fetch row
        while ( crres->Fetch() ) {
            CDB_Int v;
            crres->GetItem(&v);
            if ( v.Value() == 2 ) {
                txt.MoveTo(0); // rewind
                //update text field
                upd->UpdateTextImage (1, txt);
            }
        }
        //print resutls on the screen
        ShowResults("select int_val,fl_val,date_val,str_val,txt_val from " +
            GetTableName());
        //close cursor
        upd->Close();

        //Delete table from database
        DeleteTable(GetTableName());

        // Drop lost tables.
        DeleteLostTables();
    } catch ( CDB_Exception& e ) {
        CDB_UserHandler::GetDefault().HandleIt(&e);
        return 1;
    }

    return 0;
}

int main(int argc, const char* argv[])
{
    return CDbapiCursorApp().AppMain(argc, argv);
}

/*
 * ===========================================================================
 * $Log$
 * Revision 1.5  2004/12/20 16:20:29  ssikorsk
 * Refactoring of dbapi/driver/samples
 *
 * ===========================================================================
 */
