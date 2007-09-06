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
 * Author:  Viatcheslav Gorelenkov
 *
 */

#include <ncbi_pch.hpp>
#include <app/project_tree_builder/stl_msvc_usage.hpp>
#include <app/project_tree_builder/msvc_site.hpp>
#include <app/project_tree_builder/proj_builder_app.hpp>
#include <app/project_tree_builder/msvc_prj_defines.hpp>

#include <algorithm>

#include <corelib/ncbistr.hpp>

BEGIN_NCBI_SCOPE

CMsvcSite::TDirectoryExistenceMap CMsvcSite::sm_DirExists;

//-----------------------------------------------------------------------------
CMsvcSite::CMsvcSite(const CNcbiRegistry& registry)
    :m_Registry(registry)
{
    string str;

    if (CMsvc7RegSettings::GetMsvcVersion() < CMsvc7RegSettings::eMsvcNone) {
        // MSWin
        // Provided requests
        str = m_Registry.Get("Configure", "ProvidedRequests");
        list<string> provided;
        NStr::Split(str, LIST_SEPARATOR, provided);
        ITERATE (list<string>, it, provided) {
            m_ProvidedThing.insert(*it);
        }
        if (GetApp().GetBuildType().GetType() == CBuildType::eDll) {
            m_ProvidedThing.insert("DLL");
        }

        GetStandardFeatures(provided);
        ITERATE (list<string>, it, provided) {
            m_ProvidedThing.insert(*it);
        }

        // Not provided requests
        str = m_Registry.Get("Configure", "NotProvidedRequests");
        list<string> not_provided;
        NStr::Split(str, LIST_SEPARATOR, not_provided);
        ITERATE (list<string>, it, not_provided) {
            m_NotProvidedThing.insert(*it);
        }
    } else {
        // unix
        string unix_cfg = m_Registry.Get(CMsvc7RegSettings::GetMsvcSection(),"MetaData");
        if (!unix_cfg.empty() && CFile(unix_cfg).Exists()) {
            CSimpleMakeFileContents::LoadFrom(unix_cfg,&m_UnixMakeDef);
        }
    
        CDir status_dir(GetApp().m_StatusDir);
        CDir::TEntries files = status_dir.GetEntries("*.enabled");
        ITERATE(CDir::TEntries, f, files) {
            string name = (*f)->GetBase();
            if (name[0] == '-') {
                name = name.substr(1);
                m_NotProvidedThing.insert(name);
            } else {
                m_ProvidedThing.insert(name);
            }
        }
        // special case
        if (IsProvided("BZ2") && IsProvided("LocalBZ2"))
        {
            m_NotProvidedThing.insert("BZ2");
        }
    }

    // Lib choices
    str = m_Registry.Get("Configure", "LibChoices");
    list<string> lib_choices_list;
    NStr::Split(str, LIST_SEPARATOR, lib_choices_list);
    ITERATE(list<string>, p, lib_choices_list) {
        const string& choice_str = *p;
        string lib_id;
        string lib_3party_id;
        if ( NStr::SplitInTwo(choice_str, "/", lib_id, lib_3party_id) ) {
            m_LibChoices.push_back(SLibChoice(*this, lib_id, lib_3party_id));
        } else {
           LOG_POST(Error << "Incorrect LibChoices definition: " << choice_str);
        }
    }

}


bool CMsvcSite::IsProvided(const string& thing) const
{
    if (thing.empty()) {
        return true;
    }
    if (thing[0] == '-') {
        return !IsProvided( thing.c_str() + 1);
    }
    if ( m_NotProvidedThing.find(thing) != m_NotProvidedThing.end() ) {
        return false;
    }
    if ( m_ProvidedThing.find(thing) != m_ProvidedThing.end() ) {
        return true;
    }

    bool res = 
        CMsvc7RegSettings::GetMsvcVersion() < CMsvc7RegSettings::eMsvcNone ?
            IsDescribed(thing) : false;
    if ( res) {
        list<string> components;
        GetComponents(thing, &components);
        if (components.empty()) {
            components.push_back(thing);
        }
        // in at least one configuration all components must be ok
        ITERATE(list<SConfigInfo>, config , GetApp().GetRegSettings().m_ConfigInfo) {
            res = true;
            ITERATE(list<string>, p, components) {
                const string& component = *p;
                SLibInfo lib_info;
                GetLibInfo(component, *config, &lib_info);
                res = IsLibOk(lib_info);
                if ( !res ) {
                    break;
                }
            }
            if (res) {
                break;
            }
        }
    }
    return res;
}


bool CMsvcSite::IsDescribed(const string& section) const
{
    return m_Registry.HasEntry(section);
}


void CMsvcSite::GetComponents(const string& entry, 
                              list<string>* components) const
{
    components->clear();
    NStr::Split(m_Registry.Get(entry, "Component"), " ,\t", *components);
}

string CMsvcSite::ProcessMacros(string raw_data, bool preserve_unresolved) const
{
    string data(raw_data), raw_macro, macro, definition;
    string::size_type start, end, done = 0;
    while ((start = data.find("$(", done)) != string::npos) {
        end = data.find(")", start);
        if (end == string::npos) {
            LOG_POST(Warning << "Possibly incorrect MACRO definition in: " + raw_data);
            return data;
        }
        raw_macro = data.substr(start,end-start+1);
        if (CSymResolver::IsDefine(raw_macro)) {
            macro = CSymResolver::StripDefine(raw_macro);
            definition = m_Registry.Get(CMsvc7RegSettings::GetMsvcSection(), macro);
            if (definition.empty()) {
                definition = m_Registry.Get("Configure", macro);
            }
            if (definition.empty() && preserve_unresolved) {
                // preserve unresolved macros
                done = end;
            } else {
                data = NStr::Replace(data, raw_macro, definition);
            }
        }
    }
    return data;
}

void CMsvcSite::GetLibInfo(const string& lib, 
                           const SConfigInfo& config, SLibInfo* libinfo) const
{
    libinfo->Clear();

    string include_str    = ProcessMacros(GetOpt(m_Registry, lib, "INCLUDE", config));
    NStr::Split(include_str, LIST_SEPARATOR, libinfo->m_IncludeDir);
    
    string defines_str    = GetOpt(m_Registry, lib, "DEFINES", config);
    NStr::Split(defines_str, LIST_SEPARATOR, libinfo->m_LibDefines);

    libinfo->m_LibPath    = ProcessMacros(GetOpt(m_Registry, lib, "LIBPATH", config));

    string libs_str = GetOpt(m_Registry, lib, "LIB", config);
    NStr::Split(libs_str, LIST_SEPARATOR, libinfo->m_Libs);

    libs_str = GetOpt(m_Registry, lib, "STDLIB", config);
    NStr::Split(libs_str, LIST_SEPARATOR, libinfo->m_StdLibs);

    string macro_str = GetOpt(m_Registry, lib, "MACRO", config);
    NStr::Split(macro_str, LIST_SEPARATOR, libinfo->m_Macro);

    string files_str    = ProcessMacros(GetOpt(m_Registry, lib, "FILES", config));
    NStr::Split(files_str, LIST_SEPARATOR, libinfo->m_Files);
}


bool CMsvcSite::IsLibEnabledInConfig(const string&      lib, 
                                     const SConfigInfo& config) const
{
    string enabled_configs_str = m_Registry.Get(lib, "CONFS");
    if (enabled_configs_str.empty()) {
        return true;
    }
    list<string> enabled_configs;
    NStr::Split(enabled_configs_str, 
                LIST_SEPARATOR, enabled_configs);

    return find(enabled_configs.begin(), 
                enabled_configs.end(), 
                config.m_Name) != enabled_configs.end();
}


bool CMsvcSite::ResolveDefine(const string& define, string& resolved) const
{
    if (m_UnixMakeDef.GetValue(define,resolved)) {
        return true;
    }
    resolved = m_Registry.Get("Defines", define);
    if (resolved.empty()) {
        return m_Registry.HasEntry("Defines");
    }
    resolved = ProcessMacros(resolved);
    return true;
}


string CMsvcSite::GetConfigureDefinesPath(void) const
{
    return m_Registry.Get("Configure", "DefinesPath");
}


void CMsvcSite::GetConfigureDefines(list<string>* defines) const
{
    defines->clear();
    NStr::Split(m_Registry.Get("Configure", "Defines"), LIST_SEPARATOR,
                *defines);
}


bool CMsvcSite::IsLibWithChoice(const string& lib_id) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {
        const SLibChoice& choice = *p;
        if (lib_id == choice.m_LibId)
            return true;
    }
    return false;
}


bool CMsvcSite::Is3PartyLibWithChoice(const string& lib3party_id) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {
        const SLibChoice& choice = *p;
        if (lib3party_id == choice.m_3PartyLib)
            return true;
    }
    return false;
}


CMsvcSite::SLibChoice::SLibChoice(void)
 :m_Choice(eUnknown)
{
}


CMsvcSite::SLibChoice::SLibChoice(const CMsvcSite& site,
                                  const string&    lib,
                                  const string&    lib_3party)
 :m_LibId    (lib),
  m_3PartyLib(lib_3party)
{
    if (CMsvc7RegSettings::GetMsvcVersion() < CMsvc7RegSettings::eMsvcNone) {
        m_Choice = e3PartyLib;
        ITERATE(list<SConfigInfo>, p, GetApp().GetRegSettings().m_ConfigInfo) {
            const SConfigInfo& config = *p;
            SLibInfo lib_info;
            site.GetLibInfo(m_3PartyLib, config, &lib_info);

            if ( !CMsvcSite::IsLibOk(lib_info) ) {

                m_Choice = eLib;
                break;
            }
        }
    } else {
        m_Choice = site.IsProvided(lib_3party) ? e3PartyLib : eLib;
    }
}


CMsvcSite::ELibChoice CMsvcSite::GetChoiceForLib(const string& lib_id) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {

        const SLibChoice& choice = *p;
        if (choice.m_LibId == lib_id) 
            return choice.m_Choice;
    }
    return eUnknown;
}

CMsvcSite::ELibChoice CMsvcSite::GetChoiceFor3PartyLib(
    const string& lib3party_id, const SConfigInfo& cfg_info) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {
        const SLibChoice& choice = *p;
        if (choice.m_3PartyLib == lib3party_id) {
            if (GetApp().GetBuildType().GetType() == CBuildType::eDll) {
                return choice.m_Choice;
            } else {
                SLibInfo lib_info;
                GetLibInfo(lib3party_id, cfg_info, &lib_info);
                return IsLibOk(lib_info,true) ? e3PartyLib : eLib;
            }
        }
    }
    return eUnknown;
}


void CMsvcSite::GetLibChoiceIncludes(
    const string& cpp_flags_define, list<string>* abs_includes) const
{
    abs_includes->clear();

    const string& include_str = m_Registry.Get("LibChoicesIncludes", 
                                               cpp_flags_define);
    if (!include_str.empty()) {
        abs_includes->push_back("$(" + cpp_flags_define + ")");
    }
}

void CMsvcSite::GetLibChoiceIncludes(
    const string& cpp_flags_define, const SConfigInfo& cfg_info,
    list<string>* abs_includes) const
{
    abs_includes->clear();
    const string& include_str = m_Registry.Get("LibChoicesIncludes", 
                                               cpp_flags_define);
    //split on parts
    list<string> parts;
    NStr::Split(include_str, LIST_SEPARATOR, parts);

    string lib_id;
    ITERATE(list<string>, p, parts) {
        if ( lib_id.empty() )
            lib_id = *p;
        else  {
            SLibChoice choice = GetLibChoiceForLib(lib_id);
            SLibInfo lib_info;
            GetLibInfo(choice.m_3PartyLib, cfg_info, &lib_info);
            bool b3;
            if (GetApp().GetBuildType().GetType() == CBuildType::eDll/* &&
                GetApp().GetDllsInfo().IsDllHosted(lib_id)*/) {
                b3 = choice.m_Choice == e3PartyLib;
            } else {
                b3 = IsLibOk(lib_info, true);
            }
            if (b3) {
                copy(lib_info.m_IncludeDir.begin(), 
                    lib_info.m_IncludeDir.end(), back_inserter(*abs_includes));
            } else {
                const string& rel_include_path = *p;
                if (*p != ".") {
                    string abs_include_path = 
                        GetApp().GetProjectTreeInfo().m_Include;
                    abs_include_path = 
                        CDirEntry::ConcatPath(abs_include_path, rel_include_path);
                    abs_include_path = CDirEntry::NormalizePath(abs_include_path);
                    abs_includes->push_back(abs_include_path);
                }
            }
            lib_id.erase();
        }
    }
}

void CMsvcSite::GetLibInclude(const string& lib_id,
    const SConfigInfo& cfg_info, list<string>* includes) const
{
    includes->clear();
    if (CSymResolver::IsDefine(lib_id)) {
        GetLibChoiceIncludes( CSymResolver::StripDefine(lib_id), cfg_info, includes);
        return;
    }
    SLibInfo lib_info;
    GetLibInfo(lib_id, cfg_info, &lib_info);
    if ( IsLibOk(lib_info, true) ) {
//        includes->push_back(lib_info.m_IncludeDir);
        copy(lib_info.m_IncludeDir.begin(),
             lib_info.m_IncludeDir.end(), back_inserter(*includes));
        return;
    } else {
        if (!lib_info.IsEmpty()) {
            LOG_POST(Warning << lib_id << "|" << cfg_info.GetConfigFullName()
                          << " unavailable: library include ignored: "
                          << NStr::Join(lib_info.m_IncludeDir,","));
        }
    }
}

CMsvcSite::SLibChoice CMsvcSite::GetLibChoiceForLib(const string& lib_id) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {

        const SLibChoice& choice = *p;
        if (choice.m_LibId == lib_id) 
            return choice;
    }
    return SLibChoice();

}

CMsvcSite::SLibChoice CMsvcSite::GetLibChoiceFor3PartyLib(const string& lib3party_id) const
{
    ITERATE(list<SLibChoice>, p, m_LibChoices) {
        const SLibChoice& choice = *p;
        if (choice.m_3PartyLib == lib3party_id)
            return choice;
    }
    return SLibChoice();
}


string CMsvcSite::GetAppDefaultResource(void) const
{
    return m_Registry.Get("DefaultResource", "app");
}


void CMsvcSite::GetThirdPartyLibsToInstall(list<string>* libs) const
{
    libs->clear();

    string libs_str = m_Registry.Get("Configure", "ThirdPartyLibsToInstall");
    NStr::Split(libs_str, LIST_SEPARATOR, *libs);
}


string CMsvcSite::GetThirdPartyLibsBinPathSuffix(void) const
{
    return m_Registry.Get("Configure", "ThirdPartyLibsBinPathSuffix");
}

string CMsvcSite::GetThirdPartyLibsBinSubDir(void) const
{
    return m_Registry.Get("Configure", "ThirdPartyLibsBinSubDir");
}

void CMsvcSite::GetStandardFeatures(list<string>& features) const
{
    features.clear();
    NStr::Split(m_Registry.Get("Configure", "StandardFeatures"),
                LIST_SEPARATOR, features);
}

//-----------------------------------------------------------------------------
bool CMsvcSite::x_DirExists(const string& dir_name)
{
    TDirectoryExistenceMap::iterator it = sm_DirExists.find(dir_name);
    if (it == sm_DirExists.end()) {
        bool exists = CDirEntry(dir_name).Exists();
        it = sm_DirExists.insert
            (TDirectoryExistenceMap::value_type(dir_name, exists)).first;
    }
    return it->second;
}

bool CMsvcSite::IsLibOk(const SLibInfo& lib_info, bool silent)
{
    if ( lib_info.IsEmpty() )
        return false;
    if ( !lib_info.m_IncludeDir.empty() ) {
        ITERATE(list<string>, i, lib_info.m_IncludeDir) {
            if (!x_DirExists(*i) ) {
                if (!silent) {
                    LOG_POST(Warning << "No LIB INCLUDE: " + *i);
                }
                return false;
            }
        }
    }
    if ( !lib_info.m_LibPath.empty() &&
         !x_DirExists(lib_info.m_LibPath) ) {
        if (!silent) {
            LOG_POST(Warning << "No LIBPATH: " + lib_info.m_LibPath);
        }
        return false;
    }
    if ( !lib_info.m_LibPath.empty()) {
        ITERATE(list<string>, p, lib_info.m_Libs) {
            const string& lib = *p;
            string lib_path_abs = CDirEntry::ConcatPath(lib_info.m_LibPath, lib);
            if ( !lib_path_abs.empty() &&
                 !x_DirExists(lib_path_abs) ) {
                if (!silent) {
                    LOG_POST(Warning << "No LIB: " + lib_path_abs);
                }
                return false;
            }
        }
    }
    if ( !lib_info.m_Files.empty()) {
        ITERATE(list<string>, p, lib_info.m_Files) {
            string file = *p;
            if (!CDirEntry::IsAbsolutePath(file)) {
                file = CDirEntry::ConcatPath(GetApp().GetProjectTreeInfo().m_Root, file);
            }
            if ( !x_DirExists(file) ) {
                if (!silent) {
                    LOG_POST(Warning << "No FILES: " + file);
                }
                return false;
            }
        }
    }

    return true;
}

void CMsvcSite::ProcessMacros(const list<SConfigInfo>& configs)
{
    list<string> macros;
    NStr::Split(m_Registry.Get("Configure", "Macros"), LIST_SEPARATOR, macros);

    ITERATE(list<string>, m, macros) {
        const string& macro = *m;
        if (!IsDescribed(macro)) {
            // add empty value
            LOG_POST(Error << "Macro " << macro << " is not described");
        }
        list<string> components;
        GetComponents(macro, &components);
        bool res = false;
        ITERATE(list<string>, p, components) {
            const string& component = *p;
            if (CMsvc7RegSettings::GetMsvcVersion() < CMsvc7RegSettings::eMsvcNone) {
                ITERATE(list<SConfigInfo>, n, configs) {
                    const SConfigInfo& config = *n;
                    SLibInfo lib_info;
                    GetLibInfo(component, config, &lib_info);
                    if ( IsLibOk(lib_info) ) {
                        res = true;
                    } else {
                        if (!lib_info.IsEmpty()) {
                            LOG_POST(Warning << "Macro " << macro
                                << " cannot be resolved for "
                                << component << "|" << config.GetConfigFullName());
                        }
//                      res = false;
//                      break;
                    }
                }
            } else {
                res = IsProvided(component);
                if (!res) {
                    break;
                }
            }
        }
        if (res) {
            m_Macros.AddDefinition(macro, m_Registry.Get(macro, "Value"));
        } else {
            m_Macros.AddDefinition(macro, m_Registry.Get(macro, "DefValue"));
        }
    }
}


END_NCBI_SCOPE
