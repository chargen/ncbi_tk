#ifndef OBJTOOLS_READERS_SEQDB__SEQDBSQLITE_HPP
#define OBJTOOLS_READERS_SEQDB__SEQDBSQLITE_HPP

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
 * Author:  Thomas W Rackers
 *
 */

#include <objtools/blast/seqdb_reader/seqdb.hpp>

#ifdef HAVE_LIBSQLITE3
#include <db/sqlite/sqlitewrapp.hpp>
#endif

USING_NCBI_SCOPE;
USING_SCOPE(objects);

BEGIN_NCBI_SCOPE

/// Struct mirroring columns of "acc2oid" table in SQLite database
struct SAccOid {
    string m_acc;   /// "bare" accession w/out version suffix
    int m_ver;      /// version number
    int m_oid;      /// OID

    /// Default constructor
    SAccOid() :
        m_acc(""), m_ver(0), m_oid(-1) {}
    /// Explicit constructor
    /// @param acc accession string
    /// @param ver version number
    /// @param oid OID
    SAccOid(const string& acc, const int ver, const int oid) :
        m_acc(acc), m_ver(ver), m_oid(oid) {}
};


/// Struct mirroring columes of "volinfo" table in SQLite database
/// The "volume file" is a database volume's ".nin" or ".pin" file.
/// The modification time is the number of seconds since the UNIX epoch,
/// as would be returned by C library function "time()".
struct SVolInfo {
    const string m_path;        /// full path to volume file
    const time_t m_modTime;     /// modification time of volume file
    const int    m_vol;         /// volume number
    const int    m_oids;        /// number of OIDs in volume

    /// Explicit constructor
    /// @param path full path to volume file
    /// @param modtime modification time of volume file
    /// @param vol volume number, 0 on up
    /// @param oid number of OIDs in volume
    SVolInfo(
            const string& path,
            const time_t modTime,
            const int vol,
            const int oids
    ) : m_path(path), m_modTime(modTime), m_vol(vol), m_oids(oids) {}
};


/// This class provides search capability of (integer) OIDs keyed to
/// (string) accessions, stored in a SQLite database.
class NCBI_XOBJREAD_EXPORT CSeqDBSqlite
{
private:
    CSQLITE_Connection* m_db = NULL;
    CSQLITE_Statement* m_selectStmt = NULL;

public:
    static const int kNotFound;     /// accession not found in database
    static const int kAmbiguous;    /// more than one row found with same
                                    ///  accession and (highest) version
                                    ///  but different OIDs

    /// Constructor
    /// @param dbname Database file name
    CSeqDBSqlite(const string& dbname);

    /// Destructor
    ~CSeqDBSqlite();

    /// Set SQLite cache size.
    /// @param cache_size Cache size in bytes
    void SetCacheSize(const size_t cache_size);

    /// Get OID for single string accession.
    /// If more than one match to the accession is found,
    /// the OID of the one with the highest version number will
    /// be returned.
    /// If there are multiple instances of the accession with the same highest
    /// version number but different OIDs, kAmbiguous (-2) will be returned
    /// instead of the OID.
    /// @param accession String accession (without version suffix)
    /// @return OID >= 0 if found, kNotFound (-1) if not found,
    /// kAmbiguous (-2) if ambiguous
    /// @see GetOids
    /// @see kNotFound
    /// @see kAmbiguous
    int GetOid(const string& accession);

    /// Get OIDs for a list of string accessions.
    /// Returned vector of OIDs will have the same length as accessions;
    /// returned OID values will be as described for GetOid.
    /// @param accessions list of string accessions
    /// @return vector of OIDs, one per accession
    /// @see GetOid
    vector<int> GetOids(const list<string>& accessions);

    /// Get accessions for a single OID.
    /// OID to accession is one-to-many, so a single OID can match
    /// zero, one, or more than one accession.
    list<string> GetAccessions(const int oid);

    /// Step through all accession-to-OID rows.
    /// Will lazily execute "SELECT * ..." upon first call.
    /// If any argument is NULL, that value will not be returned.
    /// If all arguments are NULL, only 'found' (true) or 'done' (false)
    /// will be returned.
    /// When stepping is done, this method will finalize the SELECT statement.
    ///
    /// NOTE: Stepping should be repeated until this method returns false,
    /// so that the SELECT statement can be finalized and deleted.
    ///
    /// @param acc pointer to string for accession, or NULL
    /// @param ver pointer to int for version, or NULL
    /// @param oid pointer to int for OID, or NULL
    /// @return true if row is found, or false if all rows have been returned
    bool StepAccessions(string* acc, int* ver, int* oid);

    /// Step through all volumes.
    /// Will lazily execute "SELECT * ..." upon first call.
    /// If any argument is NULL, that value will not be returned.
    /// If all arguments are NULL, only 'found' (true) or 'done' (false)
    /// will be returned.
    /// When stepping is done, this method will finalize the SELECT statement.
    /// Modification time is identical to time_t in Standard C Library,
    /// which is the number of seconds since the UNIX Epoch.
    /// This can be converted to human-readable form with C functions
    /// such as ctime().
    ///
    /// NOTE: Stepping should be repeated until this method returns false,
    /// so that the SELECT statement can be finalized and deleted.
    ///
    /// @param path pointer to string for path, or NULL
    /// @param modtime pointer to int for modification time, or NULL
    /// @param volume pointer to int for volume number, or NULL
    /// @param numoids pointer to int for number of OIDS in volume, or NULL
    /// @return true if row is found, or false if all rows have been returned
    bool StepVolumes(string* path, int* modtime, int* volume, int* numoids);
};

END_NCBI_SCOPE

#endif
