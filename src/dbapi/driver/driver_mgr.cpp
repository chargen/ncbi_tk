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
 * File Description:  Driver manager
 *
 */

#include <dbapi/driver/driver_mgr.hpp>
#include <corelib/ncbidll.hpp>

BEGIN_NCBI_SCOPE

class C_xDriverMgr : public I_DriverMgr
{
public:
    C_xDriverMgr(unsigned int nof_drivers= 16) {
	m_NofRoom= nof_drivers? nof_drivers : 16;
	m_Drivers= new SDrivers[m_NofRoom];
	m_NofDrvs= 0;
    }

    FDBAPI_CreateContext GetDriver(const string& driver_name, 
				   string* err_msg= 0);

    virtual void RegisterDriver(const string&        driver_name,
                                FDBAPI_CreateContext driver_ctx_func);

    virtual ~C_xDriverMgr() {
	delete [] m_Drivers;
    }

protected:
    bool LoadDriverDll(const string& driver_name, string* err_msg);

private:
    typedef void            (*FDriverRegister) (I_DriverMgr& mgr);
    typedef FDriverRegister (*FDllEntryPoint)  (void);

    struct SDrivers {
	string               drv_name;
	FDBAPI_CreateContext drv_func;
    } *m_Drivers;

    unsigned int m_NofDrvs;
    unsigned int m_NofRoom;
    CFastMutex m_Mutex1;
    CFastMutex m_Mutex2;
};

void C_xDriverMgr::RegisterDriver(const string&        driver_name,
                                 FDBAPI_CreateContext driver_ctx_func)
{
    if(m_NofDrvs < m_NofRoom) {
        CFastMutexGuard mg(m_Mutex2);
        for(unsigned int i= m_NofDrvs; i--; ) {
            if(m_Drivers[i].drv_name == driver_name) {
                m_Drivers[i].drv_func= driver_ctx_func;
                return;
            }
        }
        m_Drivers[m_NofDrvs++].drv_func= driver_ctx_func;
        m_Drivers[m_NofDrvs-1].drv_name= driver_name;
    }
    else {
        throw CDB_ClientEx(eDB_Error, 101, "C_xDriverMgr::RegisterDriver",
                           "No space left for driver registration");
    }
	
}


FDBAPI_CreateContext C_xDriverMgr::GetDriver(const string& driver_name,
					    string* err_msg)
{
    CFastMutexGuard mg(m_Mutex1);
    unsigned int i;
    
    for(i= m_NofDrvs; i--; ) {
        if(m_Drivers[i].drv_name == driver_name) {
            return m_Drivers[i].drv_func;
        }
    }
    
    if (!LoadDriverDll(driver_name, err_msg)) {
        return 0;
    }

    for(i= m_NofDrvs; i--; ) {
        if(m_Drivers[i].drv_name == driver_name) {
            return m_Drivers[i].drv_func;
        }
    }

    throw CDB_ClientEx(eDB_Error, 200, "C_DriverMgr::GetDriver",
                       "internal error");
}


bool C_xDriverMgr::LoadDriverDll(const string& driver_name, string* err_msg)
{
    try {
        CDll drv_dll("dbapi_driver_" + driver_name);

        FDllEntryPoint entry_point;
        if ( !drv_dll.GetEntryPoint("DBAPI_E_" + driver_name, &entry_point) ) {
            drv_dll.Unload();
            return false;
        }
        FDriverRegister reg = entry_point();
        reg(*this);
        return true;
    }
    catch (exception& e) {
	if(err_msg) *err_msg= e.what();
        return false;
    }
}


static C_xDriverMgr* xDrvMgr= 0;
static int xCount= 0;
static CFastMutex xMutex;

C_DriverMgr::C_DriverMgr(unsigned int nof_drivers)
{
    CFastMutexGuard mg(xMutex); // lock the mutex
    if(!xDrvMgr) { // There is no driver manager yet
	xDrvMgr= new C_xDriverMgr(nof_drivers);
    }
    ++xCount;
}
    
C_DriverMgr::~C_DriverMgr()
{
    CFastMutexGuard mg(xMutex); // lock the mutex
    if(--xCount <= 0) { // this is a last one
	delete xDrvMgr;
	xDrvMgr= 0;
	xCount= 0;
    }
}


FDBAPI_CreateContext C_DriverMgr::GetDriver(const string& driver_name,
					    string* err_msg)
{
    return xDrvMgr->GetDriver(driver_name, err_msg);
}

void C_DriverMgr::RegisterDriver(const string&        driver_name,
				 FDBAPI_CreateContext driver_ctx_func)
{
    xDrvMgr->RegisterDriver(driver_name, driver_ctx_func);
}

END_NCBI_SCOPE



/*
 * ===========================================================================
 * $Log$
 * Revision 1.10  2002/04/12 18:48:30  soussov
 * makes driver_mgr working properly in mt environment
 *
 * Revision 1.9  2002/04/09 22:18:15  vakatov
 * Moved code from the header
 *
 * Revision 1.8  2002/04/04 23:59:37  soussov
 * bug in RegisterDriver fixed
 *
 * Revision 1.7  2002/04/04 23:57:39  soussov
 * return of error message from dlopen added
 *
 * Revision 1.6  2002/02/14 00:59:42  vakatov
 * Clean-up: warning elimination, fool-proofing, fine-tuning, identation, etc.
 *
 * Revision 1.5  2002/01/23 21:29:48  soussov
 * replaces map with array
 *
 * Revision 1.4  2002/01/20 07:26:58  vakatov
 * Fool-proofed to compile func_ptr/void_ptr type casts on all compilers.
 * Added virtual destructor.
 * Temporarily get rid of the MT-safety features -- they need to be
 * implemented here more carefully (in the nearest future).
 *
 * Revision 1.3  2002/01/20 05:24:42  vakatov
 * identation
 *
 * Revision 1.2  2002/01/17 23:19:13  soussov
 * makes gcc happy
 *
 * Revision 1.1  2002/01/17 22:17:25  soussov
 * adds driver manager
 *
 * ===========================================================================
 */
