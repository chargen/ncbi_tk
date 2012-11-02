/*  $Id$
 * =========================================================================
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
 * =========================================================================
 *
 * Author:  Jie Chen
 *
 * File Description:
 *   Cpp Discrepany Report configuration
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <connect/ncbi_core_cxx.hpp>

// Objects includes
#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seqfeat/Seq_feat.hpp>

// Object Manager includes
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/feat_ci.hpp>
#include <objmgr/align_ci.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

#include <serial/objistr.hpp>
#include <serial/serial.hpp>

#include <common/test_assert.h>

#include "hchecking_class.hpp"
#include "hauto_disc_class.hpp"
#include "hDiscRep_config.hpp"

using namespace ncbi;
using namespace objects;
using namespace DiscRepNmSpc;

static CDiscRepInfo thisInfo;
static CRef <CRuleProperties> rule_prop (new CRuleProperties);
static string       strtmp;

vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_aa;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_na;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_CFeat;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_CFeat_NotInGenProdSet;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_NotInGenProdSet;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_Bioseq_CFeat_CSeqdesc;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_GenProdSetFeat;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_SeqFeat;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_SeqEntry;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_SeqEntry_feat_desc;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_4_once;
vector < CRef <CTestAndRepData> > CRepConfig :: tests_on_BioseqSet;

CRepConfig* CRepConfig :: factory(const string& report_tp)
{
   if (report_tp == "Discrepancy") return new CRepConfDiscrepancy();
   else if(report_tp ==  "Oncaller") return new  CRepConfOncaller();
   else NCBI_THROW(CException, eUnknown, "Unrecognized report type.");
};  // CRepConfig::factory()



void CRepConfig :: Init()
{
   tests_on_Bioseq.reserve(50);
   tests_on_Bioseq_aa.reserve(50);
   tests_on_Bioseq_na.reserve(50);
   tests_on_Bioseq_CFeat.reserve(50);
   tests_on_Bioseq_NotInGenProdSet.reserve(50);
   tests_on_Bioseq_CFeat_CSeqdesc.reserve(50);
   tests_on_GenProdSetFeat.reserve(50);
   tests_on_SeqEntry.reserve(50);
   tests_on_SeqEntry_feat_desc.reserve(50);
   tests_on_BioseqSet.reserve(50);
  
}; // Init()



void CRepConfDiscrepancy :: ConfigRep()
{
//tests_on_Bioseq
   tests_on_Bioseq.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_COUNT_NUCLEOTIDES));
   tests_on_Bioseq.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_QUALITY_SCORES));
   tests_on_Bioseq.push_back(CRef <CTestAndRepData> (new CBioseq_JOINED_FEATURES));
   // oncaller tool version
   // tests_on_Bioseq.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_FEATURE_COUNT));
   thisInfo.test_item_list["DISC_QUALITY_SCORES"].clear();
   
// tests_on_Bioseq_aa
   tests_on_Bioseq_aa.push_back(CRef <CTestAndRepData>(new CBioseq_MISSING_PROTEIN_ID));
   tests_on_Bioseq_aa.push_back(
                       CRef <CTestAndRepData>(new CBioseq_INCONSISTENT_PROTEIN_ID_PREFIX));

// tests_on_Bioseq_na
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_TEST_DEFLINE_PRESENT));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_N_RUNS));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_N_RUNS_14));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_ZERO_BASECOUNT));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_TEST_LOW_QUALITY_REGION));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_PERCENT_N));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_10_PERCENTN));
   tests_on_Bioseq_na.push_back(CRef <CTestAndRepData>(new CBioseq_TEST_UNUSUAL_NT));

// tests_on_Bioseq_CFeat
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(
                                         new CBioseq_SHOW_TRANSL_EXCEPT));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(
                                         new CBioseq_MRNA_SHOULD_HAVE_PROTEIN_TRANSCRIPT_IDS));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_RRNA_NAME_CONFLICTS));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_EXTRA_MISSING_GENES));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_OVERLAPPING_CDS));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_RNA_CDS_OVERLAP));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_FIND_OVERLAPPED_GENES));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_OVERLAPPING_GENES));
   tests_on_Bioseq_CFeat.push_back(
                   CRef <CTestAndRepData>( new CBioseq_EC_NUMBER_ON_HYPOTHETICAL_PROTEIN));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_RNA_NO_PRODUCT));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_SHORT_INTRON));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_BAD_GENE_STRAND));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_SHORT_RRNA));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_TEST_OVERLAPPING_RRNAS));
   tests_on_Bioseq_CFeat.push_back(
                  CRef <CTestAndRepData>( new CBioseq_HYPOTHETICAL_CDS_HAVING_GENE_NAME));
   tests_on_Bioseq_CFeat.push_back(
                  CRef <CTestAndRepData>( new CBioseq_DISC_SUSPICIOUS_NOTE_TEXT));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(new CBioseq_NO_ANNOTATION));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(
                                                         new CBioseq_DISC_LONG_NO_ANNOTATION));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(
                                                         new CBioseq_DISC_PARTIAL_PROBLEMS));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(
                                                         new CBioseq_TEST_UNUSUAL_MISC_RNA));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(
                                                         new CBioseq_GENE_PRODUCT_CONFLICT));
   tests_on_Bioseq_CFeat.push_back( CRef <CTestAndRepData>(
                                                         new CBioseq_DISC_CDS_WITHOUT_MRNA));
   //tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_SUSPECT_PRODUCT_NAMES));

// tests_on_Bioseq_CFeat_NotInGenProdSet
   tests_on_Bioseq_CFeat_NotInGenProdSet.push_back(
                                   CRef <CTestAndRepData>(new CBioseq_DUPLICATE_GENE_LOCUS));
   tests_on_Bioseq_CFeat_NotInGenProdSet.push_back(
                                   CRef <CTestAndRepData>(new CBioseq_LOCUS_TAGS));
   tests_on_Bioseq_CFeat_NotInGenProdSet.push_back(
                               CRef <CTestAndRepData>(new CBioseq_FEATURE_LOCATION_CONFLICT));

// tests_on_Bioseq_NotInGenProdSet
   tests_on_Bioseq_NotInGenProdSet.push_back(CRef<CTestAndRepData>(new CBioseq_SHORT_CONTIG));
   tests_on_Bioseq_NotInGenProdSet.push_back(
                                     CRef<CTestAndRepData>(new CBioseq_SHORT_SEQUENCES));
   tests_on_Bioseq_NotInGenProdSet.push_back(
                                     CRef<CTestAndRepData>(new CBioseq_SHORT_SEQUENCES_200));

// tests_on_Bioseq_CFeat_CSeqdesc
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(
                                           new CBioseq_DISC_BACTERIA_SHOULD_NOT_HAVE_MRNA));
   tests_on_Bioseq_CFeat_CSeqdesc.push_back(
                                 CRef <CTestAndRepData> (new CBioseq_TEST_BAD_GENE_NAME));
   tests_on_Bioseq_CFeat_CSeqdesc.push_back(
                                 CRef <CTestAndRepData> (new CBioseq_MOLTYPE_NOT_MRNA));
   tests_on_Bioseq_CFeat_CSeqdesc.push_back(
                                 CRef <CTestAndRepData> (new CBioseq_TECHNIQUE_NOT_TSA));
   tests_on_Bioseq_CFeat_CSeqdesc.push_back(
                                 CRef <CTestAndRepData> (new CBioseq_SHORT_PROT_SEQUENCES));
   tests_on_Bioseq_CFeat.push_back(CRef <CTestAndRepData>(new CBioseq_DISC_GENE_PARTIAL_CONFLICT));

// tests_on_GenProdSetFeat


// tests_on_SeqEntry
   // asndisc version   
   tests_on_SeqEntry.push_back(CRef <CTestAndRepData>(new CSeqEntry_DISC_FEATURE_COUNT));

// tests_on_SeqEntry_feat_desc: all CSeqEntry_Feat_desc tests need RmvRedundancy
   tests_on_SeqEntry_feat_desc.push_back( 
                   CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_MISSING_STRUCTURED_COMMENTS));
   tests_on_SeqEntry_feat_desc.push_back( 
                      CRef <CTestAndRepData>(new CSeqEntry_DISC_CITSUB_AFFIL_DUP_TEXT));
   tests_on_SeqEntry_feat_desc.push_back( 
                            CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_MULTISRC));
   // asndisc vertion
   tests_on_SeqEntry_feat_desc.push_back(
                       CRef <CTestAndRepData>(new CSeqEntry_DISC_SOURCE_QUALS_ASNDISC));
   tests_on_SeqEntry_feat_desc.push_back(
                          CRef <CTestAndRepData>(new CSeqEntry_DISC_CHECK_AUTH_CAPS));
   tests_on_SeqEntry_feat_desc.push_back(
                       CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_COMMENT_PRESENT));
   tests_on_SeqEntry_feat_desc.push_back(
                 CRef <CTestAndRepData>(new CSeqEntry_DUP_DISC_ATCC_CULTURE_CONFLICT));
   tests_on_SeqEntry_feat_desc.push_back(
     CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_STRAIN_CULTURE_COLLECTION_MISMATCH));
   tests_on_SeqEntry_feat_desc.push_back(
                         CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_DEFLINE_ON_SET));
   tests_on_SeqEntry_feat_desc.push_back(
              CRef <CTestAndRepData>(new CSeqEntry_DISC_BACTERIAL_TAX_STRAIN_MISMATCH));
   tests_on_SeqEntry_feat_desc.push_back(
                   CRef <CTestAndRepData>(new CSeqEntry_DUP_DISC_CBS_CULTURE_CONFLICT));
   tests_on_SeqEntry_feat_desc.push_back(
                          CRef <CTestAndRepData>(new CSeqEntry_INCONSISTENT_BIOSOURCE));
   tests_on_SeqEntry_feat_desc.push_back(
                       CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_BIOPROJECT_ID));
   tests_on_SeqEntry_feat_desc.push_back(
                 CRef <CTestAndRepData>(new CSeqEntry_MISSING_GENOMEASSEMBLY_COMMENTS));
   tests_on_SeqEntry_feat_desc.push_back(
                            CRef <CTestAndRepData>(new CSeqEntry_TEST_HAS_PROJECT_ID));
   tests_on_SeqEntry_feat_desc.push_back(
                         CRef <CTestAndRepData>(new CSeqEntry_DIVISION_CODE_CONFLICTS));
   tests_on_SeqEntry_feat_desc.push_back(
                            CRef <CTestAndRepData>(new CSeqEntry_DISC_REQUIRED_STRAIN));
   tests_on_SeqEntry_feat_desc.push_back(
                CRef <CTestAndRepData>(new CSeqEntry_MORE_OR_SPEC_NAMES_IDENTIFIED_BY));
   tests_on_SeqEntry_feat_desc.push_back(
                       CRef <CTestAndRepData>(new CSeqEntry_MORE_NAMES_COLLECTED_BY));
   tests_on_SeqEntry_feat_desc.push_back(
                       CRef <CTestAndRepData>(new CSeqEntry_MISSING_PROJECT));
   tests_on_SeqEntry_feat_desc.push_back( 
          CRef <CTestAndRepData>( new CSeqEntry_DISC_BACTERIA_SHOULD_NOT_HAVE_ISOLATE));

// tests_on_BioseqSet
   tests_on_BioseqSet.push_back(
                       CRef <CTestAndRepData>(new CBioseqSet_DISC_NONWGS_SETS_PRESENT));

// ini.
   // search function empty?
   const CSuspect_rule_set::Tdata& rules = thisInfo.suspect_rules->Get();
   rule_prop->srch_func_empty.reserve(rules.size());   // necessary for static vector?
   rule_prop->except_empty.reserve(rules.size());
   unsigned i=0;
   ITERATE (CSuspect_rule_set::Tdata, it, rules) {
      rule_prop->srch_func_empty[i] = rule_prop->IsSearchFuncEmpty((*it)->GetFind());
      rule_prop->except_empty[i] = 
       ((*it)->IsSetExcept()) ? rule_prop->IsSearchFuncEmpty((*it)->GetExcept()) : true;
      i++;
   };

// output flags
   thisInfo.output_config.use_flag = true;
  
} // CRepConfDiscrepancy :: configRep



void CRepConfOncaller :: ConfigRep()
{
  tests_on_SeqEntry_feat_desc.push_back(
                           CRef <CTestAndRepData>(new CSeqEntry_ONCALLER_COMMENT_PRESENT));  

} // CRepConfOncaller :: ConfigRep()



void CRepConfAll :: ConfigRep()
{
  CRepConfDiscrepancy rep_disc;
  CRepConfOncaller rep_on;

  rep_disc.ConfigRep();
  rep_on.ConfigRep();
}; // CRepConfAll
  

