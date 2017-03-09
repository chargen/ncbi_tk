#ifndef MISC_NETSTORAGE___NETSTORAGEIMPL__HPP
#define MISC_NETSTORAGE___NETSTORAGEIMPL__HPP

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
 * Author: Rafael Sadyrov
 *
 */

#include <connect/services/compound_id.hpp>
#include <connect/services/neticache_client.hpp>
#include <connect/services/netstorage.hpp>
#include "filetrack.hpp"


BEGIN_NCBI_SCOPE


struct SCombinedNetStorageConfig : SNetStorage::SConfig
{
    enum EMode {
        eDefault,
        eServerless,
    };

    EMode mode;
    SFileTrackConfig ft;

    SCombinedNetStorageConfig() : mode(eDefault) {}
    void ParseArg(const string&, const string&);

    static SCombinedNetStorageConfig Build(const string& init_string)
    {
        return BuildImpl<SCombinedNetStorageConfig>(init_string);
    }

private:
    static EMode GetMode(const string&);
};

template <class TBase>
struct SNetStorageObjectDirectState : TBase
{
    template <class... TArgs>
    SNetStorageObjectDirectState(CNetStorageObjectLoc& locator, TArgs&&... args) :
        TBase(std::forward<TArgs>(args)...),
        m_Locator(locator)
    {
    }

    string GetLoc() const override           { return m_Locator.GetLocator(); }
    CNetStorageObjectLoc& Locator() override { return m_Locator; }

private:
    CNetStorageObjectLoc& m_Locator;
};


namespace NDirectNetStorageImpl
{

typedef CNetStorageObjectLoc TObjLoc;

class ILocation
{
public:
    virtual ~ILocation() {}

    virtual INetStorageObjectState* StartRead(void*, size_t, size_t*, ERW_Result*) = 0;
    virtual INetStorageObjectState* StartWrite(const void*, size_t, size_t*, ERW_Result*) = 0;
    virtual Uint8 GetSizeImpl() = 0;
    virtual CNetStorageObjectInfo GetInfoImpl() = 0;
    virtual bool ExistsImpl() = 0;
    virtual ENetStorageRemoveResult RemoveImpl() = 0;
    virtual void SetExpirationImpl(const CTimeout&) = 0;

    virtual string FileTrack_PathImpl() = 0;

    typedef pair<string, string> TUserInfo;
    virtual TUserInfo GetUserInfoImpl() = 0;

    virtual bool IsSame(const ILocation* other) const = 0;

protected:
    template <class TLocation>
    static const TLocation* To(const ILocation* location)
    {
        return dynamic_cast<const TLocation*>(location);
    }
};

struct SContext;

class ISelector
{
public:
    typedef auto_ptr<ISelector> Ptr;

    virtual ~ISelector() {}

    virtual ILocation* First() = 0;
    virtual ILocation* Next() = 0;
    virtual bool InProgress() const = 0;
    virtual void Restart() = 0;
    virtual TObjLoc& Locator() = 0;
    virtual void SetLocator() = 0;

    virtual ISelector* Clone(SNetStorageObjectImpl&, TNetStorageFlags) = 0;
    virtual const SContext& GetContext() const = 0;
};

struct SContext : CObject
{
    CNetICacheClientExt icache_client;
    SFileTrackAPI filetrack_api;
    CCompoundIDPool compound_id_pool;
    TNetStorageFlags default_flags = 0;
    string app_domain;
    size_t relocate_chunk;

    SContext(const SCombinedNetStorageConfig&, TNetStorageFlags);
    SContext(const string&, const string&,
            CCompoundIDPool::TInstance, const IRegistry&);

    TNetStorageFlags DefaultFlags(TNetStorageFlags flags) const
    {
        return flags ? flags : default_flags;
    }

    ISelector* Create(SNetStorageObjectImpl&, bool* cancel_relocate, const string&);
    ISelector* Create(SNetStorageObjectImpl&, TNetStorageFlags, const string& = kEmptyStr);
    ISelector* Create(SNetStorageObjectImpl&, bool* cancel_relocate, const string&, TNetStorageFlags, const string& = kEmptyStr);

private:
    void Init();

    CRandom m_Random;
};

}

END_NCBI_SCOPE

#endif
