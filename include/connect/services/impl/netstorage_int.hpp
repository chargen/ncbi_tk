#ifndef CONNECT_SERVICES_IMPL__NETSTORAGE_INT__HPP
#define CONNECT_SERVICES_IMPL__NETSTORAGE_INT__HPP

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
 * Author:  Dmitry Kazimirov
 *
 * File Description:
 *   NetStorage implementation declarations.
 *
 */

#include <connect/services/compound_id.hpp>
#include <connect/services/netservice_api.hpp>

BEGIN_NCBI_SCOPE

/// @internal
template <TNetStorageFlags MASK>
class CNetStorageFlagsSubset
{
    typedef CNetStorageFlagsSubset TSelf;
public:
    CNetStorageFlagsSubset(TNetStorageFlags flags) : m_Flags(flags & MASK) {}
    TSelf& operator &=(TSelf flag) { m_Flags &= flag; return *this; }
    TSelf& operator |=(TSelf flag) { m_Flags |= flag; return *this; }
    TSelf& operator ^=(TSelf flag) { m_Flags ^= flag; return *this; }
    operator TNetStorageFlags() const { return m_Flags; }

private:
    TNetStorageFlags m_Flags;
};

typedef CNetStorageFlagsSubset<fNST_AnyLoc> TNetStorageLocFlags;
typedef CNetStorageFlagsSubset<fNST_AnyAttr> TNetStorageAttrFlags;

/// @internal
class NCBI_XCONNECT_EXPORT CNetStorageObjectLoc
{
public:
    CNetStorageObjectLoc(CCompoundIDPool::TInstance cid_pool,
            TNetStorageAttrFlags flags,
            const string& app_domain,
            Uint8 random_number,
            const char* ft_site_name);
    CNetStorageObjectLoc(CCompoundIDPool::TInstance cid_pool,
            TNetStorageAttrFlags flags,
            const string& app_domain,
            const string& unique_key,
            const char* ft_site_name);
    CNetStorageObjectLoc(CCompoundIDPool::TInstance cid_pool,
            const string& object_loc);
    CNetStorageObjectLoc(CCompoundIDPool::TInstance cid_pool,
            const string& object_loc, TNetStorageAttrFlags flags);

    void SetObjectID(Uint8 object_id)
    {
        m_LocatorFlags &= ~(TLocatorFlags) fLF_NoMetaData;
        m_LocatorFlags |= fLF_HasObjectID;
        m_ObjectID = object_id;
        if ((m_LocatorFlags & fLF_HasUserKey) == 0)
            x_SetUniqueKeyFromRandom();
        m_Dirty = true;
    }

    Uint8 GetObjectID() const {return m_ObjectID;}

    void SetServiceName(const string& service_name)
    {
        if (service_name.empty() ||
                strchr(service_name.c_str(), ':') != NULL)
            ClearLocatorFlags(fLF_NetStorageService);
        else {
            m_ServiceName = service_name;
            SetLocatorFlags(fLF_NetStorageService);
        }
        m_Dirty = true;
    }

    string GetServiceName() const {return m_ServiceName;}

    ENetStorageObjectLocation GetLocation() const {return m_Location;}

    bool IsMovable() const {return (m_LocatorFlags & fLF_Movable) != 0;}
    bool IsCacheable() const {return (m_LocatorFlags & fLF_Cacheable) != 0;}
    bool IsMetaDataDisabled() const
    {
        return (m_LocatorFlags & fLF_NoMetaData) != 0;
    }

    Int8 GetTimestamp() const {return m_Timestamp;}
    CTime GetCreationTime() const {return CTime(m_Timestamp);}
    Uint8 GetRandom() const {return m_Random;}

    bool HasUserKey() const {return (m_LocatorFlags & fLF_HasUserKey) != 0;}
    string GetAppDomain() const {return m_AppDomain;}
    string GetUserKey() const {return m_UserKey;}

    string GetICacheKey() const {return m_ICacheKey;}
    string GetUniqueKey() const {return m_UniqueKey;}

    void SetCacheChunkSize(size_t cache_chunk_size)
    {
        m_CacheChunkSize = cache_chunk_size;
        m_Dirty = true;
    }

    Uint8 GetCacheChunkSize() const {return m_CacheChunkSize;}

    void SetLocation_NetCache(const string& service_name,
        Uint4 server_ip, unsigned short server_port,
        bool allow_xsite_conn);

    string GetNCServiceName() const {return m_NCServiceName;}
    Uint4 GetNetCacheIP() const {return m_NetCacheIP;}
    Uint2 GetNetCachePort() const {return m_NetCachePort;}

    bool IsXSiteProxyAllowed() const
    {
        return (m_NCFlags & fNCF_AllowXSiteConn) != 0;
    }

    void SetLocation_FileTrack(const char* ft_site_name);

    string GetFileTrackURL() const;

    string GetLocator() const
    {
        if (m_Dirty)
            x_Pack();
        return m_Locator;
    }

    TNetStorageAttrFlags GetStorageAttrFlags() const;

    // Serialize to a JSON object.
    void ToJSON(CJsonNode& root) const;
    CJsonNode ToJSON() const;

private:
    enum ELocatorFlags {
        fLF_NetStorageService   = (1 << 0),
        fLF_NoMetaData          = (1 << 1),
        fLF_HasObjectID         = (1 << 2),
        fLF_HasUserKey          = (1 << 3),
        fLF_Movable             = (1 << 4),
        fLF_Cacheable           = (1 << 5),
        fLF_DevEnv              = (1 << 6),
        fLF_QAEnv               = (1 << 7),

        eLF_AttrFlags = (
                fLF_NoMetaData |
                fLF_Movable |
                fLF_Cacheable),
        eLF_FieldFlags = (
                fLF_NetStorageService |
                fLF_HasObjectID |
                fLF_HasUserKey |
                fLF_DevEnv |
                fLF_QAEnv)
    };
    typedef unsigned TLocatorFlags;

    enum ENetCacheFlags {
        fNCF_AllowXSiteConn     = (1 << 0),
        fNCF_ServerSpecified    = (1 << 1),
    };
    typedef unsigned TNetCacheFlags;

    void Parse(const string& object_loc);
    void x_SetFileTrackSite(const char* ft_site_name);
    void x_SetUniqueKeyFromRandom();
    void x_SetUniqueKeyFromUserDefinedKey();

    void x_Pack() const;
    void SetLocatorFlags(TLocatorFlags flags) {m_LocatorFlags |= flags;}
    void ClearLocatorFlags(TLocatorFlags flags) {m_LocatorFlags &= ~flags;}

    TLocatorFlags x_StorageFlagsToLocatorFlags(
            TNetStorageAttrFlags storage_flags);

    mutable CCompoundIDPool m_CompoundIDPool;

    TLocatorFlags m_LocatorFlags;

    Uint8 m_ObjectID;

    string m_ServiceName;
    string m_LocationCode;
    ENetStorageObjectLocation m_Location;

    string m_AppDomain;

    Int8 m_Timestamp;
    Uint8 m_Random;

    string m_UserKey;

    // Either "m_Timestamp-m_Random[-m_ObjectID]" or m_UserKey.
    string m_ICacheKey;
    // "m_AppDomain-m_ICacheKey"
    string m_UniqueKey;

    Uint8 m_CacheChunkSize;

    TNetCacheFlags m_NCFlags;
    string m_NCServiceName;
    Uint4 m_NetCacheIP;
    Uint2 m_NetCachePort;

    mutable bool m_Dirty;

    mutable string m_Locator;
};

/// @internal
class CNetStorageServerError
{
public:
    enum EErrCode {
        eInvalidArgument                    = 1001,
        eMandatoryFieldsMissed              = 1002,
        eHelloRequired                      = 1003,
        eInvalidMessageType                 = 1004,
        eInvalidIncomingMessage             = 1005,
        ePrivileges                         = 1006,
        eInvalidMessageHeader               = 1007,
        eShuttingDown                       = 1008,
        eMessageAfterBye                    = 1009,
        eStorageError                       = 1010,
        eWriteError                         = 1011,
        eReadError                          = 1012,
        eInternalError                      = 1013,
        eNetStorageObjectNotFound           = 1014,
        eNetStorageAttributeNotFound        = 1015,
        eNetStorageAttributeValueNotFound   = 1016,
        eNetStorageClientNotFound           = 1017,
        eNetStorageObjectExpired            = 1018,
        eDatabaseError                      = 1019,
        eInvalidConfig                      = 1020,
        eRemoteObjectNotFound               = 1021,

        // Meta info involving operation requested while the service
        // is not configured for meta or HELLO metadata option conflict
        eInvalidMetaInfoRequest             = 1022,
        eUnknownError                       = 1023
    };
};

/// @internal
struct SNetStorageAdminImpl;

/// @internal
class NCBI_XCONNECT_EXPORT CNetStorageAdmin
{
    NCBI_NET_COMPONENT(NetStorageAdmin);

    CNetStorageAdmin(CNetStorage::TInstance netstorage_impl);

    CNetService GetService();

    CJsonNode MkNetStorageRequest(const string& request_type);

    CJsonNode ExchangeJson(const CJsonNode& request,
            CNetServer::TInstance server_to_use = NULL,
            CNetServerConnection* conn = NULL);
};

#ifdef NCBI_GRID_XSITE_CONN_SUPPORT
/// @internal
NCBI_XCONNECT_EXPORT
void g_AllowXSiteConnections(CNetStorage&);
#endif

END_NCBI_SCOPE

#endif  /* CONNECT_SERVICES_IMPL__NETSTORAGE_INT__HPP */