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
* Author:  Kamen Todorov
*
* File Description:
*   Demo of alignment building.
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <connect/ncbi_core_cxx.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

#include <test/test_assert.h>

#include <serial/objistr.hpp>
#include <serial/iterator.hpp>

#include <objtools/alnmgr/aln_asn_reader.hpp>
#include <objtools/alnmgr/aln_container.hpp>
#include <objtools/alnmgr/aln_tests.hpp>
#include <objtools/alnmgr/aln_stats.hpp>
#include <objtools/alnmgr/pairwise_aln.hpp>
#include <objtools/alnmgr/aln_converters.hpp>
#include <objtools/alnmgr/aln_rng_coll_oper.hpp> //< Temp, just to test it
#include <objtools/alnmgr/sparse_aln.hpp> //< Temp, just to test it
#include <objtools/alnmgr/aln_builders.hpp>
#include <objtools/alnmgr/aln_user_options.hpp>
using namespace ncbi;
using namespace objects;


class CAlnBuildApp : public CNcbiApplication
{
public:
    virtual void Init         (void);
    virtual int  Run          (void);
    CScope&      GetScope     (void) const;
    void         LoadInputAlns(void);
    bool         InsertAln    (const CSeq_align* aln) {
        m_AlnContainer.insert(*aln);
        aln->Validate(true);
        return true;
    }

private:
    mutable CRef<CObjectManager> m_ObjMgr;
    mutable CRef<CScope>         m_Scope;
    CAlnContainer                m_AlnContainer;
};


void CAlnBuildApp::Init(void)
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->AddDefaultKey
        ("in", "input_file_name",
         "Name of file to read from (standard input by default)",
         CArgDescriptions::eInputFile, "-");

    arg_desc->AddDefaultKey
        ("b", "bin_obj_type",
         "This forces the input file to be read in binary ASN.1 mode\n"
         "and specifies the type of the top-level ASN.1 object.\n",
         CArgDescriptions::eString, "");

    // Merge option
    arg_desc->AddDefaultKey
        ("m", "merge_option",
         "eMergeAllSeqs      = 0, ///< Merge all sequences\n"
         "eQuerySeqMergeOnly = 1, ///< Only put the query seq on same row, \n"
         "ePreserveRows      = 2, ///< Preserve all rows as they were in the input (e.g. self-align a sequence)\n"
         "eDefault           = eMergeAllSeqs",
         CArgDescriptions::eInteger, "0");

    // Program description
    string prog_description = "Alignment build application.\n";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              prog_description, false);

    SetupArgDescriptions(arg_desc.release());
}


void CAlnBuildApp::LoadInputAlns(void)
{
    const CArgs& args = GetArgs();
    string sname = args["in"].AsString();
    
    /// get the asn type of the top-level object
    string asn_type = args["b"].AsString();
    bool binary = !asn_type.empty();
    auto_ptr<CObjectIStream> in
        (CObjectIStream::Open(binary?eSerial_AsnBinary:eSerial_AsnText, sname));
    
    CAlnAsnReader reader(&GetScope());
    reader.Read(in.get(),
                bind1st(mem_fun(&CAlnBuildApp::InsertAln), this),
                asn_type);
}


CScope& CAlnBuildApp::GetScope(void) const
{
    if (!m_Scope) {
        m_ObjMgr = CObjectManager::GetInstance();
        CGBDataLoader::RegisterInObjectManager(*m_ObjMgr);
        
        m_Scope = new CScope(*m_ObjMgr);
        m_Scope->AddDefaults();
    }
    return *m_Scope;
}


int CAlnBuildApp::Run(void)
{
    // Setup application registry, error log, and MT-lock for CONNECT library
    CONNECT_Init(&GetConfig());

    LoadInputAlns();


    /// Types we use here:
    typedef CSeq_align::TDim TDim;
    typedef vector<const CSeq_align*> TAlnVector;
    typedef const CSeq_id* TSeqIdPtr;
    typedef vector<TSeqIdPtr> TSeqIdVector;
    typedef SCompareOrdered<TSeqIdPtr> TComp;
    typedef CAlnSeqIdVector<TAlnVector, TComp> TAlnSeqIdVector;
    typedef CSeqIdAlnBitmap<TAlnSeqIdVector> TSeqIdAlnBitmap;
    typedef CAlnStats<TAlnVector, TSeqIdVector, TAlnSeqIdVector> TAlnStats;


    /// Create a vector of alignments based on m_AlnContainer
    TAlnVector aln_vector(m_AlnContainer.size());
    aln_vector.assign(m_AlnContainer.begin(), m_AlnContainer.end());


    /// Create a comparison functor
    TComp comp;


    /// Create a vector of seq-ids per seq-align
    TAlnSeqIdVector aln_seq_id_vector(aln_vector, comp);


    /// Create an alignment bitmap to obtain statistics.
    TSeqIdAlnBitmap id_aln_bitmap(aln_seq_id_vector, GetScope());


    /// Crete align statistics object
    TAlnStats aln_stats(aln_vector,
                        aln_seq_id_vector,
                        id_aln_bitmap.GetAnchorRows(),
                        id_aln_bitmap.GetBaseWidths());
    aln_stats.Dump(cout);


    /// Construct a vector of anchored alignments
    typedef vector<CRef<CAnchoredAln> > TAnchoredAlnVector;
    TAnchoredAlnVector anchored_aln_vector;
    CreateAnchoredAlnVector(aln_stats, anchored_aln_vector);


    /// Choose user options
    CAlnUserOptions aln_user_options;
    aln_user_options.m_MergeOption = GetArgs()["m"].AsInteger();


    /// Build a single anchored aln
    CAnchoredAln built_anchored_aln;
    BuildAln(anchored_aln_vector,
             built_anchored_aln,
             aln_user_options,
             comp);
    built_anchored_aln.Dump(cout);


    /// Get sequence:
    CSparseAln sparse_aln(built_anchored_aln, GetScope());
    for (TDim row = 0;  row < sparse_aln.GetDim();  ++row) {
        string sequence;
        sparse_aln.GetAlnSeqString
            (row, 
             sequence, 
             sparse_aln.GetSeqAlnRange(row));
        cout << sparse_aln.GetSeqId(row).AsFastaString() << "\t"
             << sequence << endl;
    }


    return 0;
}


int main(int argc, const char* argv[])
{
    return CAlnBuildApp().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
* ===========================================================================
*
* $Log$
* Revision 1.10  2006/11/22 00:48:43  todorov
* 1) + merge options
* 2) + sequence comparison functor (when building)
*
* Revision 1.9  2006/11/20 18:54:10  todorov
* Simplified code (using supplied methods).
*
* Revision 1.8  2006/11/17 05:32:31  todorov
* Using a separate builder.
*
* Revision 1.7  2006/11/16 22:41:37  todorov
* Truncating chunks by substracting aligment collections.
*
* Revision 1.6  2006/11/16 13:51:10  todorov
* Using some CSparseAln.
*
* Revision 1.5  2006/11/14 20:42:33  todorov
* build without using CDiagRngColl.
*
* Revision 1.4  2006/11/09 00:14:27  todorov
* Working version of building a vector of anchored alignments.
*
* Revision 1.3  2006/11/08 17:42:10  todorov
* Working version extracting complete stats.
*
* Revision 1.2  2006/11/06 19:57:36  todorov
* Added comments.
*
* Revision 1.1  2006/10/19 17:12:41  todorov
* Initial revision.
*
* ===========================================================================
*/
