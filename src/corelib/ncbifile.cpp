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
 * Author: Vladimir Ivanov
 *
 * File Description:
 *    Files and directories accessory functions
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.10  2001/12/26 20:58:22  juran
 * Use an FSSpec* member instead of an FSSpec, so a forward declaration can be used.
 * Add copy constructor and assignment operator for CDirEntry on Mac OS,
 * thus avoiding memberwise copy which would blow up upon deleting the pointer twice.
 *
 * Revision 1.9  2001/12/18 21:36:38  juran
 * Remove unneeded Mac headers.
 * (Required functions copied to ncbi_os_mac.cpp)
 * MoveRename PStr to PString in ncbi_os_mac.hpp.
 * Don't use MoreFiles xxxCompat functions.  They're for System 6.
 * Don't use global scope operator on functions copied into NCBI scope.
 *
 * Revision 1.8  2001/11/19 23:38:44  vakatov
 * Fix to compile with SUN WorkShop (and maybe other) compiler(s)
 *
 * Revision 1.7  2001/11/19 18:10:13  juran
 * Whitespace.
 *
 * Revision 1.6  2001/11/15 16:34:12  ivanov
 * Moved from util to corelib
 *
 * Revision 1.5  2001/11/06 14:34:11  ivanov
 * Fixed compile errors in CDir::Contents() under MS Windows
 *
 * Revision 1.4  2001/11/01 21:02:25  ucko
 * Fix to work on non-MacOS platforms again.
 *
 * Revision 1.3  2001/11/01 20:06:48  juran
 * Replace directory streams with Contents() method.
 * Implement and test Mac OS platform.
 *
 * Revision 1.2  2001/09/19 16:22:18  ucko
 * S_IFDOOR is nonportable; make sure it exists before using it.
 * Fix type of second argument to CTmpStream's constructor (caught by gcc 3).
 *
 * Revision 1.1  2001/09/19 13:06:09  ivanov
 * Initial revision
 *
 *
 * ===========================================================================
 */

#include <corelib/ncbifile.hpp>

#ifndef NCBI_OS_MAC
#  include <sys/types.h>
#  include <sys/stat.h>
#endif

#include <stdio.h>

#if defined NCBI_OS_MSWIN 
#  include <io.h>
#  include <direct.h>
#elif defined NCBI_OS_UNIX
#  include <unistd.h>
#  include <dirent.h>
#elif defined NCBI_OS_MAC
#  include <corelib/ncbi_os_mac.hpp>
#  include <Script.h>
#endif


BEGIN_NCBI_SCOPE


#ifdef NCBI_OS_MAC

static const FSSpec sNullFSS = {0, 0, "\p"};

static bool operator==(const FSSpec &one, const FSSpec &other)
{
    return one.vRefNum == other.vRefNum
        && one.parID   == other.parID
        && PString(one.name) == PString(other.name);
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// Static functions
//

// Construct real entry mode from parts. Parameters can not have "fDefault" 
// value.
static CDirEntry::TMode s_ConstructMode(CDirEntry::TMode user_mode, 
                                        CDirEntry::TMode group_mode, 
                                        CDirEntry::TMode other_mode)
{
    CDirEntry::TMode mode = 0;
    mode |= (user_mode  << 6);
    mode |= (group_mode << 3);
    mode |= other_mode;
    return mode;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDirEntry
//


CDirEntry::CDirEntry()
#ifdef NCBI_OS_MAC
    : m_FSS(new FSSpec(sNullFSS))
#endif
{
}

#ifdef NCBI_OS_MAC
CDirEntry::CDirEntry(const CDirEntry& other) : m_FSS(new FSSpec(*other.m_FSS))
{
    m_DefaultMode[eUser]  = other.m_DefaultMode[eUser];
    m_DefaultMode[eGroup] = other.m_DefaultMode[eGroup];
    m_DefaultMode[eOther] = other.m_DefaultMode[eOther];
}

CDirEntry&
CDirEntry::operator=(const CDirEntry& other)
{
	*m_FSS = *other.m_FSS;
    m_DefaultMode[eUser]  = other.m_DefaultMode[eUser];
    m_DefaultMode[eGroup] = other.m_DefaultMode[eGroup];
    m_DefaultMode[eOther] = other.m_DefaultMode[eOther];
    return *this;
}

CDirEntry::CDirEntry(const FSSpec& fss) : m_FSS(new FSSpec(fss))
{
    m_DefaultMode[eUser]  = m_DefaultModeGlobal[eFile][eUser];
    m_DefaultMode[eGroup] = m_DefaultModeGlobal[eFile][eGroup];
    m_DefaultMode[eOther] = m_DefaultModeGlobal[eFile][eOther];
}
#endif

CDirEntry::CDirEntry(const string& name)
#ifdef NCBI_OS_MAC
    : m_FSS(new FSSpec(sNullFSS))
#endif
{
    Reset(name);
    m_DefaultMode[eUser]  = m_DefaultModeGlobal[eFile][eUser];
    m_DefaultMode[eGroup] = m_DefaultModeGlobal[eFile][eGroup];
    m_DefaultMode[eOther] = m_DefaultModeGlobal[eFile][eOther];
}

#ifdef NCBI_OS_MAC
bool
CDirEntry::operator== (const CDirEntry& other) const
{
    return *m_FSS == *other.m_FSS;
}

const FSSpec&
CDirEntry::FSS() const
{
    return *m_FSS;
}
#endif


char CDirEntry::GetPathSeparator()
{
#if defined NCBI_OS_MSWIN
    return '\\';
#elif defined NCBI_OS_UNIX
    return '/';
#elif defined NCBI_OS_MAC
    return ':';
#endif    
}


CDirEntry::TMode CDirEntry::m_DefaultModeGlobal[eUnknown][3] =
{
    // eFile
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eDir
    { CDirEntry::fDefaultDirUser, CDirEntry::fDefaultDirGroup, 
      CDirEntry::fDefaultDirOther },
    // ePipe
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eLink
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eSocket
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eDoor
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eBlockSpecial
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther },
    // eCharSpecial
    { CDirEntry::fDefaultUser, CDirEntry::fDefaultGroup, 
      CDirEntry::fDefaultOther }
};



#ifdef NCBI_OS_MAC
void CDirEntry::Reset(const string& path)
{
    OSErr err = MacPathname2FSSpec(path.c_str(), m_FSS);
    if (err != noErr && err != fnfErr) {
        *m_FSS = sNullFSS;
    }
}

string CDirEntry::GetPath(void) const
{
    OSErr err;
    char *path;
    err = MacFSSpec2FullPathname(&FSS(), &path);
    if (err != noErr) {
        return "";
    }
    return string(path);
}
#endif


CDirEntry::~CDirEntry(void)
{
	delete m_FSS;
}

void CDirEntry::SplitPath(const string& path, string* dir,
                          string* base, string* ext)
{
    // Get file name
    size_t pos = path.rfind(GetPathSeparator());
    string filename = (pos == NPOS) ? path : path.substr(pos+1);

    // Get dir
    if ( dir ) {
        *dir = (pos == NPOS) ? kEmptyStr : path.substr(0, pos+1);
    }
    // Split file name to base and extension
    pos = filename.rfind('.');
    if ( base ) {
        *base = (pos == NPOS) ? filename : filename.substr(0, pos);
    }
    if ( ext ) {
        *ext = (pos == NPOS) ? kEmptyStr : filename.substr(pos);
    }
}


string CDirEntry::MakePath(const string& dir, const string& base, 
                           const string& ext)
{
    string path;   // Result string

    // Adding "dir" to path
    path += dir;
    size_t pos = dir.length();
    // Adding path separator, if need
    if ( pos  &&  dir.at(pos-1) != GetPathSeparator() ) {
        path += GetPathSeparator();
    }
    // Adding file base
    path += base;
    
    // Adding extension
    if ( ext.length()  &&  ext.at(0) != '.' ) {
        path += '.';
    }
    path += ext;

    // Return result
    return path;
}


/*  Match "name" against the filename "mask", returning TRUE if
    it matches, FALSE if not.  
*/
bool CDirEntry::MatchesMask(const char *name, const char *mask) 
{
    char c;

    for (;;) {
        // Analyze symbol in mask
		switch ( c = *mask++ ) {
        
        case '\0':
			return *name == '\0';

        case '?':
            if ( *name == '\0' ) {
				return false;
            }
			++name;
			break;
		
        case '*':
			c = *mask;
			// Collapse multiple stars
            while ( c == '*' ) {
				c = *++mask;
            }
            if (c == '\0') {
    			return true;
			}
			// General case, use recursion
			while ( char ctmp = *name ) {
                if ( MatchesMask(name, mask) ) {
					return true;
                }
				++name;
			}
			return false;
		
        default:
            // Compare nonpattern character in mask and name
            if ( c != *name++ ) {
				return false;
            }
			break;
		}
    }
    return false;
}


bool CDirEntry::GetMode(TMode* user_mode, TMode* group_mode, TMode* other_mode)
    const
{
#ifdef NCBI_OS_MAC
    FSSpec fss = FSS();
    OSErr err = FSpCheckObjectLock(&fss);
    if (err != noErr  &&  err != fLckdErr) {
        return false;
    }
    bool locked = (err == fLckdErr);
    *user_mode = fRead | (locked ? 0 : fWrite);
    *group_mode = *other_mode = *user_mode;
    return true;
#else
    struct stat st;
    if (stat(GetPath().c_str(), &st) != 0) {
        return false;
    }
    // Other
    if (other_mode) {
        *other_mode = st.st_mode & 0007;
    }
    st.st_mode >>= 3;
    // Group
    if (group_mode) {
        *group_mode = st.st_mode & 0007;
    }
    st.st_mode >>= 3;
    // User
    if (user_mode) {
        *user_mode = st.st_mode & 0007;
    }
    return true;
#endif
}


bool CDirEntry::SetMode(TMode user_mode, TMode group_mode, TMode other_mode)
    const
{
    if (user_mode == fDefault) {
        user_mode = m_DefaultMode[eUser];
    }
#ifdef NCBI_OS_MAC
    bool wantLocked = (user_mode & fWrite) == 0;
    FSSpec fss = FSS();
    OSErr  err = FSpCheckObjectLock(&fss);
    if (err != noErr  &&  err != fLckdErr) {
        return false;
    }
    bool locked = (err == fLckdErr);
    if (locked == wantLocked) {
        return true;
    }
    err = wantLocked 
        ? ::FSpSetFLock(&fss) 
        : ::FSpRstFLock(&fss);
	
    return err == noErr;
#else
    if (group_mode == fDefault) {
        group_mode = m_DefaultMode[eGroup];
    }
    if (other_mode == fDefault) {
        other_mode = m_DefaultMode[eOther];
    }
    TMode mode = s_ConstructMode(user_mode, group_mode, other_mode);

    return chmod(GetPath().c_str(), mode) == 0;
#endif
}


void CDirEntry::SetDefaultModeGlobal(EType entry_type, TMode user_mode, 
                                     TMode group_mode, TMode other_mode)
{
    if (entry_type >= eUnknown ) {
        return;
    }
    if (entry_type == eDir ) {
        if ( user_mode == fDefault ) {
            user_mode = fDefaultDirUser;
        }
        if ( group_mode == fDefault ) {
            group_mode = fDefaultDirGroup;
        }
        if ( other_mode == fDefault ) {
            other_mode = fDefaultDirOther;
        }
    } else {
        if ( user_mode == fDefault ) {
            user_mode = fDefaultUser;
        }
        if ( group_mode == fDefault ) {
            group_mode = fDefaultGroup;
        }
        if ( other_mode == fDefault ) {
            other_mode = fDefaultOther;
        }
    }
    m_DefaultModeGlobal[entry_type][eUser]  = user_mode;
    m_DefaultModeGlobal[entry_type][eGroup] = group_mode;
    m_DefaultModeGlobal[entry_type][eOther] = other_mode;
}


void CDirEntry::SetDefaultMode(EType entry_type, TMode user_mode, 
                               TMode group_mode, TMode other_mode)
{
    if ( user_mode == fDefault ) {
        user_mode = m_DefaultModeGlobal[entry_type][eUser];
    }
    if ( group_mode == fDefault ) {
        group_mode = m_DefaultModeGlobal[entry_type][eGroup];
    }
    if ( other_mode == fDefault ) {
        other_mode = m_DefaultModeGlobal[entry_type][eOther];
    }
    m_DefaultMode[eUser]  = user_mode;
    m_DefaultMode[eGroup] = group_mode;
    m_DefaultMode[eOther] = other_mode;
}


void CDirEntry::GetDefaultModeGlobal(EType  entry_type, TMode* user_mode,
                                     TMode* group_mode, TMode* other_mode)
{
    if ( user_mode ) {
        *user_mode = m_DefaultModeGlobal[entry_type][eUser];
    }
    if ( group_mode ) {
        *group_mode = m_DefaultModeGlobal[entry_type][eGroup];
    }
    if ( other_mode ) {
        *other_mode = m_DefaultModeGlobal[entry_type][eOther];
    }
}


void CDirEntry::GetDefaultMode(TMode* user_mode, TMode* group_mode,
                               TMode* other_mode) const
{
    if ( user_mode ) {
        *user_mode = m_DefaultMode[eUser];
    }
    if ( group_mode ) {
        *group_mode = m_DefaultMode[eGroup];
    }
    if ( other_mode ) {
        *other_mode = m_DefaultMode[eOther];
    }
}


CDirEntry::EType CDirEntry::GetType(void) const
{
#if defined(NCBI_OS_MAC)
    OSErr   err;
    long    dirID;
    Boolean isDir;
    err = FSpGetDirectoryID(&FSS(), &dirID, &isDir);
    if ( err )
        return eUnknown;
    return isDir ? eDir : eFile;
#else
    struct stat st;
    if (stat(GetPath().c_str(), &st) != 0) {
        return eUnknown;
    }
    if ( (st.st_mode & S_IFREG)  == S_IFREG ) {
        return eFile;
    }
    if ( (st.st_mode & S_IFDIR)  == S_IFDIR ) {
        return eDir;
    }
    if ( (st.st_mode & S_IFCHR)  == S_IFCHR ) {
        return eCharSpecial;
    }

#  if defined(NCBI_OS_MSWIN)
    if ( (st.st_mode & _S_IFIFO) == _S_IFIFO ) {
        return ePipe;
    }

#  elif defined(NCBI_OS_UNIX)
    if ( (st.st_mode & S_IFIFO)  == S_IFIFO ) {
        return ePipe;
    }
    if ( (st.st_mode & S_IFLNK)  == S_IFLNK ) {
        return eLink;
    }
    if ( (st.st_mode & S_IFSOCK) == S_IFSOCK ) {
        return eSocket;
    }
#    ifdef S_IFDOOR /* only Solaris seems to have this */
    if ( (st.st_mode & S_IFDOOR) == S_IFDOOR ) {
        return eDoor;
    }
#    endif
    if ( (st.st_mode & S_IFBLK)  == S_IFBLK ) {
        return eBlockSpecial;
    }

#  endif

    return eUnknown;
#endif
}
 

bool CDirEntry::Rename(const string& newname)
{
#ifdef NCBI_OS_MAC
    const int maxFilenameLength = 31;
    if (newname.length() > maxFilenameLength) return false;
    Str31 newNameStr;
    Pstrcpy(newNameStr, newname.c_str());
    OSErr err = FSpRename(&FSS(), newNameStr);
    if (err != noErr) return false;
#else
    if (rename(GetPath().c_str(), newname.c_str()) != 0) {
        return false;
    }
#endif
    Reset(newname);
    return true;
}


bool CDirEntry::Remove(void) const
{
#ifdef NCBI_OS_MAC
    OSErr err = ::FSpDelete(&FSS());
    return err == noErr;
#else
    if ( IsDir() ) {
        return rmdir(GetPath().c_str()) == 0;
    } else {
        return remove(GetPath().c_str()) == 0;
    }
#endif
}



//////////////////////////////////////////////////////////////////////////////
//
// CFile
//

CFile::CFile(const string& filename) : CParent(filename)
{ 
    // Set default mode
    SetDefaultMode(eFile, fDefault, fDefault, fDefault);
}


CFile::~CFile(void)
{ 
    return;
}


Int8 CFile::GetLength(void) const
{
#ifdef NCBI_OS_MAC
    long dataSize, rsrcSize;
    OSErr err = FSpGetFileSize(&FSS(), &dataSize, &rsrcSize);
    if (err != noErr) {
        return -1;
    } else {
        return dataSize;
    }
#else
    struct stat buf;
    if ( stat(GetPath().c_str(), &buf) != 0 ) {
        return -1;
    }
    return buf.st_size;
#endif
}


string CFile::GetTmpName(void)
{
    char* filename = tmpnam(0);
    if ( !filename ) {
        return kEmptyStr;
    }
    return filename;
}


string CFile::GetTmpNameExt(const string& dir, const string& prefix)
{
#if defined(NCBI_OS_MAC)
    return kEmptyStr;
#else
    char* filename = tempnam(dir.c_str(), prefix.c_str());
    if ( !filename ) {
        return kEmptyStr;
    }
    string res(filename);
    free(filename);
    return res;   
#endif
}


class CTmpStream : public fstream
{
public:
    CTmpStream(const char *s, IOS_BASE::openmode mode) : fstream(s, mode) 
    {
        m_FileName = s; 
    }
    virtual ~CTmpStream(void) 
    { 
        CFile(m_FileName).Remove();
    }
protected:
    string m_FileName;
};


fstream* CFile::CreateTmpFile(const string& filename, 
                              ETextBinary text_binary,
                              EAllowRead  allow_read)
{
    ios::openmode mode = ios::out;
    if ( text_binary == eBinary ) {
        mode = mode | ios::binary;
    }
    if ( allow_read == eAllowRead ) {
        mode = mode | ios::in;
    }
    string tmpname = filename.empty() ? GetTmpName() : filename;
    fstream* stream = new CTmpStream(tmpname.c_str(), mode);
    return stream;
}


fstream* CFile::CreateTmpFileExt(const string& dir, const string& prefix,
                                 ETextBinary text_binary, 
                                 EAllowRead allow_read)
{
    return CreateTmpFile(GetTmpNameExt(dir, prefix), text_binary, allow_read);
}



//////////////////////////////////////////////////////////////////////////////
// CDir


CDir::CDir(void)
{
    return;
}


#if defined(NCBI_OS_MAC)
CDir::CDir(const FSSpec& fss) : CParent(fss)
{
    // Set default mode
    SetDefaultMode(eDir, fDefault, fDefault, fDefault);
}
#endif


CDir::CDir(const string& dirname) : CParent(dirname)
{
    // Set default mode
    SetDefaultMode(eDir, fDefault, fDefault, fDefault);
}


CDir::~CDir(void)
{
    return;
}


#if defined(NCBI_OS_MAC)
static const CDirEntry MacGetIndexedItem(const CDir& container, SInt16 index)
{
    FSSpec dir = container.FSS();
    FSSpec fss;  // FSSpec of item gotten.
    SInt16 actual;  // Actual number of items gotten.  Should be one or zero.
    SInt16 itemIndex = index;
    OSErr err = GetDirItems(dir.vRefNum, dir.parID, dir.name, true, true, 
                            &fss, 1, &actual, &itemIndex);
    if (err != noErr) {
        throw err;
    }
    return CDirEntry(fss);
}
#endif


CDir::TEntries CDir::GetEntries(const string& mask) const
{
    TEntries contents;
    string x_mask = mask.empty() ? string("*") : mask;

#if defined NCBI_OS_MSWIN
    // Append to the "path" mask for all files in directory
    string pattern = GetPath();
    if ( pattern[pattern.size()-1] != GetPathSeparator() ) {
        pattern += GetPathSeparator();
    }
    pattern += x_mask;

    // Open directory stream and try read info about first entry
    struct _finddata_t entry;
    long desc = _findfirst(pattern.c_str(), &entry);  // get first entry's name
    if (desc != -1) {
        contents.push_back(new CDirEntry(entry.name));
        while ( _findnext(desc, &entry) != -1 ) {
            contents.push_back(new CDirEntry(entry.name));
        }
    }
    _findclose(desc);

#elif defined NCBI_OS_UNIX
    DIR* dir = opendir(GetPath().c_str());
    if ( dir ) {
        while (struct dirent* entry = readdir(dir)) {
            if ( MatchesMask(entry->d_name, x_mask.c_str()) ) {
                contents.push_back(new CDirEntry(entry->d_name));
            }
        }
    }
    closedir(dir);

#elif defined NCBI_OS_MAC
    try {
        for (int index = 1;  ;  index++) {
            CDirEntry entry = MacGetIndexedItem(*this, index);
            if ( MatchesMask(entry.GetName().c_str(), x_mask.c_str()) ) {
                contents.push_back(new CDirEntry(entry));
            }
        }
    } catch (OSErr& err) {
        if (err != fnfErr) {
            throw COSErrException_Mac(err, "CDir::GetEntries() ");
        }
    }
#endif

    return contents;
}


bool CDir::Create(void) const
{
    TMode user_mode, group_mode, other_mode;

    GetDefaultMode(&user_mode, &group_mode, &other_mode);
    TMode mode = s_ConstructMode(user_mode, group_mode, other_mode);

#if defined NCBI_OS_MSWIN
    if ( mkdir(GetPath().c_str()) != 0 ) {
        return false;
    }
    return chmod(GetPath().c_str(), mode) == 0;

#elif defined NCBI_OS_UNIX

    return mkdir(GetPath().c_str(), mode) == 0;

#elif defined NCBI_OS_MAC
    OSErr err;
    long dirID;
	
    err = ::FSpDirCreate(&FSS(), smRoman, &dirID);
    return err == noErr;

#endif
}


bool CDir::Remove(EDirRemoveMode mode) const
{
    // Remove directory as empty
    if ( mode == eOnlyEmpty ) {
        return CParent::Remove();
    }

    CDir dir(*this);

    // List for subdirectories
    list<CDir> dirlist;

    // Read and remove all entry in derectory
    TEntries contents = dir.GetEntries();
    iterate(TEntries, entry, contents) {
#ifndef NCBI_OS_MAC
        if ( (*entry)->GetName() == "."  ||  (*entry)->GetName() == ".."  ||  
             (*entry)->GetName() == string(1, GetPathSeparator()) ) {
            continue;
        }
#endif
#ifdef NCBI_OS_MAC
        CDirEntry& item = **entry;
        // Is it directory ?
        if ( item.IsDir() ) {
            if ( mode == eRecursive ) {
                dirlist.push_back(CDir(item.FSS()));
            }
        } else {
            // It is a file
            if ( !item.Remove() ) {
                return false;
            }
        }
#else
        string path = GetPath() + GetPathSeparator() + (*entry)->GetName();
        CDirEntry item(path);
        if ( item.IsDir() ) {
            if ( mode == eRecursive ) {
                dirlist.push_back(CDir(path));
            }
        } else {
            // It is a file
            if ( !item.Remove() ) {
                return false;
            }
        }
#endif
    }
    // If need remove subdirectories
    if ( mode == eRecursive ) {
        iterate(list<CDir>, it, dirlist) {
            if ( !it->Remove(eRecursive) ) {
                return false;
            }
        }
    }
    // Remove main directory
    return CParent::Remove();
}


END_NCBI_SCOPE
