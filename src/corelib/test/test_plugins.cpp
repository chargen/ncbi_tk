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
 * File Description:
 *   Plugin manager test
 *
 */

#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/plugin_manager.hpp>

#include <test/test_assert.h>  /* This header must go last */

BEGIN_NCBI_SCOPE


struct ITest
{
    int  a;
    ITest() : a(10) {}

    virtual int GetA() const = 0;
};

struct ITest2
{
    int  b;
    ITest2() : b(0) {}
};

class CTestDriver : public ITest
{
public:
    CTestDriver() {}

    int GetA() const { return a; }
};

class CTestDriver2 : public ITest
{
public:
    CTestDriver2() {}

    int GetA() const { return 15; }
};


/*
template<> 
class CInterfaceVersion<ITest>
{ 
public: 
    enum { 
        eMajor      = 1, 
        eMinor      = 1,
        ePatchLevel = 0 
    };
    static const char* GetName() { return "itest"; } 
};
*/

NCBI_DECLARE_INTERFACE_VERSION(ITest,  "itest", 1, 1, 0);
NCBI_DECLARE_INTERFACE_VERSION(ITest2, "itest2", 1, 1, 0);


class CTestCF : public CSimpleClassFactoryImpl<ITest, CTestDriver>
{
public:
    typedef CSimpleClassFactoryImpl<ITest, CTestDriver> TParent;
public:
    CTestCF() : TParent("test_driver", 0)
    {
    }
    ~CTestCF()
    {
    }
};

class CTestCF2 : public CSimpleClassFactoryImpl<ITest, CTestDriver2>
{
public:
    typedef CSimpleClassFactoryImpl<ITest, CTestDriver2> TParent;
public:
    CTestCF2() : TParent("test_driver2", 0)
    {
    }
    ~CTestCF2()
    {
    }
};


extern "C" {

void NCBI_TestEntryPoint(CPluginManager<ITest>::TDriverInfoList&   info_list,
                         CPluginManager<ITest>::EEntryPointRequest method)
{
    CHostEntryPointImpl<CTestCF>::NCBI_EntryPointImpl(info_list, method);
}

void NCBI_Test2EntryPoint(CPluginManager<ITest>::TDriverInfoList&   info_list,
                         CPluginManager<ITest>::EEntryPointRequest method)
{
    CHostEntryPointImpl<CTestCF2>::NCBI_EntryPointImpl(info_list, method);
}

}  // extern "C"

END_NCBI_SCOPE


USING_NCBI_SCOPE;


static void s_TEST_PluginManager(void)
{
    CPluginManager<ITest> pm;

    pm.RegisterWithEntryPoint(NCBI_TestEntryPoint);

    try {
        ITest* itest = pm.CreateInstance();

        int a = itest->GetA();

        assert(a == 10);

        delete itest;


    } 
    catch (CPluginManagerException& ex)
    {
        cout << "Cannot create ITest instance. " << ex.what() << endl;
        assert(0);
    }

    // Try to find not yet registered driver
    try {
        ITest* itest = pm.CreateInstance("test_driver2");
        assert(0);
    } 
    catch (CPluginManagerException&)
    {
    }

    pm.RegisterWithEntryPoint(NCBI_Test2EntryPoint);

    try {
        ITest* itest2 = pm.CreateInstance("test_driver2");
        ITest* itest  = pm.CreateInstance("test_driver");

        int a = itest2->GetA();
        assert(a == 15);

        a = itest->GetA();
        assert(a == 10);


        delete itest2;
        delete itest;
    } 
    catch (CPluginManagerException& ex)
    {
        cout << "Cannot create ITest instance. " << ex.what() << endl;
        assert(0);
    }
}




////////////////////////////////
// Test application
//

class CTest : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};


void CTest::Init(void)
{
    SetDiagPostLevel(eDiag_Warning);
    auto_ptr<CArgDescriptions> d(new CArgDescriptions);
    d->SetUsageContext("test_dll", "DLL accessory class");
    SetupArgDescriptions(d.release());
}


int CTest::Run(void)
{
    cout << "Run test" << endl << endl;

    s_TEST_PluginManager();

    cout << endl;
    cout << "TEST execution completed successfully!" << endl << endl;
    return 0;
}


///////////////////////////////////
// APPLICATION OBJECT  and  MAIN
//

int main(int argc, const char* argv[])
{
    return CTest().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.1  2003/11/18 14:47:34  kuznets
 * Initial revision
 *
 *
 * ===========================================================================
 */
