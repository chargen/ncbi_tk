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
#include <app/project_tree_builder/msvc_configure.hpp>
#include <app/project_tree_builder/proj_builder_app.hpp>

BEGIN_NCBI_SCOPE


CMsvcConfigure::CMsvcConfigure(void)
{
}


CMsvcConfigure::~CMsvcConfigure(void)
{
}


void s_ResetLibInstallKey(const string& dir, 
                          const string& lib)
{
    string key_file_name(lib);
    NStr::ToLower(key_file_name);
    key_file_name += ".installed";
    string key_file_path = CDirEntry::ConcatPath(dir, key_file_name);
    if ( CDirEntry(key_file_path).Exists() ) {
        CDirEntry(key_file_path).Remove();
    }
}


static void s_CreateThirdPartyLibsInstallMakefile
                                            (const CMsvcSite&   site, 
                                             const list<string> libs_to_install,
                                             const SConfigInfo& config,
                                             const CBuildType&  build_type)
{
    // Create makefile path
    string makefile_path = GetApp().GetProjectTreeInfo().m_Compilers;
    makefile_path = 
        CDirEntry::ConcatPath(makefile_path, 
                              GetApp().GetRegSettings().m_CompilersSubdir);

    makefile_path = CDirEntry::ConcatPath(makefile_path, build_type.GetTypeStr());
    makefile_path = CDirEntry::ConcatPath(makefile_path, config.m_Name);
    makefile_path = CDirEntry::ConcatPath(makefile_path, 
                                          "Makefile.third_party.mk");

    // Create dir if no such dir...
    string dir;
    CDirEntry::SplitPath(makefile_path, &dir);
    CDir makefile_dir(dir);
    if ( !makefile_dir.Exists() ) {
        CDir(dir).CreatePath();
    }

    CNcbiOfstream ofs(makefile_path.c_str(), 
                      IOS_BASE::out | IOS_BASE::trunc );
    if ( !ofs )
        NCBI_THROW(CProjBulderAppException, eFileCreation, makefile_path);

    
    ITERATE(list<string>, n, libs_to_install) {
        const string& lib = *n;
        SLibInfo lib_info;
        site.GetLibInfo(lib, config, &lib_info);
        if ( !lib_info.m_LibPath.empty() ) {
            string bin_dir = lib_info.m_LibPath;
            bin_dir = 
                CDirEntry::ConcatPath(bin_dir, 
                                      site.GetThirdPartyLibsBinSubDir());
            bin_dir = CDirEntry::NormalizePath(bin_dir);
            if ( CDirEntry(bin_dir).Exists() ) {
                //
                string key(lib);
                NStr::ToUpper(key);
                key += site.GetThirdPartyLibsBinPathSuffix();

                ofs << key << " = " << bin_dir << "\n";

                s_ResetLibInstallKey(dir, lib);
            }
        }
    }
}


void CMsvcConfigure::Configure(CMsvcSite&         site, 
                               const list<SConfigInfo>& configs,
                               const string&            root_dir)
{
    LOG_POST(Info << "Starting configure.");
    
    InitializeFrom(site);
    site.ProcessMacros(configs);
    
    const CBuildType static_build(false);
    const CBuildType dll_build(true);
    ITERATE(list<SConfigInfo>, p, configs) {
        AnalyzeDefines( site, root_dir, *p, static_build);
    }
    list<SConfigInfo> dlls;
    GetApp().GetDllsInfo().GetBuildConfigs(&dlls);
    ITERATE(list<SConfigInfo>, p, dlls) {
        AnalyzeDefines( site, root_dir, *p, dll_build);
    }
    LOG_POST(Info << "Configure finished.");

    // Write makefile uses to install 3-rd party dlls
    list<string> third_party_to_install;
    site.GetThirdPartyLibsToInstall(&third_party_to_install);
    // For static buid
    ITERATE(list<SConfigInfo>, p, configs) {
        const SConfigInfo& config = *p;
        s_CreateThirdPartyLibsInstallMakefile(site, 
                                              third_party_to_install, 
                                              config, 
                                              static_build);
    }
    // For dll build
    list<SConfigInfo> dll_configs;
    GetApp().GetDllsInfo().GetBuildConfigs(&dll_configs);
    ITERATE(list<SConfigInfo>, p, dll_configs) {
        const SConfigInfo& config = *p;
        s_CreateThirdPartyLibsInstallMakefile(site, 
                                              third_party_to_install, 
                                              config, 
                                              dll_build);
    }
}


void CMsvcConfigure::InitializeFrom(const CMsvcSite& site)
{
    m_ConfigSite.clear();

    m_ConfigureDefinesPath = site.GetConfigureDefinesPath();
    if ( m_ConfigureDefinesPath.empty() )
        NCBI_THROW(CProjBulderAppException, 
           eConfigureDefinesPath, ""); //TODO - message
    else
        LOG_POST(Info  << "Path to configure defines file is: " +
                           m_ConfigureDefinesPath);

    site.GetConfigureDefines(&m_ConfigureDefines);
    if( m_ConfigureDefines.empty() )
        LOG_POST(Warning << "No configure defines.");
    else
        LOG_POST(Info    << "Configure defines: " + 
                            NStr::Join(m_ConfigureDefines, ", ") );
}


bool CMsvcConfigure::ProcessDefine(const string& define, 
                                   const CMsvcSite& site, 
                                   const SConfigInfo& config) const
{
    if ( !site.IsDescribed(define) ) {
        LOG_POST(Info << "Not defined in site: " + define);
        return false;
    }
    list<string> components;
    site.GetComponents(define, &components);
    ITERATE(list<string>, p, components) {
        const string& component = *p;
        SLibInfo lib_info;
        site.GetLibInfo(component, config, &lib_info);
        if ( !site.IsLibOk(lib_info) )
            return false;
    }
    return true;
}

void CMsvcConfigure::AnalyzeDefines(
    CMsvcSite& site, const string& root_dir,
    const SConfigInfo& config, const CBuildType&  build_type)
{
    m_ConfigSite.clear();

    ITERATE(list<string>, p, m_ConfigureDefines) {
        const string& define = *p;
        if( ProcessDefine(define, site, config) ) {
            LOG_POST(Info << config.m_Name + ": " + define + " enabled");
            m_ConfigSite[define] = '1';
        } else {
            LOG_POST(Info << config.m_Name + ": " + define + " disabled");
            m_ConfigSite[define] = '0';
        }
    }
    string ncbi_conf_msvc_site_path = 
        CDirEntry::ConcatPath(root_dir, m_ConfigureDefinesPath);
    string dir, base, ext;
    CDirEntry::SplitPath(ncbi_conf_msvc_site_path, &dir, &base, &ext);
    string filename = CDirEntry::ConcatPath( dir, base) + "." + config.m_Name + ext;
    LOG_POST(Info << "Creating MSVC site configuration in: "  + filename);
    string candidate_path = filename + ".candidate";

    CDirEntry::SplitPath(filename, &dir);
    CDir(dir).CreatePath();
    WriteNcbiconfMsvcSite(candidate_path);
    PromoteIfDifferent(filename, candidate_path);
}

void CMsvcConfigure::WriteNcbiconfMsvcSite(const string& full_path) const
{
    CNcbiOfstream  ofs(full_path.c_str(), 
                       IOS_BASE::out | IOS_BASE::trunc );
    if ( !ofs )
	    NCBI_THROW(CProjBulderAppException, eFileCreation, full_path);

    ofs << endl;

    ofs <<"/* $" << "Id" << "$" << endl;
    ofs <<"* ===========================================================================" << endl;
    ofs <<"*" << endl;
    ofs <<"*                            PUBLIC DOMAIN NOTICE" << endl;
    ofs <<"*               National Center for Biotechnology Information" << endl;
    ofs <<"*" << endl;
    ofs <<"*  This software/database is a \"United States Government Work\" under the" << endl;
    ofs <<"*  terms of the United States Copyright Act.  It was written as part of" << endl;
    ofs <<"*  the author's official duties as a United States Government employee and" << endl;
    ofs <<"*  thus cannot be copyrighted.  This software/database is freely available" << endl;
    ofs <<"*  to the public for use. The National Library of Medicine and the U.S." << endl;
    ofs <<"*  Government have not placed any restriction on its use or reproduction." << endl;
    ofs <<"*" << endl;
    ofs <<"*  Although all reasonable efforts have been taken to ensure the accuracy" << endl;
    ofs <<"*  and reliability of the software and data, the NLM and the U.S." << endl;
    ofs <<"*  Government do not and cannot warrant the performance or results that" << endl;
    ofs <<"*  may be obtained by using this software or data. The NLM and the U.S." << endl;
    ofs <<"*  Government disclaim all warranties, express or implied, including" << endl;
    ofs <<"*  warranties of performance, merchantability or fitness for any particular" << endl;
    ofs <<"*  purpose." << endl;
    ofs <<"*" << endl;
    ofs <<"*  Please cite the author in any work or product based on this material." << endl;
    ofs <<"*" << endl;
    ofs <<"* ===========================================================================" << endl;
    ofs <<"*" << endl;
    ofs <<"* Author:  ......." << endl;
    ofs <<"*" << endl;
    ofs <<"* File Description:" << endl;
    ofs <<"*   ......." << endl;
    ofs <<"*" << endl;
    ofs <<"* ATTENTION:" << endl;
    ofs <<"*   Do not edit or commit this file into CVS as this file will" << endl;
    ofs <<"*   be overwritten (by PROJECT_TREE_BUILDER) without warning!" << endl;
    ofs <<"*/" << endl;
    ofs << endl;
    ofs << endl;


    ITERATE(TConfigSite, p, m_ConfigSite) {
        if (p->second == '1') {
            ofs << "#define " << p->first << " " << p->second << endl;
        } else {
            ofs << "/* #undef " << p->first << " */" << endl;
        }
    }
    ofs << endl;
}


END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.19  2004/11/29 14:18:50  gouriano
 * Minor fix to make it compile on GCC 3.4.3
 *
 * Revision 1.18  2004/11/23 20:12:12  gouriano
 * Tune libraries with the choice for each configuration independently
 *
 * Revision 1.17  2004/11/09 19:48:46  gouriano
 * Do not write header's ifdef into ncbiconf_msvc_site.*.h
 *
 * Revision 1.16  2004/11/09 17:39:03  gouriano
 * Changed generation rules for ncbiconf_msvc_site.h
 *
 * Revision 1.15  2004/08/04 17:06:02  gouriano
 * Leave defines that do not meet definition criteria undefined
 * instead of defining them as 0
 *
 * Revision 1.14  2004/07/20 13:38:40  gouriano
 * Added conditional macro definition
 *
 * Revision 1.13  2004/06/15 15:04:41  gorelenk
 * Changed CMsvcConfigure::operator() to be correctly compiled on GCC295 and
 * GCC333 .
 *
 * Revision 1.12  2004/06/09 14:14:45  gorelenk
 * Added static helper function s_CreateThirdPartyLibsInstallMakefile.
 * Changed implementation of CMsvcConfigure::operator() to use helper above.
 * Also added generation of installation makefiles for static builds.
 *
 * Revision 1.11  2004/06/03 14:04:38  gorelenk
 * Changed CMsvcConfigure::operator() :
 * added third party libraries install key support.
 *
 * Revision 1.10  2004/05/27 13:43:52  gorelenk
 * Changed name of generated makefile for auto third-parties install to
 * 'Makefile.third_party.mk'.
 *
 * Revision 1.9  2004/05/24 15:14:07  gorelenk
 * Changed implementation of CMsvcConfigure::operator().
 *
 * Revision 1.8  2004/05/24 14:43:19  gorelenk
 * Changed implementation of CMsvcConfigure::operator() - added code section
 * for generation of makefile to install third-party libs.
 *
 * Revision 1.7  2004/05/21 21:41:41  gorelenk
 * Added PCH ncbi_pch.hpp
 *
 * Revision 1.6  2004/05/07 15:22:17  gorelenk
 * Changed implementation of CMsvcConfigure::WriteNcbiconfMsvcSite .
 *
 * Revision 1.5  2004/04/19 15:41:50  gorelenk
 * Static helper function s_LibOk changed to function IsLibOk.
 *
 * Revision 1.4  2004/02/24 20:51:59  gorelenk
 * Changed implementation of function s_LibOk
 * and member-function ProcessDefine of class CMsvcConfigure.
 *
 * Revision 1.3  2004/02/10 18:23:33  gorelenk
 * Implemented overwriting of site defines file *.h only when new file is
 * different from already present one.
 *
 * Revision 1.2  2004/02/06 23:14:59  gorelenk
 * Implemented support of ASN projects, semi-auto configure,
 * CPPFLAGS support. Second working version.
 *
 * Revision 1.1  2004/02/04 23:19:22  gorelenk
 * Initial revision.
 *
 * ===========================================================================
 */
