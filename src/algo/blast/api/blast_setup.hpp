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
* File Description:
*   Auxiliary setup functions for Blast objects interface
*
*/

#ifndef ALGO_BLAST_API___BLAST_SETUP__HPP
#define ALGO_BLAST_API___BLAST_SETUP__HPP

#include <algo/blast/api/blast_types.hpp>
#include <algo/blast/core/blast_options.h>

BEGIN_NCBI_SCOPE

BEGIN_SCOPE(objects)
    class CSeq_loc;
    class CScope;
END_SCOPE(objects)

BEGIN_SCOPE(blast)
class CBlastOption;

void
SetupQueryInfo(const TSeqLocVector& queries, const CBlastOption& options, 
               BlastQueryInfo** qinfo); // out

void
SetupQueries(const TSeqLocVector& queries, const CBlastOption& options,
             const CBlastQueryInfo& qinfo, BLAST_SequenceBlk** seqblk);

void
SetupSubjects(const TSeqLocVector& subjects, 
              CBlastOption* options,
              vector<BLAST_SequenceBlk*>* seqblk_vec, 
              unsigned int* max_subjlen);

/** Retrieves a sequence using the object manager
 * @param sl seqloc of the sequence to obtain [in]
 * @param encoding encoding for the sequence retrieved [in]
 * @param scope Scope from which the sequences are retrieved [in]
 * @param strand strand to retrieve [in]
 * @param add_nucl_sentinel true to guard nucleotide sequence with sentinel 
 * bytes (ignored for protein sequences, which always have sentinels) [in]
 * @return pair containing the buffer and its length. 
 */
pair<AutoPtr<Uint1, CDeleter<Uint1> >, int>
GetSequence(const objects::CSeq_loc& sl, Uint1 encoding, 
            objects::CScope* scope, 
            objects::ENa_strand strand = objects::eNa_strand_plus, 
            bool add_nucl_sentinel = true);

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

/** Retrieves the requested genetic code in Ncbistdaa format. 
 * @param genetic_code numeric identifier for genetic code requested [in]
 */
AutoPtr<Uint1, ArrayDeleter<Uint1> >
FindGeneticCode(int genetic_code);

/** Returns the path (including a trailing path separator) to the location
 * where the matrix can be found.
 * @param matrix_name matrix to search for
 * @param is_prot true if this is a protein matrix
 */
string
FindMatrixPath(const char* matrix_name, bool is_prot);

/** Returns the number of frames for a given BLAST program
 * @param p program 
 */
unsigned int 
GetNumberOfFrames(EProgram p);

END_SCOPE(blast)
END_NCBI_SCOPE

/*
* ===========================================================================
*
* $Log$
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
