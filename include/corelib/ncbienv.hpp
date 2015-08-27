#ifndef NCBIENV__HPP
#define NCBIENV__HPP

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
 * Authors:  Denis Vakatov, Eugene Vasilchenko
 *
 *
 */

/// @file ncbienv.hpp
/// Defines unified interface to application:
/// - Environment        -- CNcbiEnvironment
/// - Command-line args  -- CNcbiArguments


#include <corelib/ncbimtx.hpp>
#include <map>
#include <iterator>

/// Avoid name clash with the NCBI C Toolkit.
#if !defined(NCBI_OS_UNIX)  ||  defined(HAVE_NCBI_C)
#  if defined(GetProgramName)
#    undef GetProgramName
#  endif
#  define GetProgramName GetProgramName
#  if defined(SetProgramName)
#    undef SetProgramName
#  endif
#  define SetProgramName SetProgramName
#endif


/** @addtogroup Environment
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// CArgumentsException --
///
/// Define exceptions generated by CArgumentsApplication.
///
/// CArgumentsException inherits its basic functionality from CCoreException
/// and defines additional error codes for applications.

class NCBI_XNCBI_EXPORT CArgumentsException : public CCoreException
{
public:
    /// Error types that arguments processing can generate.
    enum EErrCode {
        eNegativeArgc,  ///< Negative argc value
        eNoArgs         ///< No arguments
    };

    /// Translate from the error code value to its string representation.
    virtual const char* GetErrCodeString(void) const;

    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CArgumentsException, CCoreException);
};



/////////////////////////////////////////////////////////////////////////////
///
/// CNcbiEnvironment --
///
/// Define the application environment.
///
/// CNcbiEnvironment provides a data structure for storing, accessing and
/// modifying the environment variables accessed by the C library routine
/// getenv().

class NCBI_XNCBI_EXPORT CNcbiEnvironment
{
public:
    /// Constructor.
    CNcbiEnvironment(void);

    /// Constructor with the envp parameter.
    CNcbiEnvironment(const char* const* envp);

    /// Destructor.
    virtual ~CNcbiEnvironment(void);

    /// Reset environment.
    ///
    /// Delete all cached entries, load new ones from "envp" (if not NULL).
    void Reset(const char* const* envp = 0);

    /// Get environment value by name.
    ///
    /// If environmnent value is not cached then call "Load(name)" to load
    /// the environmnent value.  The loaded name/value pair will then be
    /// cached, too, after the call to "Get()".
    const string& Get(const string& name, bool* found = NULL) const;

    /// Find all variable names starting with an optional prefix.
    void Enumerate(list<string>& names, const string& prefix = kEmptyStr)
        const;

    /// Set an environment variable by name
    ///
    /// This will throw an exception if setting the variable fails
    void Set(const string& name, const string& value);

    /// Delete an environment variable by name
    /// @param name environment variable name [in]
    void Unset(const string& name);

protected:
    /// Load value of specified environment variable.
    virtual string Load(const string& name, bool& found) const;

private:
    /// Cached environment <name,value> pair.
    struct SEnvValue {
        SEnvValue(void) : ptr(NULL) {}
        SEnvValue(const string& v, const TXChar* p) : value(v), ptr(p) {}

        string value;  // cached value
        // NULL if the corresponding environment variable is unset.
        // kEmptyXCStr if the value was loaded from the environment.
        // A string created by strdup() if the value came from Set().
        const TXChar* ptr;
    };
    typedef map<string, SEnvValue> TCache;
    mutable TCache m_Cache;
    mutable CFastMutex m_CacheMutex;
};



/////////////////////////////////////////////////////////////////////////////
///
/// CAutoEnvironmentVariable --
///
/// Establish an environment setting for a limited time.
///
/// CAutoEnvironmentVariable establishes an environment variable setting
/// for the lifetime of the instance (which may be associated with a unit
/// test case), restoring the previous value (if any) when destroyed.
class NCBI_XNCBI_EXPORT CAutoEnvironmentVariable
{
public:
    /// Initializes the environment variable passed as an argument to the
    /// corresponding value ("1" by default)
    /// @param var_name environment variable name [in]
    /// @param value value to set the environment variable to [in]
    CAutoEnvironmentVariable(const CTempString& var_name,
                             const CTempString& value = "1",
                             CNcbiEnvironment*  env = NULL);

    /// Destructor which restores the modifications made in the environment by
    /// this class
    ~CAutoEnvironmentVariable();

private:
    /// Affected CNcbiEnvironment instance
    AutoPtr<CNcbiEnvironment> m_Env;
    /// Name of the environment variable manipulated
    string                    m_VariableName;
    /// Previous value of the environment variable manipulated
    string                    m_PrevValue;
    /// Was the variable originally set at all?
    bool                      m_WasSet;
};



/////////////////////////////////////////////////////////////////////////////
///
/// CEnvironmentCleaner --
///
/// Remove unwanted settings from the environment, for instance to allow
/// test suites to start from suitably clean slates (in which case a global
/// static instance may be in order).
class NCBI_XNCBI_EXPORT CEnvironmentCleaner
{
public:
    /// Immediately clean some settings, to be passed in as a NULL-terminated
    /// sequence of C strings.
    CEnvironmentCleaner(const char* s = NULL, ...);

    /// Clean the specified setting.
    void Clean(const string& name);
};



/////////////////////////////////////////////////////////////////////////////
///
/// CNcbiArguments --
///
/// Store application command-line arguments & application name.
///
/// CNcbiArgument provides a data structure for storing and accessing the
/// command line arguments and application name.

class NCBI_XNCBI_EXPORT CNcbiArguments
{
public:
    /// Constructor.
    CNcbiArguments(int argc,                ///< Standard argument count
                   const char* const* argv, ///< Standard argument vector
                   const string& program_name = kEmptyStr, ///< Program name
                   const string& real_name = kEmptyStr ///< Resolved name
                  );

    /// Destructor.
    virtual ~CNcbiArguments(void);

    /// Copy constructor.
    CNcbiArguments(const CNcbiArguments& args);

    /// Assignment operator.
    CNcbiArguments& operator= (const CNcbiArguments& args);

    /// Reset arguments.
    ///
    /// Delete all cached args and program name.  Load new ones from "argc",
    /// "argv", "program_name", and "real_name".
    void Reset(int argc,                    ///< Standard argument count
               const char* const* argv,     ///< Standard argument vector
               const string& program_name = kEmptyStr, ///< Program name
               const string& real_name = kEmptyStr ///< Resolved name
              );

    /// Get size (number) of arguments.
    SIZE_TYPE Size(void) const { return m_Args.size(); }

    /// Get the argument specified by "pos".
    const string& operator[] (SIZE_TYPE pos) const { return m_Args[pos]; }

    /// Add a new argument.
    void Add(const string& arg);

    /// Delete arguments from 1 to n.
    void Shift(int n=1);

    /// Get program name.
    const string& GetProgramName(EFollowLinks follow_links = eIgnoreLinks)
        const;

    /// Get program base name.
    string GetProgramBasename(EFollowLinks follow_links = eIgnoreLinks) const;

    /// Get program directory name.
    ///
    /// Program name includes the last '/'.
    string GetProgramDirname (EFollowLinks follow_links = eIgnoreLinks) const;

    /// Set program name.  If real_name is supplied, it should be the
    /// fully resolved path to the executable (whereas program_name
    /// may legitimately involve symlinks).
    void SetProgramName(const string& program_name,
                        const string& real_name = kEmptyStr);

private:
    string        m_ProgramName;  ///< Program name if different from the
                                  ///< default m_Args[0]
    deque<string> m_Args;         ///< Queue of arguments

    mutable string     m_ResolvedName;
    mutable CFastMutex m_ResolvedNameMutex;
};


END_NCBI_SCOPE


/* @} */

#endif  /* NCBIENV__HPP */
