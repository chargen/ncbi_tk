#ifndef NETCACHED__HPP
#define NETCACHED__HPP

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
 * Authors:  Anatoliy Kuznetsov, Victor Joukov
 *
 * File Description: Network cache daemon
 *
 */

#include <connect/services/netcache_api_expt.hpp>

#include <connect/server.hpp>
#include <connect/server_monitor.hpp>

#include <corelib/ncbimtx.hpp>
#include <corelib/ncbi_config.hpp>

#include "smng_thread.hpp"
#include "nc_storage.hpp"
#include "nc_utils.hpp"


BEGIN_NCBI_SCOPE


class CNetCacheDApp;
class CNCMessageHandler;


/// Netcache server 
class CNetCacheServer : public CServer
{
public:
    /// Create server
    ///
    /// @param do_reinit
    ///   TRUE if reinitialization should be forced (was requested in command
    ///   line)
    CNetCacheServer(bool do_reinit);
    virtual ~CNetCacheServer();

    /// Check if server was asked to stop
    virtual bool ShutdownRequested(void);
    /// Get signal number by which server was asked to stop
    int GetSignalCode(void) const;
    /// Ask server to stop after receiving given signal
    void RequestShutdown(int signal = 0);
    
    // Check if session management turned on
    bool IsManagingSessions(void) const;
    /// Register new session
    ///
    /// @return
    ///   TRUE if session has been registered,
    ///   FALSE if session management cannot do it (shutdown)
    bool RegisterSession  (const string& host, unsigned int pid);
    /// Unregister session
    void UnregisterSession(const string& host, unsigned int pid);

    /// Get monitor for the server
    CServer_Monitor* GetServerMonitor(void);
    /// Get storage for the given cache name.
    /// If cache name is empty then it should be main storage for NetCache,
    /// otherwise it's storage for ICache implementation
    CNCBlobStorage* GetBlobStorage(const string& cache_name);
    /// Get next blob id to incorporate it into generated blob key
    int GetNextBlobId(void);
    /// Get configuration registry for the server
    const IRegistry& GetRegistry(void);
    /// Get server host
    const string& GetHost(void) const;
    /// Get server port
    unsigned int GetPort(void) const;
    /// Get inactivity timeout for each connection
    unsigned GetInactivityTimeout(void) const;
    /// Get timeout for each executed command
    unsigned GetCmdTimeout(void) const;
    /// Create CTime object in fast way (via CFastTime)
    CTime GetFastTime(void);

    // Statistics methods

    /// Add finished command to statistics
    ///
    /// @param cmd_span
    ///   Time command was executed
    /// @param state_spans
    ///   Array of times spent in each handler state during command execution
    void AddFinishedCmd(double cmd_span, const vector<double>& state_spans);
    /// Add closed connection to statistics
    ///
    /// @param conn_span
    ///   Time which connection stayed opened
    void AddClosedConnection(double conn_span);
    /// Add opened connection to statistics
    void AddOpenedConnection(void);
    /// Remove opened connection from statistics (after OnOverflow was called)
    void RemoveOpenedConnection(void);
    /// Add to statistics connection that will be closed because of maximum
    /// number of connections exceeded.
    void AddOverflowConnection(void);
    /// Add to statistics command terminated because of timeout
    void AddTimedOutCommand(void);

    /// Print full server statistics into stream
    void PrintServerStats(CNcbiIostream* ios);

private:
    /// Read integer configuration value from server's registry
    int    x_RegReadInt   (const IRegistry& reg,
                           const char*      value_name,
                           int              def_value);
    /// Read boolean configuration value from server's registry
    bool   x_RegReadBool  (const IRegistry& reg,
                           const char*      value_name,
                           bool             def_value);
    /// Read string configuration value from server's registry
    string x_RegReadString(const IRegistry& reg,
                           const char*      value_name,
                           const string&    def_value);

    /// Create all blob storage instances
    ///
    /// @param reg
    ///   Registry to read configuration for storages
    /// @param do_reinit
    ///   Flag if all storages should be forced to reinitialize
    void x_CreateStorages(const IRegistry& reg, bool do_reinit);

    /// Start session management thread
    void x_StartSessionManagement(unsigned int shutdown_timeout);
    /// Stop session management thread
    void x_StopSessionManagement (void);

    /// Print full server statistics into stream or diagnostics
    void x_PrintServerStats(CPrintTextProxy& proxy);


    typedef map<string, AutoPtr<CNCBlobStorage> >   TStorageMap;


    /// Host name where server runs
    string                         m_Host;
    /// Port where server runs
    unsigned                       m_Port;
    // Some variable that should be here because of CServer requirements
    STimeout                       m_ServerAcceptTimeout;
    /// Flag that server received a shutdown request
    bool                           m_Shutdown;
    /// Signal which caused the shutdown request
    int                            m_Signal;
    /// Time to wait for the client on the connection (seconds)
    unsigned                       m_InactivityTimeout;
    /// Maximum time span which each command can work in
    unsigned                       m_CmdTimeout;
    /// Quick local timer
    CFastLocalTime                 m_FastTime;
    /// Map of strings to blob storages
    TStorageMap                    m_StorageMap;
    /// Counter for blob id
    CAtomicCounter                 m_BlobIdCounter;
    /// Session management thread
    CRef<CSessionManagementThread> m_SessionMngThread;
    /// Server monitor
    CServer_Monitor                m_Monitor;

    /// Mutex for working with statistics
    CFastMutex                     m_StatsMutex;
    /// Maximum time connection was opened
    double                         m_MaxConnSpan;
    /// Number of connections closed
    Uint8                          m_ClosedConns;
    /// Number of connections opened
    Uint8                          m_OpenedConns;
    /// Number of connections closed because of maximum number of opened
    /// connections limit.
    Uint8                          m_OverflowConns;
    /// Sum of times all connections stayed opened
    double                         m_ConnsSpansSum;
    /// Maximum time one command was executed
    double                         m_MaxCmdSpan;
    /// Total number of executed commands
    Uint8                          m_CntCmds;
    /// Sum of times all commands were executed
    double                         m_CmdsSpansSum;
    /// Sums of times handlers spent in every state
    vector<double>                 m_StatesSpansSums;
    /// Number of commands terminated because of command timeout
    Uint8                          m_TimedOutCmds;
};


/// NetCache daemon application
class CNetCacheDApp : public CNcbiApplication
{
protected:
    virtual void Init(void);
    virtual int  Run (void);
};



inline bool
CNetCacheServer::ShutdownRequested(void)
{
    return m_Shutdown;
}

inline int
CNetCacheServer::GetSignalCode(void) const
{
    return m_Signal;
}

inline int
CNetCacheServer::GetNextBlobId(void)
{
    return int(m_BlobIdCounter.Add(1));
}

inline unsigned int
CNetCacheServer::GetCmdTimeout(void) const
{
    return m_CmdTimeout;
}

inline void
CNetCacheServer::RequestShutdown(int sig)
{
    if (!m_Shutdown) {
        m_Shutdown = true;
        m_Signal = sig;
    }
}

inline bool
CNetCacheServer::IsManagingSessions(void) const
{
    return !m_SessionMngThread.Empty();
}

inline bool
CNetCacheServer::RegisterSession(const string& host, unsigned int pid)
{
    if (m_SessionMngThread.Empty())
        return false;

    return m_SessionMngThread->RegisterSession(host, pid);
}

inline void
CNetCacheServer::UnregisterSession(const string& host, unsigned int pid)
{
    if (m_SessionMngThread.Empty())
        return;

    m_SessionMngThread->UnregisterSession(host, pid);
}

inline CServer_Monitor*
CNetCacheServer::GetServerMonitor(void)
{
    return &m_Monitor;
}

inline const IRegistry&
CNetCacheServer::GetRegistry(void)
{
    return CNcbiApplication::Instance()->GetConfig();
}

inline const string&
CNetCacheServer::GetHost(void) const
{
    return m_Host;
}

inline unsigned int
CNetCacheServer::GetPort(void) const
{
    return m_Port;
}

inline unsigned int
CNetCacheServer::GetInactivityTimeout(void) const
{
    return m_InactivityTimeout;
}

inline CTime
CNetCacheServer::GetFastTime(void)
{
    return m_FastTime.GetLocalTime();
}

inline CNCBlobStorage*
CNetCacheServer::GetBlobStorage(const string& cache_name)
{
    TStorageMap::iterator it = m_StorageMap.find(cache_name);
    if (it == m_StorageMap.end()) {
        return NULL;
    }
    return it->second.get();
}

inline void
CNetCacheServer::AddClosedConnection(double conn_span)
{
    CFastMutexGuard guard(m_StatsMutex);

    ++m_ClosedConns;
    m_ConnsSpansSum += conn_span;
    m_MaxConnSpan = max(m_MaxConnSpan, conn_span);
}

inline void
CNetCacheServer::AddOpenedConnection(void)
{
    CFastMutexGuard guard(m_StatsMutex);
    ++m_OpenedConns;
}

inline void
CNetCacheServer::RemoveOpenedConnection(void)
{
    CFastMutexGuard guard(m_StatsMutex);
    --m_OpenedConns;
}

inline void
CNetCacheServer::AddOverflowConnection(void)
{
    CFastMutexGuard guard(m_StatsMutex);
    ++m_OverflowConns;
}

inline void
CNetCacheServer::AddTimedOutCommand(void)
{
    CFastMutexGuard guard(m_StatsMutex);
    ++m_TimedOutCmds;
}

inline void
CNetCacheServer::PrintServerStats(CNcbiIostream* ios)
{
    CFastMutexGuard guard(m_StatsMutex);
    CPrintTextProxy proxy(CPrintTextProxy::ePrintStream, ios);

    x_PrintServerStats(proxy);
}

END_NCBI_SCOPE

#endif /* NETCACHED__HPP */
