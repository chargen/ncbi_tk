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
 * Author:  Christiam Camacho
 *
 */

/** @file blast_setup.hpp
 * Internal auxiliary setup classes/functions for C++ BLAST APIs.
 * These facilities are free of any dependencies on the NCBI C++ object
 * manager.
 */

#ifndef ALGO_BLAST_API___BLAST_SETUP__HPP
#define ALGO_BLAST_API___BLAST_SETUP__HPP

#include <algo/blast/api/blast_aux.hpp>
#include <algo/blast/core/blast_options.h>
#include <algo/blast/api/blast_exception.hpp>
#include <algo/blast/api/blast_types.hpp>

// Object includes
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Na_strand.hpp>
#include <objects/seq/Seq_data.hpp>

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(blast)
class CBlastOptions;

/// Structure to store sequence data and its length for use in the CORE
/// of BLAST (it's a malloc'ed array of Uint1 and its length)
/// FIXME: do not confuse with blast_seg.c's SSequence
struct SBlastSequence {
    // AutoPtr<Uint1, CDeleter<Uint1> > == TAutoUint1Ptr
    TAutoUint1Ptr   data;       /**< Sequence data */
    TSeqPos         length;     /**< Length of the buffer above (not
                                  necessarily sequence length!) */

    /** Default constructor */
    SBlastSequence()
        : data(NULL), length(0) {}

    /** Allocates a sequence buffer of the specified length
     * @param buf_len number of bytes to allocate [in]
     */
    SBlastSequence(TSeqPos buf_len)
        : data((Uint1*)calloc(buf_len, sizeof(Uint1))), length(buf_len)
    {
        if ( !data ) {
            NCBI_THROW(CBlastSystemException, eOutOfMemory, 
               "Failed to allocate " + NStr::IntToString(buf_len) + " bytes");
        }
    }

    /** Parametrized constructor 
     * @param d buffer containing sequence data [in]
     * @param l length of buffer above [in]
     */
    SBlastSequence(Uint1* d, TSeqPos l)
        : data(d), length(l) {}
};

/// Allows specification of whether sentinel bytes should be used or not
enum ESentinelType {
    eSentinels,         ///< Use sentinel bytes
    eNoSentinels        ///< Do not use sentinel bytes
};

/// Lightweight wrapper around an indexed sequence container. These sequences
/// are then used to set up internal BLAST data structures for sequence data
class IBlastQuerySource : public CObject 
{
public:
    /// Our no-op virtual destructor
    virtual ~IBlastQuerySource() {}
    
    /// Return strand for a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual objects::ENa_strand GetStrand(int index) const = 0;
    
    /// Return the number of elements in the sequence container
    virtual TSeqPos Size() const = 0;

    /// Returns true if the container is empty, else false
    bool Empty() const { return (Size() == 0); }
    
    /// Return the filtered (masked) regions for a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual CConstRef<objects::CSeq_loc> GetMask(int index) = 0;
    
    /// Return the filtered (masked) regions for a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual TMaskedQueryRegions GetMaskedRegions(int index) = 0;
    
    /// Return the CSeq_loc associated with a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual CConstRef<objects::CSeq_loc> GetSeqLoc(int index) const = 0;

    /// Return the sequence identifier associated with a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual const objects::CSeq_id* GetSeqId(int index) const = 0;
    
    /// Return the sequence data for a sequence
    /// @param index index of the sequence in the sequence container [in]
    /// @param encoding desired encoding [in]
    /// @param strand strand to fetch [in]
    /// @param sentinel specifies to use or not to use sentinel bytes around
    ///        sequence data. Note that this is ignored for proteins, as in the
    ///        CORE of BLAST, proteins always have sentinel bytes [in]
    /// @param warnings if not NULL, warnings will be returned in this string
    ///        [in|out]
    /// @return SBlastSequence structure containing sequence data requested
    virtual SBlastSequence
    GetBlastSequence(int index, EBlastEncoding encoding, 
                     objects::ENa_strand strand, ESentinelType sentinel, 
                     std::string* warnings = 0) const = 0;
    
    /// Return the length of a sequence
    /// @param index index of the sequence in the sequence container [in]
    virtual TSeqPos GetLength(int index) const = 0;
};

/// Choose between a Seq-loc specified query strand and the strand obtained
/// from the CBlastOptions
/// @param query_seqloc Seq-loc corresponding to a given query sequence [in]
/// @param program program type from the CORE's point of view [in]
/// @param strand_option strand as specified by the BLAST options [in]
objects::ENa_strand
BlastSetup_GetStrand(const objects::CSeq_loc& query_seqloc,
                     EBlastProgramType program,
                     objects::ENa_strand strand_option);

/// Lightweight wrapper around sequence data which provides a CSeqVector-like
/// interface to the data
class IBlastSeqVector {
public:
    /// Our no-op virtual destructor
    virtual ~IBlastSeqVector() {}

    /// Sets the encoding for the sequence data.
    /// Two encodings are really necessary: ncbistdaa and ncbi4na, both use 1
    /// byte per residue/base
    virtual void SetCoding(objects::CSeq_data::E_Choice coding) = 0;
    /// Returns the length of the sequence data (in the case of nucleotides,
    /// only one strand)
    /// @throws CBlastException if the size returned is 0
    TSeqPos size() const {
        TSeqPos retval = x_Size();
        if (retval == 0) {
            NCBI_THROW(CBlastException, eInvalidArgument, 
                       "Sequence with 0 length");
        }
        return retval;
    }
    /// Allows index-based access to the sequence data
    virtual Uint1 operator[] (TSeqPos pos) const = 0;

    /// For nucleotide sequences this instructs the implementation to convert
    /// its representation to be that of the plus strand
    void SetPlusStrand() {
        x_SetPlusStrand();
        m_Strand = objects::eNa_strand_plus;
    }
    /// For nucleotide sequences this instructs the implementation to convert
    /// its representation to be that of the minus strand
    void SetMinusStrand() {
        x_SetMinusStrand();
        m_Strand = objects::eNa_strand_minus;
    }
    /// Accessor for the strand currently set
    objects::ENa_strand GetStrand() const {
        return m_Strand;
    }
    /// Returns the compressed nucleotide data for the plus strand, still
    /// occupying one base per byte.
    virtual SBlastSequence GetCompressedPlusStrand() = 0;

protected:
    /// Method which retrieves the size of the sequence vector, as described in
    /// the size() method above
    virtual TSeqPos x_Size() const = 0;
    /// Method which does the work for setting the plus strand of the 
    /// nucleotide sequence data
    virtual void x_SetPlusStrand() = 0;
    /// Method which does the work for setting the minus strand of the 
    /// nucleotide sequence data
    virtual void x_SetMinusStrand() = 0;

    /// Maintains the state of the strand currently saved by the implementation
    /// of this class
    objects::ENa_strand m_Strand;
};

/** ObjMgr Free version of SetupQueryInfo.
 * NB: effective length will be assigned inside the engine.
 * @param queries Vector of query locations [in]
 * @param prog program type from the CORE's point of view [in]
 * @param strand_opt Unless the strand option is set to single strand, the 
 * actual CSeq_locs in the TSeqLocVector dictacte which strand to use
 * during the search [in]
 * @param qinfo Allocated query info structure [out]
 */
void
SetupQueryInfo_OMF(const IBlastQuerySource& queries,
                   EBlastProgramType prog,
                   objects::ENa_strand strand_opt,
                   BlastQueryInfo** qinfo);

/// ObjMgr Free version of SetupQueries.
/// @param queries vector of blast::SSeqLoc structures [in]
/// @param qinfo BlastQueryInfo structure to obtain context information [in]
/// @param seqblk Structure to save sequence data, allocated in this 
/// function [out]
/// @param messages object to save warnings/errors for all queries [out]
/// @param prog program type from the CORE's point of view [in]
/// @param strand_opt Unless the strand option is set to single strand, the 
/// actual CSeq_locs in the TSeqLocVector dictacte which strand to use
/// during the search [in]
/// @param genetic_code genetic code string as returned by
/// blast::FindGeneticCode()

void
SetupQueries_OMF(IBlastQuerySource& queries,
                 BlastQueryInfo* qinfo, 
                 BLAST_SequenceBlk** seqblk,
                 EBlastProgramType prog, 
                 objects::ENa_strand strand_opt,
                 const Uint1* genetic_code,
                 TSearchMessages& messages);

/** Object manager free version of SetupSubjects
 * @param subjects Vector of subject locations [in]
 * @param program BLAST program [in]
 * @param seqblk_vec Vector of subject sequence data structures [out]
 * @param max_subjlen Maximal length of the subject sequences [out]
 */
void
SetupSubjects_OMF(IBlastQuerySource& subjects,
                  EBlastProgramType program,
                  vector<BLAST_SequenceBlk*>* seqblk_vec,
                  unsigned int* max_subjlen);

/** Object manager free version of GetSequence */
SBlastSequence
GetSequence_OMF(IBlastSeqVector& sv, EBlastEncoding encoding, 
            objects::ENa_strand strand, 
            ESentinelType sentinel,
            std::string* warnings = 0);

/** Calculates the length of the buffer to allocate given the desired encoding,
 * strand (if applicable) and use of sentinel bytes around sequence.
 * @param sequence_length Length of the sequence [in]
 * @param encoding Desired encoding for calculation (supported encodings are
 *        listed in GetSequence()) [in]
 * @param strand Which strand to use for calculation [in]
 * @param sentinel Whether to include or not sentinels in calculation. Same
 *        criteria as GetSequence() applies [in]
 * @return Length of the buffer to allocate to contain original sequence of
 *        length sequence_length for given encoding and parameter constraints.
 *        If the sequence_length is 0, the return value will be 0 too
 * @throws CBlastException in case of unsupported encoding
 */
TSeqPos
CalculateSeqBufferLength(TSeqPos sequence_length, EBlastEncoding encoding,
                         objects::ENa_strand strand =
                         objects::eNa_strand_unknown,
                         ESentinelType sentinel = eSentinels)
                         THROWS((CBlastException));

/// Compresses the sequence data passed in to the function from 1 base per byte
/// to 4 bases per byte
/// @param source input sequence data in ncbi2na format, with ambiguities
/// randomized [in]
/// @return compressed version of the input
/// @throws CBlastException in case of memory allocation failure
/// @todo use CSeqConvert::Pack?
SBlastSequence CompressNcbi2na(const SBlastSequence& source);

/** Convenience function to centralize the knowledge of which sentinel bytes we
 * use for supported encodings. Note that only eBlastEncodingProtein,
 * eBlastEncodingNucleotide, and eBlastEncodingNcbi4na support sentinel bytes, 
 * any other values for encoding will cause an exception to be thrown.
 * @param encoding Encoding for which a sentinel byte is needed [in]
 * @return sentinel byte
 * @throws CBlastException in case of unsupported encoding
 */
Uint1 GetSentinelByte(EBlastEncoding encoding) THROWS((CBlastException));

#if 0
// not used right now
/** Translates nucleotide query sequences to protein in the requested frame
 * @param nucl_seq forward (plus) strand of the nucleotide sequence [in]
 * @param nucl_seq_rev reverse (minus) strand of the nucleotide sequence [in]
 * @param nucl_length length of a single strand of the nucleotide sequence [in]
 * @param frame frame to translate, allowed values: 1,2,3,-1,-2,-3 [in]
 * @param translation buffer to hold the translation, should be allocated
 * outside this function [out]
 */
void
BLASTGetTranslation(const Uint1* nucl_seq, const Uint1* nucl_seq_rev, 
                    const int nucl_length, const short frame, Uint1* translation);
#endif

/** Returns the path (including a trailing path separator) to the location
 * where the BLAST database can be found.
 * @param dbname Database to search for
 * @param is_prot true if this is a protein matrix
 */
string
FindBlastDbPath(const char* dbname, bool is_prot);

/** Returns the number of contexts for a given BLAST program
 * @sa BLAST_GetNumberOfContexts
 * @param p program 
 */
unsigned int 
GetNumberOfContexts(EBlastProgramType p);


/// Returns the encoding for the sequence data used in BLAST for the query
/// @param program program type [in]
/// @throws CBlastException in case of unsupported program
EBlastEncoding
GetQueryEncoding(EBlastProgramType program);

/// Returns the encoding for the sequence data used in BLAST2Sequences for 
/// the subject
/// @param program program type [in]
/// @throws CBlastException in case of unsupported program
EBlastEncoding
GetSubjectEncoding(EBlastProgramType program);

BLAST_SequenceBlk*
SafeSetupQueries(IBlastQuerySource& queries,
                 const CBlastOptions* options,
                 BlastQueryInfo* query_info,
                 TSearchMessages& messages);

BlastQueryInfo*
SafeSetupQueryInfo(const IBlastQuerySource& queries, 
                   const CBlastOptions* options);


/// Returns the path to a specified matrix.  
/// This is the implementation of the GET_MATRIX_PATH callback. 
///
/// @param matrix_name matrix name (e.g., BLOSUM62) [in]
/// @param is_prot matrix is for proteins if TRUE [in]
/// @return path to matrix, should be deallocated by user.
char*
BlastFindMatrixPath(const char* matrix_name, Boolean is_prot);

/// Collection of BlastSeqLoc lists for filtering processing.
///
/// This class acts as a container for frame values and collections of
/// BlastSeqLoc objects used by the blast filtering processing code.
/// The support for filtering of blastx searches adds complexity and
/// creates more opportunities for errors to occur.  This class was
/// designed to handle some of that complexity, and guard against some
/// of those possible errors.

class CBlastQueryFilteredFrames : public CObject {
public:
    /// Data type for frame value, however inputs to methods use "int"
    /// instead of this type for readability and brevity.
    typedef CSeqLocInfo::ETranslationFrame ETranslationFrame;
    
    /// Construct container for frame values and BlastSeqLocs for the
    /// specified search program.
    /// @param program The type of search being done.
    CBlastQueryFilteredFrames(EBlastProgramType program);
    
    /// Construct container for frame values and BlastSeqLocs from a
    /// TMaskedQueryRegions vector.
    /// @param program Search program value used [in]
    /// @param mqr MaskedQueryRegions to convert [in]
    CBlastQueryFilteredFrames(EBlastProgramType           program,
                              const TMaskedQueryRegions & mqr);
    
    /// Destructor; frees any BlastSeqLoc lists not released by the
    /// caller.
    ~CBlastQueryFilteredFrames();
    
    /// Add a masked interval to the specified frame.
    ///
    /// The specified interval of the specified frame is masked.  This
    /// creates a BlastSeqLoc object inside this container for that
    /// frame, which will be freed at destruction time unless the
    /// client code calls Release() for that frame.
    ///
    /// @param intv The interval to mask.
    /// @param frame The specific frame, expressed as a value from ETranslationFrame, on which this interval falls.
    void AddSeqLoc(const objects::CSeq_interval & intv, int frame);
    
    /// Access the BlastSeqLocs for a given frame.
    ///
    /// A pointer is returned to the list of BlastSeqLocs associated
    /// with a given frame.
    /// @param frame The specific frame, expressed as a value from ETranslationFrame, on which this interval falls.
    BlastSeqLoc ** operator[](int frame);
    
    /// Release the BlastSeqLocs for a given frame.
    ///
    /// The given frame is cleared (the data removed) without freeing
    /// the associated objects.  The calling code takes responsibility
    /// for freeing the associated list of objects.
    /// @param frame The specific frame, expressed as a value from ETranslationFrame, on which this interval falls.
    void Release(int frame);
    
    /// Check whether the query is multiframe for this type of search.
    bool QueryHasMultipleFrames() const;
    
    /// Returns the list of frame values for which this object
    /// contains masking information.
    vector<ETranslationFrame> ListFrames() const;
    
    /// Returns true if this object contains any masking information.
    bool Empty() const;
    
    /// Adjusts all stored masks from nucleotide to protein offsets.
    ///
    /// Values stored here must be converted to protein offsets after
    /// a certain stage of processing.  This method only has an effect
    /// for types of searches that need this service (which are those
    /// searches where the query sequence is translated.)  Additional
    /// calls to this method will have no effect.
    ///
    /// @param dna_length The query length in nucleotide bases.
    void UseProteinCoords(TSeqPos dna_length);
    
private:
    /// Prevent copy construction.
    CBlastQueryFilteredFrames(CBlastQueryFilteredFrames & f);

    /// Prevent assignment.
    CBlastQueryFilteredFrames & operator=(CBlastQueryFilteredFrames & f);
    
    /// Verify the specified frame value.
    void x_VerifyFrame(int frame);
    
    /// Returns true if this program needs coordinate translation.
    bool x_NeedsTrans();
    
    /// The type of search being done.
    EBlastProgramType m_Program;
    
    /// Frame and BlastSeqLoc* info type.
    typedef map<ETranslationFrame, BlastSeqLoc*> TFrameSet;
    
    /// Frame and BlastSeqLoc* data.
    TFrameSet m_Seqlocs;
    
    /// True if this object's masked regions store DNA coordinates
    /// that will later be translated into protein coordinates.
    bool m_TranslateCoords;
};


END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */

/*
* ===========================================================================
*
* $Log$
* Revision 1.64  2006/09/01 16:46:52  camacho
* + BlastSetup_GetStrand to consolidate assignment of strand obtained from a Seq-loc and CBlastOptions
*
* Revision 1.63  2006/06/05 13:28:39  madden
* Add BlastFindMatrixPath, remove FindMatrixOrPath
*
* Revision 1.62  2006/05/24 17:20:50  madden
* Replace FindMatrixPath with FindMatrixOrPath
*
* Revision 1.61  2006/05/12 18:00:08  camacho
* Minor
*
* Revision 1.60  2006/02/27 15:43:47  camacho
* Fixed bug in CBlastQuerySourceOM::GetMaskedRegions.
* Made IBlastQuerySource::GetMaskedRegions a non-const method.
*
* Revision 1.59  2006/02/22 18:34:17  bealer
* - Blastx filtering support, CBlastQueryVector class.
*
* Revision 1.58  2006/01/24 15:24:30  camacho
* 1. + IBlastQuerySource::GetSeqId
* 2. IBlastQuerySource::GetMask is not a const method anymore
* 3. Remove const type qualifier for IBlastQuerySource argument to
*    SetupQueries_OMF and SetupSubjects_OMF
*
* Revision 1.57  2006/01/12 20:39:22  camacho
* Removed const from BlastQueryInfo argument to functions (to allow setting of context-validity flag)
*
* Revision 1.56  2005/12/23 16:27:25  camacho
* doxygen fixes
*
* Revision 1.55  2005/12/16 20:51:18  camacho
* Diffuse the use of CSearchMessage, TQueryMessages, and TSearchMessages
*
* Revision 1.54  2005/11/15 22:43:47  camacho
* Fix comment
*
* Revision 1.53  2005/09/28 18:23:08  camacho
* Rearrangement of headers/functions to segregate object manager dependencies.
*
* Revision 1.52  2005/09/26 16:22:52  camacho
* IBlastQuerySource is now a subclass of CObject.
* Removed unneeded forward declaration.
*
* Revision 1.51  2005/09/02 15:58:14  camacho
* Rename GetNumberOfFrames -> GetNumberOfContexts, delegate to CORE function
*
* Revision 1.50  2005/07/07 16:32:11  camacho
* Revamping of BLAST exception classes and error codes
*
* Revision 1.49  2005/07/06 17:47:50  camacho
* Doxygen and other minor fixes
*
* Revision 1.48  2005/06/23 16:18:46  camacho
* Doxygen fixes
*
* Revision 1.47  2005/06/22 21:03:02  camacho
* Doxygen comments
*
* Revision 1.46  2005/06/20 17:32:47  camacho
* Add blast::GetSequence object manager-free interface
*
* Revision 1.45  2005/06/10 18:38:49  camacho
* Doxygen fixes
*
* Revision 1.44  2005/06/10 18:08:36  camacho
* Document IBlastQuerySource
*
* Revision 1.43  2005/06/10 14:55:54  camacho
* Give warnings argument in IBlastQuerySource::GetBlastSequence default value
*
* Revision 1.42  2005/06/09 20:34:52  camacho
* Object manager dependent functions reorganization
*
* Revision 1.41  2005/06/08 19:20:48  camacho
* Minor change in SetupQueries
*
* Revision 1.40  2005/06/01 15:51:59  camacho
* doxygen fix
*
* Revision 1.39  2005/05/24 21:10:43  camacho
* Add SBlastSequence constructor
*
* Revision 1.38  2005/05/24 20:02:42  camacho
* Changed signature of SetupQueries and SetupQueryInfo
*
* Revision 1.37  2005/05/10 21:23:59  camacho
* Fix to prior commit
*
* Revision 1.36  2005/05/10 16:08:39  camacho
* Changed *_ENCODING #defines to EBlastEncoding enumeration
*
* Revision 1.35  2005/04/06 21:06:18  dondosha
* Use EBlastProgramType instead of EProgram in non-user-exposed functions
*
* Revision 1.34  2005/03/04 16:53:27  camacho
* more doxygen fixes
*
* Revision 1.33  2005/03/04 16:07:05  camacho
* doxygen fixes
*
* Revision 1.32  2005/03/02 14:25:58  camacho
* Removed unneeded NCBI_XBLAST_EXPORT
*
* Revision 1.31  2005/03/01 20:52:42  camacho
* Doxygen fixes
*
* Revision 1.30  2005/01/21 17:38:36  camacho
* Update documentation
*
* Revision 1.29  2005/01/06 16:07:55  camacho
* + warnings output parameter to blast::GetSequence
*
* Revision 1.28  2005/01/06 15:41:20  camacho
* Add Blast_Message output parameter to SetupQueries
*
* Revision 1.27  2004/12/28 16:47:43  camacho
* 1. Use typedefs to AutoPtr consistently
* 2. Remove exception specification from blast::SetupQueries
* 3. Use SBlastSequence structure instead of std::pair as return value to
*    blast::GetSequence
*
* Revision 1.26  2004/08/16 19:48:30  dondosha
* Removed duplicate declaration of GetProgramFromBlastProgramType
*
* Revision 1.25  2004/08/11 14:24:50  camacho
* Move FindGeneticCode
*
* Revision 1.24  2004/08/11 11:56:48  ivanov
* Added export specifier NCBI_XBLAST_EXPORT
*
* Revision 1.23  2004/07/08 20:19:29  gorelenk
* Temporary added export spec to GetProgramFromBlastProgramType
*
* Revision 1.22  2004/07/06 15:49:30  dondosha
* Added GetProgramFromBlastProgramType function
*
* Revision 1.21  2004/05/19 14:52:01  camacho
* 1. Added doxygen tags to enable doxygen processing of algo/blast/core
* 2. Standardized copyright, CVS $Id string, $Log and rcsid formatting and i
*    location
* 3. Added use of @todo doxygen keyword
*
* Revision 1.20  2004/04/06 20:45:28  dondosha
* Added function FindBlastDbPath: should be moved to seqdb library
*
* Revision 1.19  2004/03/15 19:57:52  dondosha
* SetupSubjects takes just program argument instead of CBlastOptions*
*
* Revision 1.18  2004/03/06 00:39:47  camacho
* Some refactorings, changed boolen parameter to enum in GetSequence
*
* Revision 1.17  2003/12/29 17:03:47  camacho
* Added documentation to GetSequence
*
* Revision 1.16  2003/10/29 04:45:58  camacho
* Use fixed AutoPtr for GetSequence return value
*
* Revision 1.15  2003/09/12 17:52:42  camacho
* Stop using pair<> as return value from GetSequence
*
* Revision 1.14  2003/09/11 20:55:01  camacho
* Temporary fix for AutoPtr return value
*
* Revision 1.13  2003/09/11 17:45:03  camacho
* Changed CBlastOption -> CBlastOptions
*
* Revision 1.12  2003/09/10 04:25:28  camacho
* Minor change to return type of GetSequence
*
* Revision 1.11  2003/09/09 15:57:23  camacho
* Fix indentation
*
* Revision 1.10  2003/09/09 12:57:15  camacho
* + internal setup functions, use smart pointers to handle memory mgmt
*
* Revision 1.9  2003/08/28 22:43:02  camacho
* Change BLASTGetSequence signature
*
* Revision 1.8  2003/08/19 13:45:21  dicuccio
* Removed 'USING_SCOPE(objects)'.  Changed #include guards to be standards
* compliant.  Added 'objects::' where necessary.
*
* Revision 1.7  2003/08/18 20:58:56  camacho
* Added blast namespace, removed *__.hpp includes
*
* Revision 1.6  2003/08/11 19:55:04  camacho
* Early commit to support query concatenation and the use of multiple scopes.
* Compiles, but still needs work.
*
* Revision 1.5  2003/08/11 13:58:51  dicuccio
* Added export specifiers.  Fixed problem with unimplemented private copy ctor
* (truly make unimplemented)
*
* Revision 1.4  2003/08/01 22:35:02  camacho
* Added function to get matrix path (fixme)
*
* Revision 1.3  2003/07/30 15:00:01  camacho
* Do not use Malloc/MemNew/MemFree
*
* Revision 1.2  2003/07/23 21:29:06  camacho
* Update BLASTFindGeneticCode to get genetic code string with C++ toolkit
*
* Revision 1.1  2003/07/10 18:34:19  camacho
* Initial revision
*
*
* ===========================================================================
*/

#endif  /* ALGO_BLAST_API___BLAST_SETUP__HPP */
