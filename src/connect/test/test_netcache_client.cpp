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
 * Authors:  Anatoliy Kuznetsov
 *
 * File Description:  NetCache client test
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbireg.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbimisc.hpp>

#include <connect/netcache_client.hpp>
#include <connect/ncbi_socket.hpp>
/* This header must go last */
#include "test_assert.h"


USING_NCBI_SCOPE;

    
///////////////////////////////////////////////////////////////////////


/// Test application
///
/// @internal
///
class CTestNetCacheClient : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};


/// @internal

struct STransactionInfo
{
    string    key;
    size_t    blob_size;
    double    transaction_time;
    double    connection_time;
};



/// @internal
static 
bool s_CheckExists(const string& host, unsigned short port, const string& key)
{
    CSocket sock(host, port);
    CNetCacheClient nc_client(&sock);

    unsigned char dataBuf[1024] = {0,};
    CNetCacheClient::EReadResult rres = 
        nc_client.GetData(key, dataBuf, sizeof(dataBuf));

    if (rres == CNetCacheClient::eNotFound)
        return false;

    return true;
}

/// @internal
static
void s_PutBlob(const string&             host,
               unsigned short            port,
               const void*               buf, 
               size_t                    size, 
               vector<STransactionInfo>* log)
{
    STransactionInfo info;
    info.blob_size = size;

    CStopWatch sw(true);
    CNetCacheClient nc_client(host, port);

    info.connection_time = sw.Elapsed();
    sw.Restart();
    info.key = nc_client.PutData(buf, size, 60 * 5);
    info.transaction_time = sw.Elapsed();

    log->push_back(info);
}

/// @internal
static
void s_ReportStatistics(const vector<STransactionInfo>& log)
{
    double sum, sum_conn, sum_tran;
    sum = sum_conn = sum_tran = 0.0;
    ITERATE(vector<STransactionInfo>, it, log) {
        sum_conn += it->connection_time;
        sum_tran += it->transaction_time;
        sum += it->connection_time + it->transaction_time;
    }
    double avg, avg_conn, avg_tran;
    avg = sum / double(log.size());
    avg_conn = sum_conn / double(log.size());
    avg_tran = sum_tran / double(log.size());

    NcbiCout << "Sum, Conn, Trans" << endl;
    NcbiCout << fixed << sum << ", " << sum_conn << ", " << sum_tran << NcbiEndl;
    NcbiCout << "Avg, Conn, Trans" << endl;
    NcbiCout << fixed << avg << ", " << avg_conn << ", " << avg_tran << NcbiEndl;

}



/// @internal
static
void s_StressTest(const string&             host,
                  unsigned short            port,
                  size_t                    size, 
                  unsigned int              repeats,
                  vector<STransactionInfo>* log)
{
    cout << "Stress test. BLOB size = " << size 
         << " Repeats = " << repeats
         << ". Please wait... " << endl;

    AutoPtr<char, ArrayDeleter<char> > buf = new char[size];
    memset(buf.get(), 0, size);

    for (unsigned i = 0; i < repeats; ++i) {
        s_PutBlob(host, port, buf.get(), size, log);

        if (i % 1000 == 0) cout << "." << flush;
    }
    cout << endl;
}



void CTestNetCacheClient::Init(void)
{
    // Setup command line arguments and parameters

    // Create command-line argument descriptions class
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // Specify USAGE context
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Net cache client");
    
    arg_desc->AddPositional("hostname", 
                            "NetCache host name.", CArgDescriptions::eString);

    arg_desc->AddPositional("port",
                            "Port number.", 
                            CArgDescriptions::eInteger);
    
    // Setup arg.descriptions for this application
    SetupArgDescriptions(arg_desc.release());
}



int CTestNetCacheClient::Run(void)
{
    CArgs args = GetArgs();
    const string& host  = args["hostname"].AsString();
    unsigned short port = args["port"].AsInteger();

    const char test_data[] = "A quick brown fox, jumps over lazy dog.";
    string key;

    {{
        CSocket sock(host, port);
        STimeout to = {0,0};
        sock.SetTimeout(eIO_ReadWrite, &to);
        CNetCacheClient nc_client(&sock);

        key = nc_client.PutData(test_data, sizeof(test_data));

        assert(!key.empty());
    }}

    {{
        CSocket sock(host, port);
        CNetCacheClient nc_client(&sock);

        char dataBuf[1024];
        IReader* reader = nc_client.GetData(key);
        assert(reader);
        reader->Read(dataBuf, 1024);
        delete reader;

        int res = strcmp(dataBuf, test_data);
        assert(res == 0);
    }}

    {{
        CSocket sock(host, port);
        CNetCacheClient nc_client(&sock);

        char dataBuf[1024] = {0};
        CNetCacheClient::EReadResult rres = 
            nc_client.GetData(key, dataBuf, sizeof(dataBuf));
        assert(rres == CNetCacheClient::eReadComplete);

        int res = strcmp(dataBuf, test_data);
        assert(res == 0);
        sock.Close();
    }}

    // timeout test
    {{
        CSocket sock(host, port);
        CNetCacheClient nc_client(&sock);

        key = nc_client.PutData(test_data, sizeof(test_data), 60);
        assert(!key.empty());
        sock.Close();
    }}

    bool exists;
    exists = s_CheckExists(host, port, key);
    assert(exists);
/*
    unsigned delay = 70;
    cout << "Sleeping for " << delay << " seconds. Please wait...." << flush;
    SleepSec(delay);
    cout << "ok." << endl;

    exists = s_CheckExists(host, port, key);
    assert(!exists);
*/

    vector<STransactionInfo> log;

    unsigned repeats = 5000;
    s_StressTest(host, port, 256, repeats, &log);
    s_ReportStatistics(log);
    NcbiCout << NcbiEndl;


    s_StressTest(host, port, 1024 * 5, repeats, &log);
    s_ReportStatistics(log);
    NcbiCout << NcbiEndl;


    s_StressTest(host, port, 1024 * 1024 * 5, repeats/10, &log);
    s_ReportStatistics(log);
    NcbiCout << NcbiEndl;


    cout << "Shutdown server" << endl;
    // Shutdown server
    {{
        CNetCacheClient nc_client(host, port);
        nc_client.ShutdownServer();
    }}
    
    return 0;
}


int main(int argc, const char* argv[])
{
    return CTestNetCacheClient().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.5  2004/10/20 15:37:19  kuznets
 * Added stress test for different BLOB sizes
 *
 * Revision 1.4  2004/10/13 12:54:34  kuznets
 * +Stress test
 *
 * Revision 1.3  2004/10/08 12:30:35  lavr
 * Cosmetics
 *
 * Revision 1.2  2004/10/07 19:02:23  kuznets
 * flush at the end of line
 *
 * Revision 1.1  2004/10/05 19:05:33  kuznets
 * Initial revision
 *
 * ===========================================================================
 */
