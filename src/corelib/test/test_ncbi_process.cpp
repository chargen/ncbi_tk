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
 * Authors:  Aaron Ucko, Vladimir Ivanov
 *
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbiexec.hpp>
#include <corelib/ncbifile.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbi_process.hpp>

#include <test/test_assert.h>  /* This header must go last */


USING_NCBI_SCOPE;


/////////////////////////////////
// General tests
//

static void Test_Process(void)
{
    LOG_POST("\nProcess tests:\n");

    string app = CNcbiApplication::Instance()->GetArguments().GetProgramName();
    TPid pid;

    {{
        LOG_POST("CMD = " << app << " -sleep 3");
        pid = CExec::SpawnL(CExec::eNoWait, app.c_str(), "-sleep", "3", 0);
        LOG_POST("PID = " << pid);
        CProcess process(pid, CProcess::eHandle);
        assert(process.IsAlive());
        assert(process.Wait() == 88);
        assert(!process.IsAlive());
        assert(process.Wait(2000) == -1);
        assert(process.Kill());
    }}
    {{
        LOG_POST("CMD = " << app << " -sleep 10");
        pid = CExec::SpawnL(CExec::eNoWait, app.c_str(), "-sleep", "10", 0);
        LOG_POST("PID = " << pid);
        CProcess process(pid, CProcess::eHandle);
        assert(process.IsAlive());
        assert(process.Wait(2000) == -1);
        assert(process.Kill());
        assert(!process.IsAlive());
        int exitcode = process.Wait();
        LOG_POST("Wait(pid) = " << exitcode);
        assert(exitcode != 88);
    }}
}


/////////////////////////////////
// PIDGuard test
//
// NOTE:  ppid is int rather than TPid in these two functions because we need
//        it to be signed.

static void Test_PIDGuardChild(int ppid, string lockfile)
{
    CNcbiOstrstream oss;
    oss << CNcbiApplication::Instance()->GetArguments().GetProgramName()
        << " -parent " << ppid << " -lockfile " << lockfile << '\0';
    assert( !CExec::System(oss.str()) );
    oss.freeze(false);
}

static void Test_PIDGuard(int ppid, string lockfile)
{
    if (lockfile.empty()) {
        // Fixed names are usually more appropriate, but here we don't
        // want independent tests to step on each other....
        lockfile = CFile::GetTmpName();
    }
    TPid my_pid = CProcess::GetCurrentPid();
    assert(my_pid > 0);
    LOG_POST("\nTest_PIDGuard starting:\nmy_pid = " << my_pid
             << ", ppid = " << ppid << ", lockfile = " << lockfile << '\n');

    // Parent
    if (ppid == 0) {
        CPIDGuard guard(lockfile);
        try {
            LOG_POST("Expect an exception now.");
            CPIDGuard guard2(lockfile);
            ERR_POST("Should have been locked (by myself)");
            _TROUBLE;
        } catch (CPIDGuardException& e) {
            LOG_POST(e.what());
            assert(e.GetErrCode() == CPIDGuardException::eStillRunning);
            assert(e.GetPID() == my_pid);
        }
        Test_PIDGuardChild(my_pid, lockfile);
        guard.Release();
        Test_PIDGuardChild(-1, lockfile);
        Test_PIDGuardChild(-2, lockfile);
        CFile(lockfile).Remove();
    }
    // Child run with parent lock open
    else if (ppid > 0) {
        try {
            LOG_POST("Expect an exception now.");
            CPIDGuard guard(lockfile);
            ERR_POST("Should have been locked (by parent)");
            _TROUBLE;
        } catch (CPIDGuardException& e) {
            LOG_POST(e.what());
            assert(e.GetErrCode() == CPIDGuardException::eStillRunning);
            assert(e.GetPID() == ppid);
        }
    } else if (ppid == -1) {
        new CPIDGuard(lockfile); // deliberate leak
        LOG_POST("Left stale lock.");
    } else if (ppid == -2) {
        CPIDGuard guard(lockfile);
        TPid old_pid = guard.GetOldPID();
        assert(old_pid > 0);
        LOG_POST("Old PID was " << old_pid);
    } else {
        _TROUBLE;
    }

    CPIDGuard unique_guard(CFile::GetTmpName());
}


/////////////////////////////////
// Test application
//

class CTestApplication : public CNcbiApplication
{
public:
    virtual void Init(void);
    virtual int  Run (void);
};


void CTestApplication::Init(void)
{
    // Set error posting and tracing on maximum
    SetDiagTrace(eDT_Enable);
    SetDiagPostFlag(eDPF_All);
    SetDiagPostLevel(eDiag_Info);

    // Create command-line argument descriptions class
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // Specify USAGE context
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Test process management functions");
    // Specific to Process test
    arg_desc->AddDefaultKey("sleep", "sec", "for internal use only",
                            CArgDescriptions::eInteger, "0");
    // Specific to PID guard test
    arg_desc->AddDefaultKey("parent", "PID", "for internal use only",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("lockfile", "filename", "parent's lock file",
                            CArgDescriptions::eString, kEmptyStr);

    // Setup arg.descriptions for this application
    SetupArgDescriptions(arg_desc.release());
}


int CTestApplication::Run(void)
{
    CArgs args = GetArgs();

    // Is this an instance executed from process test?
    int sec = args["sleep"].AsInteger();
    if ( sec ) {
        LOG_POST("Client is sleeping " << sec << " sec.");
        SleepSec(sec);
        return 88;
    }

    // Main tests

    // General tests
    if ( !args["parent"].AsInteger() ) {
        Test_Process();
    }
    // PIDGuard tests
    Test_PIDGuard(args["parent"].AsInteger(), args["lockfile"].AsString());

    return 0;
}


  
///////////////////////////////////
// APPLICATION OBJECT  and  MAIN
//

static CTestApplication theTestApplication;


int main(int argc, const char* argv[])
{
    // Execute main application function
    return theTestApplication.AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2004/05/14 13:59:51  gorelenk
 * Added include of ncbi_pch.hpp
 *
 * Revision 1.1  2003/09/25 16:59:11  ivanov
 * Initial revision
 *
 * ===========================================================================
 */
