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
* Author:  Aaron Ucko, NCBI
*
* File Description:
*   new (early 2003) flat-file generator -- qualifier types
*   (mainly of interest to implementors)
*
* ===========================================================================
*/
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <util/sgml_entity.hpp>
#include <serial/enumvalues.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/pub/Pub_set.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Code_break.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>
//#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seq/seqport_util.hpp>
#include <objmgr/seq_vector.hpp>

#include <objtools/format/items/qualifiers.hpp>
#include <objtools/format/context.hpp>
#include "utils.hpp"


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

const string IFlatQVal::kSpace     = " ";
const string IFlatQVal::kSemicolon = ";";
const string IFlatQVal::kComma     = ",";
const string IFlatQVal::kEOL       = "\n";


static void s_StripTags( string& str )
{
    // Purpose: Strip HTML like tags from the given string
    string stripped;
    string::size_type gt = str.find('<');
    while ( gt != string::npos ) {
        string::size_type lt = str.find( '>', gt );
        if ( lt != string::npos ) {
            stripped += str.substr( 0, gt );
            str = str.substr( lt+1 );
            gt = str.find('<');
        }
        else {
            break;
        }
    }
    stripped += str;
    str = stripped;
}


static bool s_IsNote(IFlatQVal::TFlags flags, CBioseqContext& ctx)
{
    return (flags & IFlatQVal::fIsNote)  &&  !ctx.Config().IsModeDump();
}


static bool s_StringIsJustQuotes(const string& str)
{
    ITERATE(string, it, str) {
        if ( (*it != '"')  &&  (*it != '\'') ) {
            return false;
        }
    }

    return true;
}

static string s_GetGOText(const CUser_field& field, bool is_ftable)
{
    const string* text_string = NULL,
                * evidence = NULL,
                * go_id = NULL,
                * go_ref = NULL;
    string temp;
    int pmid = 0;
    
    

    ITERATE (CUser_field::C_Data::TFields, it, field.GetData().GetFields()) {
        if ( !(*it)->IsSetLabel()  ||  !(*it)->GetLabel().IsStr() ) {
            continue;
        }
        
        const string& label = (*it)->GetLabel().GetStr();
        const CUser_field::C_Data& data = (*it)->GetData();
        
        if (data.IsStr()) {
            if (label == "text string") {
                text_string = &data.GetStr();
            } else if (label == "go id") {
                go_id = &data.GetStr();
            } else if (label == "evidence") {
                evidence = &data.GetStr();
            } else if (label == "go ref") {
                go_ref = &data.GetStr();
            }
        } else if (data.IsInt()) {
            if (label == "go id") {
                NStr::IntToString(temp, data.GetInt());
                go_id = &temp;
            } else if (label == "pubmed id") {
                pmid = data.GetInt();
            }
        }
    }
    
    string go_text;
    if (text_string != NULL) {
        go_text = *text_string;
    }
    if ( is_ftable ) {
        go_text += '|';
        if (go_id != NULL) {
            go_text += *go_id;
        }
        go_text += '|';
        if ( pmid != 0 ) {
            go_text +=  NStr::IntToString(pmid);
        }
        if (evidence != NULL) {
            go_text += '|';
            go_text += *evidence;
        }
    } else { 
        if (go_id != NULL) {
            go_text = string( "GO:" ) + *go_id;
            if ( text_string != 0 ) {
                go_text += string( " - " ) + *text_string;
            }
            if ( evidence != 0 ) {
                go_text += string( " [Evidence " ) + *evidence + string( "]" );
            }
            if ( pmid != 0 ) {
                go_text += string( " [PMID " ) + NStr::IntToString(pmid) + string( "]" );
            }
            if ( go_ref != 0 ) {
                go_text += string( " [GO Ref " ) + *go_ref + string( "]" );
            }
        }
    }
    NStr::TruncateSpacesInPlace(go_text);
    return go_text;
}


static void s_ReplaceUforT(string& codon)
{
    NON_CONST_ITERATE (string, base, codon) {
        if (*base == 'T') {
            *base = 'U';
        }
    }
}


static char s_MakeDegenerateBase(const string &str1, const string& str2)
{
    static const char kIdxToSymbol[] = "?ACMGRSVUWYHKDBN";
    
    vector<char> symbol_to_idx(256, '\0');
    for (size_t i = 0; i < sizeof(kIdxToSymbol) - 1; ++i) {
        symbol_to_idx[kIdxToSymbol[i]] = i;
    }

    size_t idx = symbol_to_idx[str1[2]] | symbol_to_idx[str2[2]];
    return kIdxToSymbol[idx];
}


static size_t s_ComposeCodonRecognizedStr(const CTrna_ext& trna, string& recognized)
{
    recognized.erase();

    if (!trna.IsSetCodon()) {
        return 0;
    }

    list<string> codons;
    
    ITERATE (CTrna_ext::TCodon, it, trna.GetCodon()) {
        string codon = CGen_code_table::IndexToCodon(*it);
        s_ReplaceUforT(codon);
        if (!codon.empty()) {
            codons.push_back(codon);
        }
    }
    if (codons.empty()) {
        return 0;
    }
    size_t size = codons.size();
    if (size > 1) {
        codons.sort();

        list<string>::iterator it = codons.begin();
        list<string>::iterator prev = it++;
        while (it != codons.end()) {
            string& codon1 = *prev;
            string& codon2 = *it;
            if (codon1[0] == codon2[0]  &&  codon1[1] == codon2[1]) {
                codon1[2] = s_MakeDegenerateBase(codon1, codon2);
                it = codons.erase(it);
            } else {
                prev = it;
                ++it;
            }
        }
    }

    recognized = NStr::Join(codons, ", ");
    return size;
}


/////////////////////////////////////////////////////////////////////////////
// CFormatQual - low-level formatted qualifier

CFormatQual::CFormatQual
(const string& name,
 const string& value, 
 const string& prefix,
 const string& suffix,
 TStyle style) :
    m_Name(name), m_Value(value), m_Prefix(prefix), m_Suffix(suffix),
    m_Style(style), m_AddPeriod(false)
{
    NStr::TruncateSpacesInPlace(m_Value, NStr::eTrunc_End);
}


CFormatQual::CFormatQual(const string& name, const string& value, TStyle style) :
    m_Name(name), m_Value(value), m_Prefix(" "), m_Suffix(kEmptyStr),
    m_Style(style), m_AddPeriod(false)
{
    NStr::TruncateSpacesInPlace(m_Value, NStr::eTrunc_End);
}


// === CFlatStringQVal ======================================================

CFlatStringQVal::CFlatStringQVal(const string& value, TStyle style)
    :  IFlatQVal(&kSpace, &kSemicolon),
       m_Value(value), m_Style(style), m_AddPeriod(0)
{
	MakeLegalFlatFileString( m_Value );
    NStr::TruncateSpacesInPlace(m_Value);
}


CFlatStringQVal::CFlatStringQVal
(const string& value,
 const string& pfx,
 const string& sfx,
 TStyle style)
    :   IFlatQVal(&pfx, &sfx),
        m_Value(value),
        m_Style(style), m_AddPeriod(0)
{
	MakeLegalFlatFileString( m_Value );
    NStr::TruncateSpacesInPlace(m_Value);
}


void CFlatStringQVal::Format(TFlatQuals& q, const string& name,
                           CBioseqContext& ctx, IFlatQVal::TFlags flags) const
{
    flags |= m_AddPeriod;

    ETildeStyle tilde_style = (name == "seqfeat_note" ? eTilde_note : eTilde_space);
    ExpandTildes(m_Value, tilde_style);
                
    TFlatQual qual = x_AddFQ(q, (s_IsNote(flags, ctx) ? "note" : name), m_Value, m_Style);
    if ((flags & fAddPeriod)  &&  qual) {
        qual->SetAddPeriod();
    }
}


// === CFlatNumberQVal ======================================================


void CFlatNumberQVal::Format
(TFlatQuals& quals,
 const string& name,
 CBioseqContext& ctx,
 TFlags flags) const
{
    if (ctx.Config().CheckQualSyntax()) {
        if (NStr::IsBlank(m_Value)) {
            return;
        }
        bool has_space = false;
        ITERATE (string, it, m_Value) {
            if (isspace((unsigned char)(*it))) {
                has_space = true;
            } else if (has_space) {
                // non-space after space
                return;
            }
        }
    }

    CFlatStringQVal::Format(quals, name, ctx, flags);
}


// === CFlatBondQVal ========================================================

void CFlatBondQVal::Format
(TFlatQuals& quals,
 const string& name,
 CBioseqContext& ctx,
 TFlags flags) const
{
    string value = m_Value;
    if (s_IsNote(flags, ctx)) {
        value += " bond";
    }
    x_AddFQ(quals, (s_IsNote(flags, ctx) ? "note" : name), value, m_Style);
}


// === CFlatGeneQVal ========================================================

void CFlatGeneQVal::Format
(TFlatQuals& quals,
 const string& name,
 CBioseqContext& ctx,
 TFlags flags) const
{
    if (ctx.IsJournalScan()) {
        s_StripTags(m_Value);
        Sgml2Ascii(m_Value);
    }
    CFlatStringQVal::Format(quals, name, ctx, flags);
}


// === CFlatSiteQVal ========================================================

void CFlatSiteQVal::Format
(TFlatQuals& quals,
 const string& name,
 CBioseqContext& ctx,
 TFlags flags) const
{
    if ( m_Value == "transmembrane-region" ) {
        m_Value = "transmembrane region";
    }
    if ( m_Value == "signal-peptide" ) {
        m_Value = "signal peptide";
    }
    if ( m_Value == "transit-peptide" ) {
        m_Value = "transit peptide";
    }
    if (m_Value != "transit peptide" && m_Value != "signal peptide" &&
        m_Value != "transmembrane region" && s_IsNote(flags, ctx)) 
    {
        m_Value += " site";
    }
    CFlatStringQVal::Format(quals, name, ctx, flags);
}


// === CFlatStringListQVal ==================================================


void CFlatStringListQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    if (m_Value.empty()) {
        return;
    }

    if ( s_IsNote(flags, ctx) ) {
        m_Suffix = &kSemicolon;
    }

    x_AddFQ(q, 
            (s_IsNote(flags, ctx) ? "note" : name),
            JoinString(m_Value, "; "),
            m_Style);
}


// === CFlatGeneSynonymsQVal ================================================

void CFlatGeneSynonymsQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    if (GetValue().empty()) {
        return;
    }

    string qual = "gene_synonym";
    list<string> synonyms = GetValue();
    for ( list<string>::iterator it = synonyms.begin(); it != synonyms.end(); it++ ) {
        x_AddFQ( q, qual, *it, m_Style );
    }
}

// === CFlatCodeBreakQVal ===================================================

void CFlatCodeBreakQVal::Format(TFlatQuals& q, const string& name,
                              CBioseqContext& ctx, IFlatQVal::TFlags) const
{
    static const char* kOTHER = "OTHER";

    ITERATE (CCdregion::TCode_break, it, m_Value) {
        string pos = CFlatSeqLoc((*it)->GetLoc(), ctx).GetString();
        const char* aa  = kOTHER;
        switch ((*it)->GetAa().Which()) {
        case CCode_break::C_Aa::e_Ncbieaa:
            aa = GetAAName((*it)->GetAa().GetNcbieaa(), true);
            break;
        case CCode_break::C_Aa::e_Ncbi8aa:
            aa = GetAAName((*it)->GetAa().GetNcbi8aa(), false);
            break;
        case CCode_break::C_Aa::e_Ncbistdaa:
            aa = GetAAName((*it)->GetAa().GetNcbistdaa(), false);
            break;
        default:
            return;
        }
        x_AddFQ(q, name, "(pos:" + pos + ",aa:" + aa + ')', 
            CFormatQual::eUnquoted);
    }
}


CFlatCodonQVal::CFlatCodonQVal(unsigned int codon, unsigned char aa, bool is_ascii)
    : m_Codon(CGen_code_table::IndexToCodon(codon)),
      m_AA(GetAAName(aa, is_ascii)), m_Checked(true)
{
}


void CFlatCodonQVal::Format(TFlatQuals& q, const string& name, CBioseqContext& ctx,
                          IFlatQVal::TFlags) const
{
    if ( !m_Checked ) {
        // ...
    }
    x_AddFQ(q, name, "(seq:\"" + m_Codon + "\",aa:" + m_AA + ')');
}


void CFlatExperimentQVal::Format(TFlatQuals& q, const string& name,
                          CBioseqContext&, IFlatQVal::TFlags) const
{
    const char* s = "experimental evidence, no additional details recorded";
    x_AddFQ(q, name, s, CFormatQual::eQuoted);
}


CFlatInferenceQVal::CFlatInferenceQVal( const string& gbValue ) :
    m_str( "non-experimental evidence, no additional details recorded" )
{
    //
    //  the initial "non-experimental ..." is just a default value which may be
    //  overridden through an additional "inference" Gb-qual in the ASN.1.
    //  However, it can't be overriden to be just anything, only certain strings
    //  are allowed.
    //  The following code will change m_str from its default if gbValue is a
    //  legal replacement for "non-experimental ...", and leave it alone 
    //  otherwise.
    //
	string prefix = "";
	string remainder = "";
    CInferencePrefixList::GetPrefixAndRemainder (gbValue, prefix, remainder);
	if (!NStr::IsBlank(prefix)) {
        m_str = gbValue;
    }
}


void CFlatInferenceQVal::Format(TFlatQuals& q, const string& name,
                          CBioseqContext&, IFlatQVal::TFlags) const
{
    x_AddFQ(q, name, m_str, CFormatQual::eQuoted);
}


void CFlatIllegalQVal::Format(TFlatQuals& q, const string&, CBioseqContext &ctx,
                            IFlatQVal::TFlags) const
{
    // XXX - return if too strict
    x_AddFQ(q, m_Value->GetQual(), m_Value->GetVal());
}


void CFlatMolTypeQVal::Format(TFlatQuals& q, const string& name,
                            CBioseqContext& ctx, IFlatQVal::TFlags flags) const
{
    const char* s = 0;
    switch ( m_Biomol ) {

    default:
        break;

    case CMolInfo::eBiomol_unknown:
        switch ( m_Mol ) {
        case CSeq_inst::eMol_dna:  
            s = "unassigned DNA"; 
            break;
        case CSeq_inst::eMol_rna:  
            s = "unassigned RNA"; 
            break;
        default:                   
            break;
        }
        break;

    case CMolInfo::eBiomol_genomic:
        switch ( m_Mol ) {
        case CSeq_inst::eMol_dna:  
            s = "genomic DNA";  
            break;
        case CSeq_inst::eMol_rna:  
            s = "genomic RNA";  
            break;
        default:                   
            break;
        }
        break;

    case CMolInfo::eBiomol_mRNA:     
        s = "mRNA";      
        break;

    case CMolInfo::eBiomol_rRNA:     
        s = "rRNA";      
        break;
    
    case CMolInfo::eBiomol_tRNA:     
        s = "tRNA";      
        break;

    case CMolInfo::eBiomol_pre_RNA:  
    case CMolInfo::eBiomol_snRNA:    
    case CMolInfo::eBiomol_scRNA:    
    case CMolInfo::eBiomol_snoRNA:
    case CMolInfo::eBiomol_ncRNA:
    case CMolInfo::eBiomol_tmRNA:
    case CMolInfo::eBiomol_transcribed_RNA:
        s = "transcribed RNA";     
        break;

    case CMolInfo::eBiomol_other_genetic:
    case CMolInfo::eBiomol_other:
        switch ( m_Mol ) {
        case CSeq_inst::eMol_dna:  
            s = "other DNA";  
            break;
        case CSeq_inst::eMol_rna:  
            s = "other RNA";  
            break;
        default:                   
            break;
        }
        break;

    case CMolInfo::eBiomol_cRNA:
        s = "viral cRNA";     
        break;

    }

    if ( s == 0 ) {
        switch ( m_Mol ) {
        case CSeq_inst::eMol_rna:
            s = "unassigned RNA";
            break;
        case CSeq_inst::eMol_aa:
            s = 0;
            break;
        case CSeq_inst::eMol_dna:
        default:
            s = "unassigned DNA";
            break;
        }
    }

    if ( s != 0 ) {
        x_AddFQ(q, name, s);
    }
}

static string s_GetSpecimenVoucherTag(
    const string& strRawName,
    const string& strBase,
    const string& strInst )
{
    CNcbiOstrstream text;
    
    return CNcbiOstrstreamToString(text);
}


static string s_GetSpecimenVoucherText(
    CBioseqContext& ctx,
    const string& strRawName )
{
    const string strAtccBase( "http://www.atcc.org/SearchCatalogs/linkin?id=" );
    const string strCcmpBase( "http://ccmp.bigelow.org/SD/display.php?starin=CCMP" );
    const string strUamBase( "http://arctos.database.museum/SpecimenDetail.cfm?GUID=" );
    const string strCcugBase( "http://www.ccug.se/default.cfm?page=search_record.cfm&db=mc&s_tests=1&ccugno=" );
    const string strDsmzBase( "http://www.dsmz.de/microorganisms/search_no.php?q=" );
    const string strFsuBase( "http://www.prz.uni-jena.de/data.php?fsu=" );
    const string strPcmbBase( "http://www2.bishopmuseum.org/HBS/PCMB/results3.asp?searchterm3=" );
    const string strKuiBase( "http://collections.nhm.ku.edu/KU_Fish/detail.jsp?record=" );
    const string strKuitBase( "http://collections.nhm.ku.edu/KU_Tissue/detail.jsp?record=" );
    const string strBcrcBase( "http://strain.bcrc.firdi.org.tw/BSAS/controller?event=SEARCH&bcrs_no=" );
    const string strPccBase( "http://www.pasteur.fr/recherche/banque/PCC/docs/pcc" );
    
    const string strMsbInst( "Museum of Southwestern Biology, University of New Mexico" );
    const string strUamInst( "Museum of Alaska Museum of the North" );
    const string strWnmuInst( "Western New Mexico Museum" );
    const string strPsuInst( "Portland State University" );
    const string strCrcmInst( "Charles R. Conner Museum, Washington State University" );
    const string strDgrInst( "Division of Genomic Resources, University of New Mexico" );
    const string strKwpInst( "Kenelm W. Philip Collection, Museum of Alaska Museum of the North" );
    const string strMvzInst( "Museum of Vertebrate Zoology, University of California" );
    const string strNbsbInst( "National Biomonitoring Specimen Bank, U.S. Geological Survey" );
    const string strAtccInst( "American Type Culture Collection" );
    const string strCcmpInst( "Provasoli-Guillard National Center for Culture of Marine Phytoplankton" );
    const string strCcugInst( "Culture Collection, University of Goteborg, Department of Clinical Bacteriology" );
    const string strDsmzInst( "German Resource Center for Biological Material" );
    const string strFsuInst( "Fungal Reference Center, University of Jena" );
    const string strPcmbInst( "Pacific Center for Molecular Biodiversity" );
    const string strKuInst( "University of Kansas, Museum of Natural History" );
    const string strBcrcInst( "Bioresource Collection and Research Center" );
    const string strPccInst( "Pasteur Culture Collection of Cyanobacteria" );
    
    if ( ! ctx.Config().DoHTML() ) {
        return strRawName;
    }
    
    CNcbiOstrstream text;
    
    //  Base, institute, colon, id, none
    if ( NStr::StartsWith( strRawName, "CRCM:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "CRCM:Bird" ) );
        text << "CRCM:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strCrcmInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DGR:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DGR:Bird" ) );
        text << "DGR:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strDgrInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DGR:Ento" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DGR:Ento" ) );
        text << "DGR:Ento:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strDgrInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DGR:Fish" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DGR:Fish" ) );
        text << "DGR:Fish:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strDgrInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DGR:Herp" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DGR:Herp" ) );
        text << "DGR:Herp:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strDgrInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DGR:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DGR:Mamm" ) );
        text << "DGR:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strDgrInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "KWP:Ento" ) ) {
        string strId = strRawName.substr( 1 + strlen( "KWP:Ento" ) );
        text << "KWP:Ento:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strKwpInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MSB:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MSB:Mamm" ) );
        text << "MSB:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMsbInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MSB:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MSB:Bird" ) );
        text << "MSB:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMsbInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MSB:Para" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MSB:Para" ) );
        text << "MSB:Para:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMsbInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Bird" ) );
        text << "MVZ:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Egg" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Egg" ) );
        text << "MVZ:Egg:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Herp" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Herp" ) );
        text << "MVZ:Herp:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Hild" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Hild" ) );
        text << "MVZ:Hild:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Img" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Img" ) );
        text << "MVZ:Img:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Mamm" ) );
        text << "MVZ:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZ:Page" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZ:Page" ) );
        text << "MVZ:Page:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "MVZObs:Herp" ) ) {
        string strId = strRawName.substr( 1 + strlen( "MVZObs:Herp" ) );
        text << "MVZObs:Herp:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strMvzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "NBSB:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "NBSB:Bird" ) );
        text << "NBSB:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strNbsbInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "PSU:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "PSU:Mamm" ) );
        text << "PSU:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strPsuInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Bird" ) );
        text << "UAM:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Bryo" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Bryo" ) );
        text << "UAM:Bryo:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Crus" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Crus" ) );
        text << "UAM:Crus:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Ento" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Ento" ) );
        text << "UAM:Ento:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Fish" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Fish" ) );
        text << "UAM:Fish:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Herb" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Herb" ) );
        text << "UAM:Herb:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Herp" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Herp" ) );
        text << "UAM:Herp:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Mamm" ) );
        text << "UAM:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Moll" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Moll" ) );
        text << "UAM:Moll:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "UAM:Paleo" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Paleo" ) );
        text << "UAM:Paleo:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    else if ( NStr::StartsWith( strRawName, "UAMObs:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "UAM:Mamm" ) );
        text << "UAM:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strUamInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "WNMU:Bird" ) ) {
        string strId = strRawName.substr( 1 + strlen( "WNMU:Bird" ) );
        text << "WNMU:Bird:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strWnmuInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "WNMU:Fish" ) ) {
        string strId = strRawName.substr( 1 + strlen( "WNMU:Fish" ) );
        text << "WNMU:Fish:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strWnmuInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "WNMU:Mamm" ) ) {
        string strId = strRawName.substr( 1 + strlen( "WNMU:Mamm" ) );
        text << "WNMU:Mamm:<a ";
        text << "href=\"" << strUamBase << strRawName << "\" title=\"" << strWnmuInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    //  Base, none, none, id, none
    else if ( NStr::StartsWith( strRawName, "ATCC" ) ) {
        string strId = strRawName.substr( 1 + strlen( "ATCC" ) );
        text << "ATCC:<a ";
        text << "href=\"" << strAtccBase << strId << "\" title=\"" << strAtccInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "CCPM" ) ) {
        string strId = strRawName.substr( 1 + strlen( "CCMP" ) );
        text << "CCMP:<a ";
        text << "href=\"" << strCcmpBase << strId << "\" title=\"" << strCcmpInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "CCUG" ) ) {
        string strId = strRawName.substr( 1 + strlen( "CCUG" ) );
        text << "CCUG:<a ";
        text << "href=\"" << strCcugBase << strId << "\" title=\"" << strCcugInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "DSM" ) ) {
        string strId = strRawName.substr( 1 + strlen( "DSM" ) );
        text << "DSM:<a ";
        text << "href=\"" << strDsmzBase << strId << "\" title=\"" << strDsmzInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "FSU<DEU>" ) ) {
        string strId = strRawName.substr( 1 + strlen( "FSU<DEU>" ) );
        text << "FSU&ge;DEU&le;:<a ";
        text << "href=\"" << strFsuBase << strId << "\" title=\"" << strFsuInst << "\">";
        text << strId;
        text << "</a>";
    }
    
    else if ( NStr::StartsWith( strRawName, "PCMB" ) ) {
        string strId = strRawName.substr( 1 + strlen( "PCMB" ) );
        text << "PCMB:<a ";
        text << "href=\"" << strPcmbBase << strId << "\" title=\"" << strPcmbInst << "\">";
        text << strId;
        text << "</a>";
    }

    else if ( NStr::StartsWith( strRawName, "KU:I" ) ) {
        string strId = strRawName.substr( 1 + strlen( "KU:I" ) );
        text << "KU:I:<a ";
        text << "href=\"" << strKuiBase << strId << "\" title=\"" << strKuInst << "\">";
        text << strId;
        text << "</a>";
    }

    else if ( NStr::StartsWith( strRawName, "KU:IT" ) ) {
        string strId = strRawName.substr( 1 + strlen( "KU:IT" ) );
        text << "KU:IT:<a ";
        text << "href=\"" << strKuitBase << strId << "\" title=\"" << strKuInst << "\">";
        text << strId;
        text << "</a>";
    }

    //base, none, prefix, postfix
    else if ( NStr::StartsWith( strRawName, "BCRC" ) ) {
        const string strPrefix( "" );
        const string strPostfix( "&type_id=6&keyword=;;" ); 
        string strId = strRawName.substr( 1 + strlen( "BCRC" ) );
        text << "BCRC:<a ";
        text << "href=\"" << strBcrcBase << strPrefix << strId << strPostfix << "\" title=\"" << strBcrcInst << "\">";
        text << strId;
        text << "</a>";
    }
        
    else if ( NStr::StartsWith( strRawName, "PCC" ) ) {
        const string strPrefix( "" );
        const string strPostfix( ".htm" ); 
        string strId = strRawName.substr( 1 + strlen( "PCC" ) );
        text << "PCC:<a ";
        text << "href=\"" << strPccBase << strPrefix << strId << strPostfix << "\" title=\"" << strPccInst << "\">";
        text << strId;
        text << "</a>";
    }
        
    //  no pattern, just replicate
    else {
        text << strRawName;
    }
    return CNcbiOstrstreamToString(text);
}

void CFlatOrgModQVal::Format(TFlatQuals& q, const string& name,
                           CBioseqContext& ctx, IFlatQVal::TFlags flags) const
{
    TFlatQual qual;

    string subname = m_Value->GetSubname();
    if ( s_StringIsJustQuotes(subname) ) {
        subname = kEmptyStr;
    }
    ConvertQuotes(subname);
    
    if (s_IsNote(flags, ctx)) {
        bool add_period = RemovePeriodFromEnd(subname, true);
        if (!subname.empty()) {
            bool is_src_orgmod_note = 
                (flags & IFlatQVal::fIsSource)  &&  (name == "orgmod_note");
            if (is_src_orgmod_note) {
                if (add_period) {
                    AddPeriod(subname);
                }
                m_Suffix = &kEOL;
                qual = x_AddFQ(q, "note", s_GetSpecimenVoucherText(ctx, subname));
            } else {
                qual = x_AddFQ(q, "note", name + ": " + s_GetSpecimenVoucherText(ctx, subname));
            }
            if (add_period  &&  qual) {
                qual->SetAddPeriod();
            }
        }
    } else {
        x_AddFQ(q, name, s_GetSpecimenVoucherText(ctx, subname) );
    }
}


void CFlatOrganelleQVal::Format(TFlatQuals& q, const string& name,
                              CBioseqContext&, IFlatQVal::TFlags) const
{
    const string& organelle
        = CBioSource::GetTypeInfo_enum_EGenome()->FindName(m_Value, true);

    switch (m_Value) {
    case CBioSource::eGenome_chloroplast: 
    case CBioSource::eGenome_chromoplast:
    case CBioSource::eGenome_cyanelle:    
    case CBioSource::eGenome_apicoplast:
    case CBioSource::eGenome_leucoplast:  
    case CBioSource::eGenome_proplastid:
        x_AddFQ(q, name, "plastid:" + organelle);
        break;

    case CBioSource::eGenome_kinetoplast:
        x_AddFQ(q, name, "mitochondrion:kinetoplast");
        break;

    case CBioSource::eGenome_mitochondrion: 
    case CBioSource::eGenome_plastid:
    case CBioSource::eGenome_nucleomorph:
        x_AddFQ(q, name, organelle);
        break;

    case CBioSource::eGenome_macronuclear: 
    case CBioSource::eGenome_proviral:
        x_AddFQ(q, organelle, kEmptyStr, CFormatQual::eEmpty);
        break;
    case CBioSource::eGenome_virion:
//        x_AddFQ(q, organelle, kEmptyStr, CFormatQual::eEmpty);
        break;

    case CBioSource::eGenome_plasmid: 
    case CBioSource::eGenome_transposon:
        x_AddFQ(q, organelle, kEmptyStr);
        break;

    case CBioSource::eGenome_insertion_seq:
        x_AddFQ(q, "insertion_seq", kEmptyStr);
        break;

    default:
        break;
    }
    
}


void CFlatPubSetQVal::Format(TFlatQuals& q, const string& name,
                           CBioseqContext& ctx, IFlatQVal::TFlags) const
{
    ITERATE (vector< CRef<CReferenceItem> >, it, ctx.GetReferences()) {
        if ((*it)->Matches(*m_Value)) {
            x_AddFQ(q, name, '[' + NStr::IntToString((*it)->GetSerial()) + ']',
                    CFormatQual::eUnquoted);
        }
    }
}


void CFlatSeqIdQVal::Format(TFlatQuals& q, const string& name,
                          CBioseqContext& ctx, IFlatQVal::TFlags) const
{
    // XXX - add link in HTML mode
    string id_str;
    if ( m_Value->IsGi() ) {
        if ( m_GiPrefix ) {
            id_str = "GI:";
        }
        m_Value->GetLabel(&id_str, CSeq_id::eContent);
    } else {
        id_str = m_Value->GetSeqIdString(true);
    }
    x_AddFQ(q, name, id_str);
}


void s_ConvertGtLt(string& subname)
{
    SIZE_TYPE pos;
    for (pos = subname.find('<'); pos != NPOS; pos = subname.find('<', pos)) {
        subname.replace(pos, 1, "&lt");
    }
    for (pos = subname.find('>'); pos != NPOS; pos = subname.find('>', pos)) {
        subname.replace(pos, 1, "&gt");
    }
}

void CFlatSubSourcePrimer::Format(
    TFlatQuals& q, 
    const string& name,
    CBioseqContext& ctx, 
    IFlatQVal::TFlags flags) const
{
    vector< string > fwd_names;
    if ( ! m_fwd_name.empty() ) {
        string fwd_name = NStr::Replace( m_fwd_name, "(", "" );
        NStr::ReplaceInPlace( fwd_name, ")", "" );
        NStr::Tokenize( fwd_name, ",", fwd_names );
    }
    if ( fwd_names.empty() ) {
        return;
    } 
    
    vector< string > rev_names;
    if ( ! m_rev_name.empty() ) {
        string rev_name = NStr::Replace( m_rev_name, "(", "" );
        NStr::ReplaceInPlace( rev_name, ")", "" );
        NStr::Tokenize( rev_name, ",", rev_names );
    }
    if ( rev_names.size() != fwd_names.size() ) {
        return;
    }

    vector< string > fwd_seqs;
    if ( ! m_fwd_seq.empty() ) {
        string fwd_seq = NStr::Replace( m_fwd_seq, "(", "" );
        NStr::ReplaceInPlace( fwd_seq, ")", "" );
        NStr::Tokenize( fwd_seq, ",", fwd_seqs );
    }
    if ( fwd_seqs.size() != fwd_names.size() ) {
        return;
    }

    vector< string > rev_seqs;
    if ( ! m_rev_seq.empty() ) {
        string rev_seq = NStr::Replace( m_rev_seq, "(", "" );
        NStr::ReplaceInPlace( rev_seq, ")", "" );
        NStr::Tokenize( rev_seq, ",", rev_seqs );
    }
    if ( rev_seqs.size() != fwd_names.size() ) {
        return;
    }

    for ( size_t i=0; i < fwd_names.size(); ++i ) {

        string value( "fwd_name: " );
        value += fwd_names[i];
        value += ", fwd_seq: ";
        value += fwd_seqs[i];
        value += ", rev_name: ";
        value += rev_names[i];
        value += ", rev_seq: ";
        value += rev_seqs[i];
        x_AddFQ( q, "PCR_primers", value );
    }
}

void CFlatSubSourceQVal::Format(TFlatQuals& q, const string& name,
                              CBioseqContext& ctx, IFlatQVal::TFlags flags) const
{
    TFlatQual qual;
    string subname = m_Value->CanGetName() ? m_Value->GetName() : kEmptyStr;
    if ( s_StringIsJustQuotes(subname) ) {
        subname = kEmptyStr;
    }
    ConvertQuotes(subname);
    if (ctx.Config().DoHTML()) {
        s_ConvertGtLt(subname);
    }

    if ( s_IsNote(flags, ctx) ) {
        bool add_period = RemovePeriodFromEnd(subname, true);
        if (!subname.empty()) {
            bool is_subsource_note =
                m_Value->GetSubtype() == CSubSource::eSubtype_other;
            if (is_subsource_note) {
                if (add_period) {
                    AddPeriod(subname);
                }
                m_Suffix = &kEOL;
                qual = x_AddFQ(q, "note", subname);
            } else {
                qual = x_AddFQ(q, "note", name + ": " + subname);        
            }
            if (add_period  &&  qual) {
                qual->SetAddPeriod();
            }
        }
    } else {
        CSubSource::TSubtype subtype = m_Value->GetSubtype();
        switch( subtype ) {

        case CSubSource::eSubtype_germline:
        case CSubSource::eSubtype_rearranged:
        case CSubSource::eSubtype_transgenic:
        case CSubSource::eSubtype_environmental_sample:
            x_AddFQ(q, name, kEmptyStr, CFormatQual::eEmpty);
            break;

        case CSubSource::eSubtype_plasmid_name:
            ExpandTildes(subname, eTilde_space);
            x_AddFQ(q, name, subname);
            break;

        default:
            if ( ! subname.empty() ) {
                ExpandTildes(subname, eTilde_space);
                x_AddFQ(q, name, subname);
            }
            break;
        }
    }
}


void CFlatXrefQVal::Format(TFlatQuals& q, const string& name,
                         CBioseqContext& ctx, IFlatQVal::TFlags flags) const
{
    ITERATE (TXref, it, m_Value) {
        const CDbtag& dbt = **it;
        if (!m_Quals.Empty()  &&  x_XrefInGeneXref(dbt)) {
            continue;
        }

        CDbtag::TDb db = dbt.GetDb();
        if (db == "PID"  ||  db == "GI") {
            continue;
        }

        if (db == "cdd") {
            db = "CDD"; // canonicalize
        }

        if (ctx.Config().DropBadDbxref()) {
            if (!dbt.IsApproved(ctx.IsRefSeq())) {
                continue;
            }
            if ( (flags | IFlatQVal::fIsSource) && db == "dbEST" ) {
                // approved for features only
                continue;
            }
        }

        const CDbtag::TTag& tag = (*it)->GetTag();
        string id;
        if (tag.IsId()) {
            id = NStr::IntToString(tag.GetId());
        } else if (tag.IsStr()) {        
            id = tag.GetStr();
            if (NStr::EqualNocase(db, "MGI")  ||  NStr::EqualNocase(db, "MGD")) {
                if (NStr::StartsWith(id, "MGI:", NStr::eNocase)  ||
                    NStr::StartsWith(id, "MGD:", NStr::eNocase)) {
                    db = "MGI";
                    id.erase(0, 4);
                }
            }
        }
        if (NStr::IsBlank(id)) {
            continue;
        }

        CNcbiOstrstream db_xref;
        db_xref << db << ':';
        if (ctx.Config().DoHTML()) {
            string url = dbt.GetUrl();
            if (!NStr::IsBlank(url)) {
                db_xref <<  "<a href=" <<  url << '>' << id << "</a>";
            } else {
                db_xref << id;
            }
        } else {
            db_xref << id;
        }

        x_AddFQ(q, name, CNcbiOstrstreamToString(db_xref));
    }
}


bool CFlatXrefQVal::x_XrefInGeneXref(const CDbtag& dbtag) const
{
    if ( !m_Quals->HasQual(eFQ_gene_xref) ) {
        return false;
    }

    typedef TQuals::const_iterator TQCI;
    TQCI gxref = m_Quals->LowerBound(eFQ_gene_xref);
    TQCI end = m_Quals->end();
    while (gxref != end  &&  gxref->first == eFQ_gene_xref) {
        const CFlatXrefQVal* xrefqv =
            dynamic_cast<const CFlatXrefQVal*>(gxref->second.GetPointerOrNull());
        if (xrefqv != NULL) {
            ITERATE (TXref, dbt, xrefqv->m_Value) {
                if (dbtag.Match(**dbt)) {
                    return true;
                }
            }
        }
        ++gxref;
    }
    /*pair<TQCI, TQCI> gxrefs = m_Quals->GetQuals(eFQ_gene_xref);
    for ( TQCI it = gxrefs.first; it != gxrefs.second; ++it ) {
        const CFlatXrefQVal* xrefqv =
            dynamic_cast<const CFlatXrefQVal*>(it->second.GetPointerOrNull());
        if ( xrefqv != 0 ) {
            ITERATE (TXref, dbt, xrefqv->m_Value) {
                if ( dbtag.Match(**dbt) ) {
                    return true;
                }
            }
        }
    }*/
    return false;
}


static size_t s_CountAccessions(const CUser_field& field)
{
    size_t count = 0;

    if (!field.IsSetData()  ||  !field.GetData().IsFields()) {
        return 0;
    }
    
    //
    //  Each accession consists of yet another block of "Fields" one of which carries
    //  a label named "accession":
    //
    ITERATE (CUser_field::TData::TFields, it, field.GetData().GetFields()) {
        const CUser_field& uf = **it;
        if ( uf.CanGetData()  &&  uf.GetData().IsFields() ) {
        
            ITERATE( CUser_field::TData::TFields, it2, uf.GetData().GetFields() ) {
                const CUser_field& inner = **it2;
                if ( inner.IsSetLabel() && inner.GetLabel().IsStr() ) {
                    if ( inner.GetLabel().GetStr() == "accession" ) {
                        ++count;
                    }
                }
            }
        }
    }
    return count;
}


void CFlatModelEvQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    size_t num_mrna = 0, num_prot = 0, num_est = 0;
    const string* method = 0;

    ITERATE (CUser_object::TData, it, m_Value->GetData()) {
        const CUser_field& field = **it;
        if (!field.IsSetLabel()  &&  field.GetLabel().IsStr()) {
            continue;
        }
        const string& label = field.GetLabel().GetStr();
        if (label == "Method") {
            method = &label;
            if ( field.CanGetData() && field.GetData().IsStr() ) {
                method = &( field.GetData().GetStr() );
            }
        } else if (label == "mRNA") {
            num_mrna = s_CountAccessions(field);
        } else if (label == "EST") {
            num_est  = s_CountAccessions(field);
        } else if (label == "Protein") {
            num_prot = s_CountAccessions(field);
        }
    }

    CNcbiOstrstream text;
    text << "Derived by automated computational analysis";
    if (method != NULL  &&  !NStr::IsBlank(*method)) {
         text << " using gene prediction method: " << *method;
    }
    text << ".";

    if (num_mrna > 0  ||  num_est > 0  ||  num_prot > 0) {
        text << " Supporting evidence includes similarity to:";
    }
    string prefix = " ";
    if (num_mrna > 0) {
        text << prefix << num_mrna << " mRNA";
        if (num_mrna > 1) {
            text << 's';
        }
        prefix = ", ";
    }
    if (num_est > 0) {
        text << prefix << num_est << " EST";
        if (num_est > 1) {
            text << 's';
        }
        prefix = ", ";
    }
    if (num_prot > 0) {
        text << prefix << num_prot << " Protein";
        if (num_prot > 1) {
            text << 's';
        }
    }

    x_AddFQ(q, name, CNcbiOstrstreamToString(text));
}


void CFlatGoQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    _ASSERT(m_Value->GetData().IsFields());
    bool is_ftable = ctx.Config().IsFormatFTable();

    if ( s_IsNote(flags, ctx) ) {
        static const string sfx = ";";
        m_Prefix = &kEOL;
        m_Suffix = &sfx;
        x_AddFQ(q, "note", name + ": " + s_GetGOText(*m_Value, is_ftable));
    } else {
        x_AddFQ(q, name, s_GetGOText(*m_Value, is_ftable));
    }
}


void CFlatAnticodonQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    if ( !m_Anticodon->IsInt()  ||  m_Aa.empty() ) {
        return;
    }

    CNcbiOstrstream text;
    text << "(pos:" ;
    if (ctx.Config().IsModeRelease()) {
        CSeq_loc::TRange range = m_Anticodon->GetTotalRange();
        text << range.GetFrom() + 1 << ".." << range.GetTo() + 1;
    } else {
        text << CFlatSeqLoc(*m_Anticodon, ctx).GetString();
    }
    text << ",aa:" << m_Aa << ')';

    x_AddFQ(q, name, CNcbiOstrstreamToString(text), CFormatQual::eUnquoted);
}


void CFlatTrnaCodonsQVal::Format
(TFlatQuals& q,
 const string& name,
 CBioseqContext& ctx,
 IFlatQVal::TFlags flags) const
{
    if (!m_Value  ||  !m_Value->IsSetCodon()) {
        return;
    }
    string recognized;
    size_t num = s_ComposeCodonRecognizedStr(*m_Value, recognized);
    if ( 0 == num ) {
        return;
    }

    if ( ctx.Config().CodonRecognizedToNote() ) {
        if (num == 1) {
            string str = "codon recognized: " + recognized;
            if (NStr::Find(m_Seqfeat_note, str) == NPOS) {
                x_AddFQ(q, name, str);
            }
        }
        else {
            x_AddFQ(q, name, "codons recognized: " + recognized);
        }
    } else {
        x_AddFQ(q, "codon_recognized", recognized);
    }
}


void CFlatProductNamesQVal::Format
(TFlatQuals& quals,
 const string& name,
 CBioseqContext& ctx,
 TFlags flags) const
{
    if (m_Value.size() < 2) {
        return;
    }
    bool note = s_IsNote(flags, ctx);
    
    CProt_ref::TName::const_iterator it = m_Value.begin();
    ++it;  // first is used for /product
    while (it != m_Value.end()  &&  !NStr::IsBlank(*it)) {
        if (*it != m_Gene) {
            x_AddFQ(quals, (note ? "note" : name), *it);
        }
        ++it;
    }
}


END_SCOPE(objects)
END_NCBI_SCOPE
