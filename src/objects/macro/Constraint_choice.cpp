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
 * Author: J. Chen
 *
 * File Description:
 *   Evaluate object match to CConstraint_choice
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'macro.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/macro/Constraint_choice.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CConstraint_choice::~CConstraint_choice(void)
{
}

/*

bool CString_constraint :: ObjectMatch(const CBioSource& biosrc)
{
   vector <string> strs; 
   GetStringsFromObject(biosrc, strs);
   ITERATE (vector <string>, it, strs) {
      if (x_DoesSingleStringMatchConstraint(*it)) {
          strs.clear();
          return true;
      }
   }
   strs.clear();
   return false;
};

bool bool CString_constraint :: ObjectMatch(const CCGPSetData& cgp, const CScope scope)
{
  * CDS-Gene-Prot set *
  bool all_match = true, any_match = false;
  vector <string> strs;
  if (cgp.gene) {
       GetStringsFromObject(*cgp.gene, strs);
       if (x_DoesObjectMatchStringConstraint (*cgp.gene, scope, strs)) {
          any_match = true;
       }
       else {
          any_match = false;
       }
       strs.clear();
  }
  if (cgp.cds && (!any_match || all_match)) {
      GetStringsFromObject(*cgp.cds, strs);
      if (x_DoesObjectMatchStringConstraint( *cgp.cds, scope, strs)) {
         any_match = true;
      }
      else {
         all_match = false;
      }
      strs.clear();
  }
  if (cgp.mrna && (!any_match || all_match)) {
      GetStringsFromObject(*cgp.mrna, strs);
      if (x_DoesObjectMatchStringConstraint( *cgp.mrna, scope, strs)) {
         any_match = true;
      }
      else {
         all_match = false;
      }
      strs.clear();
  }
  if (cgp.prot  && (!any_match || all_match)) {
      GetStringsFromObject(*cgp.prot, strs);
      if (x_DoesObjectMatchStringConstraint( *cgp.prot, scope, strs)) {
         any_match = true;
      }
      else all_match = false;
      strs.clear();
  }
  if (!any_match || all_match) {
     ITERATE (vector <CConstRef <CSeq_feat>, it, cgp.mat_peptide_list) {
        GetStringsFromObject(**it, strs);
        if (x_DoesObjectMatchStringConstraint( **it, scope, strs)) {
           any_match = true;
        }
        else all_match = false;
        strs.clear();
        if (any_match && !all_match) {
            break;
        }
     }
  }

  if (GetNot_present()) {
      return all_match;
  }
  else return any_match;
};
 */

/*
bool CConstraint_choice :: Match(const CSeq_feat& feat, CConstRef <CScope> scope) const
{
  switch (Which()) {
    case e_String :
       return GetString().DoesObjectMatchStringConstraint(feat, scope);
//       return GetString().ObjectMatch(feat, scope, strs_in_feat);
    case CConstraint_choice::e_Location :
       return GetLocation().Match(feat, scope);
    case CConstraint_choice::e_Field :
       return GetField().Match (feat, scope);
    case CConstraint_choice::e_Source :
*
       if (data.GetData().IsBiosrc()) {
          return x_DoesBiosourceMatchConstraint ( data.GetData().GetBiosrc(), 
                                                cons.GetSource());
       }
       else {
          CBioseq_Handle 
             seq_hl = sequence::GetBioseqFromSeqLoc(data.GetLocation(), 
                                                    *thisInfo.scope);
          const CBioSource* src = GetBioSource(seq_hl);
          if (src) {
            return x_DoesBiosourceMatchConstraint(*src, cons.GetSource());
          }
          else return false;
       }
/
       break;
    case CConstraint_choice::e_Cdsgeneprot_qual :
       return x_DoesFeatureMatchCGPQualConstraint (data, 
                                                cons.GetCdsgeneprot_qual());
****
      if (choice == 0) {
        rval = DoesCGPSetMatchQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQDESC) {
        rval = DoesSeqDescMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQFEAT) {
        rval = DoesFeatureMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_BIOSEQ) {
        rval = DoesSequenceMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else {
        rval = FALSE;
      }
****
    case CConstraint_choice::e_Cdsgeneprot_pseudo :
  //     return x_DoesFeatureMatchCGPPseudoConstraint (data, cons.GetCdsgeneprot_pseudo());
*
      if (choice == 0) {
        rval = DoesCGPSetMatchPseudoConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQFEAT) {
        rval = DoesFeatureMatchCGPPseudoConstraint (data, cons->data.ptrvalue);
      }
*
    case CConstraint_choice::e_Sequence :
      {
*
        if (m_bioseq_hl) { 
            return x_DoesSequenceMatchSequenceConstraint(cons.GetSequence());
        }
*
        break;
      }
    case CConstraint_choice::e_Pub:
*
      if (data.GetData().IsPub()) {
         return x_DoesPubMatchPublicationConstraint(data.GetData().GetPub(), cons.GetPub());
      }
*
      break;
    case CConstraint_choice::e_Molinfo:
 //      return x_DoesObjectMatchMolinfoFieldConstraint (data, cons.GetMolinfo()); // use bioseq_hl
    case CConstraint_choice::e_Field_missing:
*
     if (x_GetConstraintFieldFromObject(data, cons.GetField_missing()).empty()){
           return true; 
     }
     else return false;
*
    case CConstraint_choice::e_Translation:
     // must be coding region or protein feature
*
      if (data.GetData().IsProt()) {
         const CSeq_feat* cds = sequence::GetCDSForProduct(m_bioseq_hl);
         if (cds) {
            return x_DoesCodingRegionMatchTranslationConstraint (
                              *cds, cons.GetTranslation());
         }
      }
      else if (data.GetData().IsCdregion()) {
         return x_DoesCodingRegionMatchTranslationConstraint(
                                             data, cons.GetTranslation());
      }
*
    default: break;
  }
  return true;
};
*/

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1744, CRC32: 24faae6b */
