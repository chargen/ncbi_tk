#ifndef UTIL_COMPRESS__TAR__HPP
#define UTIL_COMPRESS__TAR__HPP

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
 * Authors:  Vladimir Ivanov
 *
 * File Description:  TAR archive API
 *                    Now supported only POSIX.1-1988 (ustar) format.
 *                    GNU tar format is supported partially.
 *                    New archives creates using GNU format.
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbifile.hpp>
#include <corelib/ncbi_mask.hpp>

/** @addtogroup Compression
 *
 * @{
 */

BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// CTarException --
///
/// Define exceptions generated by TAR API.
///
/// CTarException inherits its basic functionality from CCoreException
/// and defines additional error codes for TAR-archive operations.

class NCBI_XUTIL_EXPORT CTarException : public CCoreException
{
public:
    /// Error types that file operations can generate.
    enum EErrCode {
        eUnsupportedTarFormat,
        eUnsupportedEntryType,
        eBadName,
        eLongName,
        eCRC,
        eCreate,
        eOpen,
        eRead,
        eWrite,
        eBackup,
        eMemory,
        eRestoreAttrs
    };

    /// Translate from an error code value to its string representation.
    virtual const char* GetErrCodeString(void) const
    {
        switch (GetErrCode()) {
        case eUnsupportedTarFormat: return "eUnsupportedTarFormat";
        case eUnsupportedEntryType: return "eUnsupportedEntryType";
        case eBadName:              return "eBadName";
        case eLongName:             return "eTooLongName";
        case eCRC:                  return "eCRC";
        case eCreate:               return "eCreate";
        case eOpen:                 return "eOpen";
        case eRead:                 return "eRead";
        case eWrite:                return "eWrite";
        case eBackup:               return "eBackup";
        case eMemory:               return "eMemory";
        case eRestoreAttrs:         return "eRestoreAttrs";
        default:                    return CException::GetErrCodeString();
        }
    }
    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CTarException, CCoreException);
};


//////////////////////////////////////////////////////////////////////////////
///
/// CTarEntryInfo class
///
/// Store information about TAR archive entry

class NCBI_XUTIL_EXPORT CTarEntryInfo
{
public:
    /// Which entry type.
    enum EType {
        eFile        = CDirEntry::eFile,    ///< Regular file
        eDir         = CDirEntry::eDir,     ///< Directory
        eLink        = CDirEntry::eLink,    ///< Symbolic link
        eUnknown     = CDirEntry::eUnknown, ///< Unknown type
        eGNULongName = eUnknown + 1,        ///< GNU long name
        eGNULongLink = eUnknown + 2         ///< GNU long link
    };

    // Constructor
    CTarEntryInfo(void)
        : m_Type(eUnknown)
    {
        memset(&m_Stat, 0, sizeof(m_Stat));
    }

    // Setters
    void SetName(const string& name)      { m_Name          = name;        }
    void SetType(EType type)              { m_Type          = type;        }
    void SetSize(Int8 size)               { m_Stat.st_size  = (off_t)size; }
    void SetMode(unsigned int mode)       { m_Stat.st_mode  = mode;        }
    void SetUserId(unsigned int uid)      { m_Stat.st_uid   = uid;         }
    void SetGroupId(unsigned int gid)     { m_Stat.st_gid   = gid;         }
    void SetLinkName(const string& name)  { m_LinkName      = name;        }
    void SetUserName(const string& name)  { m_UserName      = name;        }
    void SetGroupName(const string& name) { m_GroupName     = name;        }
    void SetModificationTime(time_t t)    { m_Stat.st_mtime = t;           }

    // Getters
    string       GetName(void)      const { return m_Name;         }
    EType        GetType(void)      const { return m_Type;         }
    Int8         GetSize(void)      const { return m_Stat.st_size; }
    unsigned int GetMode(void)      const { return m_Stat.st_mode; }
    void         GetMode(CDirEntry::TMode* user_mode,
                         CDirEntry::TMode* group_mode = 0,
                         CDirEntry::TMode* other_mode = 0) const;
    unsigned int GetUserId(void)    const { return m_Stat.st_uid;  }
    unsigned int GetGroupId(void)   const { return m_Stat.st_gid;  }
    string       GetLinkName(void)  const { return m_LinkName;     }
    string       GetUserName(void)  const { return m_UserName;     }
    string       GetGroupName(void) const { return m_GroupName;    }
    time_t       GetModificationTime(void) const { return m_Stat.st_mtime; }

private:
    string       m_Name;       ///< Name of file
    string       m_LinkName;   ///< Name of linked file if type is eLink
    EType        m_Type;       ///< Type
    string       m_UserName;   ///< User name
    string       m_GroupName;  ///< Group name (empty string for MSWin)
    struct stat  m_Stat;       ///< Dir entry compatible info

    friend class CTar;
};


//////////////////////////////////////////////////////////////////////////////
///
/// CTar class
///
/// Throw exceptions on error.
/// Note that if the stream constructor was used, that CTar can take only
/// one pass along archive. This means that only one action will be success.
/// Before second action, you should set up a stream pointer to beginning of
/// archive yourself, if it is possible.

class NCBI_XUTIL_EXPORT CTar
{
public:
    /// General flags
    enum EFlags {
        // --- Extract/List/Test ---
        /// Ignore blocks of zeros in archive.
        /// Generally, 2 or more consecutive zero blocks indicate EOF.
        fIgnoreZeroBlocks  = (1<<1),

        // --- Extract/Append ---
        ///< Add/overwrite entries instead of sym.links
        fFollowLinks       = (1<<2),  

        // --- Extract ---
        /// Allow to overwrite existent entries with entries from archive
        fOverwrite         = (1<<3),  
        /// Update entries that is older than entries in archive
        fUpdate            = (1<<4) | fOverwrite,
        /// Backup destination if it exists (all entries including dirs)
        fBackup            = (1<<5) | fOverwrite,
        ///< If destination entry exists, it must have the same type as source
        fEqualTypes        = (1<<6),
        /// Create extracted files with the same ownership
        fPreserveOwner     = (1<<7),
        /// Create extracted files with the same permissions
        fPreservePerm      = (1<<8),
        /// Preserve date/times for extracted files
        fPreserveTime      = (1<<9),
        /// Preserve all attributes
        fPreserveAll       = fPreserveOwner | fPreservePerm | fPreserveTime,

        /// Default flags
        fDefault           = fOverwrite | fPreserveAll
    };
    typedef unsigned int TFlags;  ///< Binary OR of "EFlags"


    /// Constructors
    CTar(const string& file_name);
    CTar(CNcbiIos& stream);

    /// Destructor
    virtual ~CTar(void);

    /// Define a vector of pointers to entries.
    typedef vector< AutoPtr<CTarEntryInfo> > TEntries;


    //------------------------------------------------------------------------
    // Main functions
    //------------------------------------------------------------------------

    /// Create a new empty archive
    ///
    /// If file with such name already exists it will be rewritten.
    /// @sa
    ///   Append, Update
    void Create(void);

    /// Append entries to the end of an archive that already exists.
    ///
    /// Append entries can be directories and files. Entries names cannot
    /// contains '..'. Leading slash for absolute paths will be removed.
    /// All names will be converted to Unix format.
    /// The entry will be added to the end of archive.
    /// @sa
    ///   Create, Update
    void Append(const string& entry_name);

    /// Only append files that are newer than copy in archive.
    ///
    /// Add more recent copies of archive members to the end of an
    /// archive, if they exists.
    /// @sa
    ///   Create, Append
    void Update(const string& entry_name);

/*
    // Delete from the archive (not for use on magnetic tapes :-))
    void Delete(const string& entry_name);

    // Add one or more pre-existing archives to the end of another archive.
    void Concatenate();

    // Find differences between entries in archive and their counterparts
    // in the file system.
    TEntries Diff(const string& diff_dir);
*/

    /// Extract archive to specified directory.
    ///
    /// Extract all archive entries which names matches specified masks.
    /// @param dst_dir
    ///   Directory to extract files.
    /// @sa SetMask,  UnsetMask
    void Extract(const string& dst_dir);

    /// Get information about archive entries.
    ///
    /// @return
    ///   An array containing all archive entries which names matches
    ///   specified masks.
    /// @sa SetMask
    TEntries List(void);

    /// Test archive integrity.
    /// 
    /// Emulate extracting files from archive without creating it on a disk.
    /// @sa SetMask
    void Test(void);


    //------------------------------------------------------------------------
    // Utility functions
    //------------------------------------------------------------------------

    /// Get flags.
    TFlags GetFlags(void) const;

    /// Set flags.
    void SetFlags(TFlags flags);

    /// Set name mask.
    ///
    /// Use this set of masks to process entries which names matches
    /// this masks while trying to list/test/extract entries from TAR archive.
    /// If masks is not defined that process all archive entries.
    /// @param mask
    ///   Set of masks.
    /// @param if_to_own
    ///   Flag to take ownership on the masks (delete on destruction).
    /// @param use_case
    ///   Whether to do a case sensitive compare(eCase -- default), or a
    ///   case-insensitive compare (eNocase).
    void SetMask(CMask *mask, EOwnership if_to_own = eNoOwnership,
                 NStr::ECase use_case = NStr::eCase);

    /// Unset used name masks.
    ///
    /// Means that all entries in the archive will be processed.
    void UnsetMask();

    /// Get base directory to seek added files
    string GetBaseDir(void);

    /// Set base directory to seek added files
    ///
    /// Used for files that have relative path.
    void SetBaseDir(const string& dir_name);

protected:
    /// File archive open mode
    enum EOpenMode {
        eCreate,
        eRead,
        eUpdate,
        eUndefined
    };
    enum EStatus {
        eSuccess = 0,
        eFailure,
        eEOF,
        eZeroBlock
    };
    enum EAction {
        eList,
        eExtract,
        eTest
    };
    enum EMask {
        eUseMask,
        eIgnoreMask
    };

    /// Structure with additional info for processing entries.
    /// Each action interpret field in this structure differently.
    struct SProcessData {
        TEntries entries;
        string   dir;
    };
    
    // Open/close file
    void  x_Open(EOpenMode mode);
    void  x_Close(void);
    
    // Read information about next entry in the TAR archive
    EStatus x_ReadEntryInfo(CTarEntryInfo& info);
    
    // Write information about entry into TAR archive
    void x_WriteEntryInfo(const string& entry_name, CTarEntryInfo& info);

    // Add entry info to list of entries
    void x_AddEntryInfoToList(const CTarEntryInfo& info,
                              TEntries& entries) const;

    // Reader. Read archive and process some "action".
    void x_ReadAndProcess(EAction action, SProcessData *data = 0,
                          EMask use_mask = eUseMask);

    // Process next entry from archive accordingly to specified action.
    // If do_process == TRUE, just skip entry in the stream.
    void x_ProcessEntry(CTarEntryInfo& info, bool do_process, EAction action,
                        SProcessData *data = 0);

    // Restore attributes for specified entry.
    // If 'target' not specified, than CDirEntry will be constructed
    // from 'info'. In this case, 'info' should have correct name for
    // destination dir entry.
    void x_RestoreAttrs(const CTarEntryInfo& info, CDirEntry* target = 0);

    // Increase absolute position in the stream
    void x_IncreaseStreamPos(streamsize size);

    // Read/write specified number of bytes from/to stream
    streamsize x_ReadStream(char *buffer, streamsize n);
    void       x_WriteStream(char *buffer, streamsize n);

    // Check path and convert it to archive name
    string x_ToArchiveName(const string& path) const;

    // Append entry to archive
    void x_Append(const string& entry_name, TEntries* update_list = 0);
    // Append file entry to archive. Accessory function for x_Append().
    void x_AppendFile(const string& entry_name, CTarEntryInfo& info);

protected:
    string         m_FileName;       ///< TAR archive file name.
    CNcbiIos*      m_Stream;         ///< Archive stream (used for I/O).
    CNcbiFstream*  m_FileStream;     ///< File archive stream.
    EOpenMode      m_FileStreamMode; ///< File stream open mode.
    streampos      m_StreamPos;      ///< Current position in m_Stream.
    streamsize     m_BufferSize;     ///< Buffer size for IO operations.
    char*          m_Buffer;         ///< Pointer to working I/O buffer.
    TFlags         m_Flags;          ///< Bitwise OR of flags.
    CMask*         m_Mask;           ///< Masks for list/test/extract.
    EOwnership     m_MaskOwned;      ///< Flag to take ownership for m_Mask.
    NStr::ECase    m_MaskUseCase;    ///< Flag for case sensitive/insensitive
                                     ///< matching by mask.
    string         m_BaseDir;        ///< Base directory to seek added files
                                     ///< without full path.
    string         m_LongName;       ///< Previously defined long name.
    string         m_LongLinkName;   ///< Previously defined long link name.
};


//////////////////////////////////////////////////////////////////////////////
//
// Inline methods
//

inline
void CTar::Create(void)
{
    x_Open(eCreate);
}

inline
void CTar::Append(const string& entry_name)
{
    x_Open(eUpdate);
    x_Append(entry_name);
}

inline
CTar::TEntries CTar::List(void)
{
    SProcessData data;
    x_ReadAndProcess(eList, &data);
    return data.entries;
}

inline
void CTar::Test(void)
{
    x_ReadAndProcess(eTest);
}

inline
CTar::TFlags CTar::GetFlags(void) const
{
    return m_Flags;
}

inline
void CTar::SetFlags(TFlags flags)
{
    m_Flags = flags;
}

inline
void CTar::SetMask(CMask *mask, EOwnership if_to_own, NStr::ECase use_case)
{
    UnsetMask();
    m_Mask = mask;
    m_MaskOwned = if_to_own;
    m_MaskUseCase = use_case;
}

inline
void CTar::UnsetMask()
{
    // Delete owned mask
    if ( m_MaskOwned ) {
        delete m_Mask;
        m_Mask = 0;
    }
}

inline
string CTar::GetBaseDir(void)
{
    return m_BaseDir;
}

inline
void CTar::SetBaseDir(const string& dir_name)
{
    m_BaseDir = CDirEntry::AddTrailingPathSeparator(dir_name);
}

inline 
streamsize CTar::x_ReadStream(char *buffer, streamsize n)
{
    streamsize nread = m_Stream->rdbuf()->sgetn(buffer, n);
    x_IncreaseStreamPos(nread);
    return nread;
}

inline 
void CTar::x_WriteStream(char *buffer, streamsize n)
{
    streamsize nwrite = m_Stream->rdbuf()->sputn(buffer, n);
    if ( nwrite != n ) {
        NCBI_THROW(CTarException, eWrite, "Cannot write to archive");
    }
    x_IncreaseStreamPos(nwrite);
}

inline 
void CTar::x_IncreaseStreamPos(streamsize size)
{
    m_StreamPos += size;
}


END_NCBI_SCOPE


/* @} */


/*
 * ===========================================================================
 * $Log$
 * Revision 1.6  2005/05/05 12:32:33  ivanov
 * + CTar::Update()
 *
 * Revision 1.5  2005/04/27 13:52:58  ivanov
 * Added support for (re)storing permissions/owner/times
 *
 * Revision 1.4  2005/01/31 15:30:59  ivanov
 * Lines wrapped at 79th column
 *
 * Revision 1.3  2005/01/31 14:23:35  ivanov
 * Added class CTarEntryInfo to store information about TAR entry.
 * Added CTar methods:           Create, Append, List, Test.
 * Added CTar utility functions: GetFlags/SetFlags, SetMask/UnsetMask,
 *                               GetBaseDir/SetBaseDir.
 *
 * Revision 1.2  2004/12/14 17:55:48  ivanov
 * Added GNU tar long name support
 *
 * Revision 1.1  2004/12/02 17:46:14  ivanov
 * Initial draft revision
 *
 * ===========================================================================
 */

#endif  /* UTIL_COMPRESS__TAR__HPP */
