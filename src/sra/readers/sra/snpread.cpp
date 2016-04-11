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
 * Authors:  Eugene Vasilchenko
 *
 * File Description:
 *   Access to SNP files
 *
 */

#include <ncbi_pch.hpp>
#include <sra/readers/sra/snpread.hpp>
#include <corelib/ncbistr.hpp>
#include <corelib/ncbi_param.hpp>
#include <objects/general/general__.hpp>
#include <objects/seqloc/seqloc__.hpp>
#include <objects/seqfeat/seqfeat__.hpp>
#include <objects/seqres/seqres__.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/seq/Annotdesc.hpp>
#include <objects/seqtable/seqtable__.hpp>
#include <sra/error_codes.hpp>
#include <objmgr/impl/snp_annot_info.hpp>
#include <unordered_map>

#include <sra/readers/sra/kdbread.hpp>

BEGIN_STD_NAMESPACE;

template<>
struct hash<ncbi::CTempString>
{
    size_t operator()(ncbi::CTempString val) const
        {
            unsigned long __h = 5381;
            for ( auto c : val ) {
                __h = __h*17 + c;
            }
            return size_t(__h);
        }
};

END_STD_NAMESPACE;

BEGIN_NCBI_NAMESPACE;

#define NCBI_USE_ERRCODE_X   SNPReader
NCBI_DEFINE_ERR_SUBCODE_X(1);

BEGIN_NAMESPACE(objects);


#define RC_NO_MORE_ALIGNMENTS RC(rcApp, rcQuery, rcSearching, rcRow, rcNotFound)

static const TSeqPos kPageSize = 5000;
static const TSeqPos kMaxSNPLength = 256;
static const size_t kFlagsSize = 8;
static const TSeqPos kCoverageZoom = 100;
static const size_t kMax_AlleleLength  = 32;
static const char kAlleleSeparator = '|';


/////////////////////////////////////////////////////////////////////////////
// CSNPDb_Impl
/////////////////////////////////////////////////////////////////////////////


struct CSNPDb_Impl::SSNPTableCursor : public CObject {
    explicit SSNPTableCursor(const CVDBTable& table);
    
    CVDBCursor m_Cursor;

    DECLARE_VDB_COLUMN_AS_STRING(ACCESSION);
    DECLARE_VDB_COLUMN_AS(INSDC_coord_zero, BLOCK_FROM);
    DECLARE_VDB_COLUMN_AS(Uint4, FEAT_COUNT);
    DECLARE_VDB_COLUMN_AS_STRING(FEAT_TYPE);
    DECLARE_VDB_COLUMN_AS(Uint4, FEAT_SUBTYPE);
    DECLARE_VDB_COLUMN_AS(INSDC_coord_zero, FROM);
    DECLARE_VDB_COLUMN_AS(INSDC_coord_len, LEN);
    DECLARE_VDB_COLUMN_AS(TVDBRowId, EXTRA_ROW_NUM);
    DECLARE_VDB_COLUMN_AS(Uint4, FEAT_ID_PREFIX);
    DECLARE_VDB_COLUMN_AS(Uint8, FEAT_ID_VALUE);
    DECLARE_VDB_COLUMN_AS(Uint8, BIT_FLAGS);
    DECLARE_VDB_COLUMN_AS(Uint4, FEAT_ZOOM_COUNT);
};


CSNPDb_Impl::SSNPTableCursor::SSNPTableCursor(const CVDBTable& table)
    : m_Cursor(table),
      INIT_VDB_COLUMN(ACCESSION),
      INIT_VDB_COLUMN(BLOCK_FROM),
      INIT_VDB_COLUMN(FEAT_COUNT),
      INIT_VDB_COLUMN(FEAT_TYPE),
      INIT_VDB_COLUMN(FEAT_SUBTYPE),
      INIT_VDB_COLUMN(FROM),
      INIT_VDB_COLUMN(LEN),
      INIT_VDB_COLUMN(EXTRA_ROW_NUM),
      INIT_VDB_COLUMN(FEAT_ID_PREFIX),
      INIT_VDB_COLUMN(FEAT_ID_VALUE),
      INIT_VDB_COLUMN(BIT_FLAGS),
      INIT_VDB_COLUMN(FEAT_ZOOM_COUNT)
{
}


struct CSNPDb_Impl::SExtraTableCursor : public CObject {
    explicit SExtraTableCursor(const CVDBTable& table);
    
    CVDBCursor m_Cursor;

    DECLARE_VDB_COLUMN_AS_STRING(RS_ALLELES);
};


CRef<CSNPDb_Impl::SSNPTableCursor> CSNPDb_Impl::SNP(TVDBRowId row)
{
    CRef<SSNPTableCursor> curs = m_SNP.Get(row);
    if ( !curs ) {
        curs = new SSNPTableCursor(SNPTable());
    }
    return curs;
}


CRef<CSNPDb_Impl::SExtraTableCursor> CSNPDb_Impl::Extra(TVDBRowId row)
{
    CRef<SExtraTableCursor> curs = m_Extra.Get(row);
    if ( !curs ) {
        if ( const CVDBTable& table = ExtraTable() ) {
            curs = new SExtraTableCursor(table);
        }
    }
    return curs;
}


void CSNPDb_Impl::Put(CRef<SSNPTableCursor>& curs, TVDBRowId row)
{
    m_SNP.Put(curs, row);
}


void CSNPDb_Impl::Put(CRef<SExtraTableCursor>& curs, TVDBRowId row)
{
    m_Extra.Put(curs, row);
}


CSNPDb_Impl::SExtraTableCursor::SExtraTableCursor(const CVDBTable& table)
    : m_Cursor(table),
      INIT_VDB_COLUMN(RS_ALLELES)
{
}


CSNPDb_Impl::CSNPDb_Impl(CVDBMgr& mgr,
                         CTempString path_or_acc)
    : m_Mgr(mgr),
      m_ExtraTableIsOpened(false)
{
    // SNP VDB are multi-table VDB objects.
    // However, there could be other VDBs in the same namespace (NA*)
    // so we have to check this situation and return normal eNotFoundDb error.
    try {
        CTempString acc = path_or_acc;
        if ( acc == "NA000000000" ) {
            acc = "~holmesbr/test_vdb/database";
        }
        m_Db = CVDB(m_Mgr, acc);
    }
    catch ( CSraException& exc ) {
        bool another_vdb_table = false;
        if ( exc.GetErrCode() != exc.eNotFoundDb ) {
            // check if the accession refers some other VDB object
            try {
                CVDBTable table(mgr, path_or_acc);
                another_vdb_table = true;
            }
            catch ( CSraException& /*exc2*/ ) {
            }
        }
        if ( another_vdb_table ) {
            // It's some other VDB table object
            // report eNotFoundDb with original rc
            NCBI_THROW2_FMT(CSraException, eNotFoundDb,
                            "Cannot open VDB: "<<path_or_acc,
                            exc.GetRC());
        }
        else {
            // neither VDB nor another VDB table
            // report original exception
            throw;
        }
    }
    m_SNPTable = CVDBTable(m_Db, "features");
    // only one ref seq
    if ( CRef<SSNPTableCursor> snp = SNP() ) {
        SSeqInfo* info = 0;
        for ( TVDBRowId row = 1, max_row = snp->m_Cursor.GetMaxRowId();
              row <= max_row; ++row ) {
            // read range and names
            CTempString ref_id = *snp->ACCESSION(row);
            if ( !info || info->m_Name != ref_id ) {
                m_SeqList.push_back(SSeqInfo());
                info = &m_SeqList.back();
                info->m_Name = info->m_SeqId = ref_id;
                info->m_Seq_ids.assign(1, Ref(new CSeq_id(ref_id)));
                info->m_Seq_id_Handle =
                    CSeq_id_Handle::GetHandle(*info->GetMainSeq_id());
                info->m_Circular = false;
            }
            SSeqInfo::SPageSet pset;
            pset.m_SeqPos = *snp->BLOCK_FROM(row);
            pset.m_PageCount = 1;
            pset.m_RowId = row;
            if ( !info->m_PageSets.empty() ) {
                SSeqInfo::SPageSet& prev_pset = info->m_PageSets.back();
                if ( prev_pset.GetSeqPosEnd(kPageSize) == pset.m_SeqPos &&
                     prev_pset.GetRowIdEnd() == pset.m_RowId ) {
                    prev_pset.m_PageCount += 1;
                    continue;
                }
            }
            info->m_PageSets.push_back(pset);
        }
        Put(snp);
    }

    NON_CONST_ITERATE ( TSeqInfoList, it, m_SeqList ) {
        m_SeqMapByName.insert
            (TSeqInfoMapByName::value_type(it->m_Name, it));
        ITERATE ( CBioseq::TId, id_it, it->m_Seq_ids ) {
            CSeq_id_Handle idh = CSeq_id_Handle::GetHandle(**id_it);
            m_SeqMapBySeq_id.insert
                (TSeqInfoMapBySeq_id::value_type(idh, it));
        }
    }
}


CSNPDb_Impl::~CSNPDb_Impl(void)
{
}


inline
void CSNPDb_Impl::OpenTable(CVDBTable& table,
                            const char* table_name,
                            volatile bool& table_is_opened)
{
    CFastMutexGuard guard(m_TableMutex);
    if ( !table_is_opened ) {
        table = CVDBTable(m_Db, table_name, CVDBTable::eMissing_Allow);
        table_is_opened = true;
    }
}


void CSNPDb_Impl::OpenExtraTable(void)
{
    OpenTable(m_ExtraTable, "extras", m_ExtraTableIsOpened);
}


TSeqPos CSNPDb_Impl::GetPageSize(void) const
{
    return kPageSize;
}


/////////////////////////////////////////////////////////////////////////////
// CSNPDbSeqIterator
/////////////////////////////////////////////////////////////////////////////


CSNPDbSeqIterator::CSNPDbSeqIterator(const CSNPDb& db)
    : m_Db(db),
      m_Iter(db->GetSeqInfoList().begin())
{
}


CSNPDbSeqIterator::CSNPDbSeqIterator(const CSNPDb& db,
                                     const string& name,
                                     EByName)
{
    CSNPDb_Impl::TSeqInfoMapByName::const_iterator iter =
        db->m_SeqMapByName.find(name);
    if ( iter != db->m_SeqMapByName.end() ) {
        m_Db = db;
        m_Iter = iter->second;
    }
}


CSNPDbSeqIterator::CSNPDbSeqIterator(const CSNPDb& db,
                                     const CSeq_id_Handle& seq_id)
{
    CSNPDb_Impl::TSeqInfoMapBySeq_id::const_iterator iter =
        db->m_SeqMapBySeq_id.find(seq_id);
    if ( iter != db->m_SeqMapBySeq_id.end() ) {
        m_Db = db;
        m_Iter = iter->second;
    }
}


const CSNPDb_Impl::SSeqInfo& CSNPDbSeqIterator::GetInfo(void) const
{
    if ( !*this ) {
        NCBI_THROW(CSraException, eInvalidState,
                   "CSNPDbSeqIterator is invalid");
    }
    return *m_Iter;
}


void CSNPDbSeqIterator::Reset(void)
{
    m_Db.Reset();
}


bool CSNPDbSeqIterator::IsCircular(void) const
{
    return GetInfo().m_Circular;
}


CRange<TSeqPos> CSNPDbSeqIterator::GetSNPRange(void) const
{
    const CSNPDb_Impl::SSeqInfo::TPageSets& psets = GetInfo().m_PageSets;
    return COpenRange<TSeqPos>(psets.front().m_SeqPos,
                               psets.back().GetSeqPosEnd(kPageSize));
}


CRange<TVDBRowId> CSNPDbSeqIterator::GetVDBRowRange(void) const
{
    const CSNPDb_Impl::SSeqInfo::TPageSets& psets = GetInfo().m_PageSets;
    return COpenRange<TVDBRowId>(psets.front().m_RowId,
                                 psets.back().GetRowIdEnd());
}


BEGIN_LOCAL_NAMESPACE;


CRef<CSeq_annot> x_NewAnnot(void)
{
    CRef<CSeq_annot> annot(new CSeq_annot);
    CRef<CAnnotdesc> desc(new CAnnotdesc);
    desc->SetName("SNP");
    annot->SetDesc().Set().push_back(desc);
    return annot;
}


CRef<CSeq_graph> x_NewCoverageGraph(const CSNPDbSeqIterator& it)
{
    CRef<CSeq_graph> graph(new CSeq_graph);
    graph->SetTitle("SNP Density");
    graph->SetLoc().SetInt().SetId(const_cast<CSeq_id&>(*it.GetSeqId()));
    graph->SetComp(kCoverageZoom);
    graph->SetNumval(0);
    CByte_graph& data = graph->SetGraph().SetByte();
    data.SetAxis(0);
    data.SetMin(0);
    data.SetMax(0);
    data.SetValues();
    return graph;
}


void x_ConvertByteToInt(CSeq_graph& graph)
{
    CConstRef<CByte_graph> old_data(&graph.GetGraph().GetByte());
    CInt_graph& data = graph.SetGraph().SetInt();
    data.SetAxis(0);
    data.SetMin(0);
    data.SetMax(old_data->GetMax());
    const CByte_graph::TValues& old_values = old_data->GetValues();
    CInt_graph::TValues& values = data.SetValues();
    values.assign(old_values.begin(), old_values.end());
}


inline TSeqPos x_RoundCoveragePos(TSeqPos pos)
{
    return pos - pos%kCoverageZoom;
}


inline TSeqPos x_RoundCoveragePosUp(TSeqPos pos)
{
    return x_RoundCoveragePos(pos+kCoverageZoom-1);
}


inline void x_AdjustRange(CRange<TSeqPos>& range,
                          const CSNPDbSeqIterator& it)
{
    range = range.IntersectionWith(it.GetSNPRange());
}


inline void x_AdjustCoverageGraphRange(CRange<TSeqPos>& range,
                                       const CSNPDbSeqIterator& it)
{
    x_AdjustRange(range, it);
    range.SetFrom(x_RoundCoveragePos(range.GetFrom()));
    range.SetToOpen(x_RoundCoveragePosUp(range.GetToOpen()));
}


TSeqPos x_AddCoverageLoc(CSeq_graph& graph, TSeqPos pos, TSeqPos end)
{
    _ASSERT(end > pos);
    _ASSERT((end-pos)%kCoverageZoom == 0);
    TSeqPos add = (end-pos) / kCoverageZoom;
    graph.SetNumval() += add;
    CSeq_interval& interval = graph.SetLoc().SetInt();
    if ( !interval.IsSetFrom() ) {
        interval.SetFrom(pos);
    }
    else {
        _ASSERT(interval.GetTo()+1 == pos);
    }
    interval.SetTo(end-1);
    return add;
}


void x_AddCoverageZeroes(CSeq_graph& graph, TSeqPos add)
{
    CSeq_graph::TGraph& g = graph.SetGraph();
    if ( g.IsByte() ) {
        CByte_graph& data = g.SetByte();
        CByte_graph::TValues& values = data.SetValues();
        values.resize(values.size()+add);
    }
    else {
        CInt_graph& data = g.SetInt();
        CInt_graph::TValues& values = data.SetValues();
        values.resize(values.size()+add);
    }
}


void x_AddCoverageValues(CSeq_graph& graph, TSeqPos add, const Uint4* ptr)
{
    CSeq_graph::TGraph& g = graph.SetGraph();
    Uint4 max_value = *max_element(ptr, ptr+add);
    if ( g.IsByte() ) {
        if ( max_value <= 255 ) {
            CByte_graph& data = g.SetByte();
            data.SetMax(max(data.GetMax(), int(max_value)));
            CByte_graph::TValues& values = data.SetValues();
            values.insert(values.end(), ptr, ptr+add);
            return;
        }
        x_ConvertByteToInt(graph);
    }
    CInt_graph& data = g.SetInt();
    data.SetMax(max(data.GetMax(), int(max_value)));
    CInt_graph::TValues& values = data.SetValues();
    values.insert(values.end(), ptr, ptr+add);
}


inline
void x_AddEmptyCoverage(CSeq_graph& graph, TSeqPos pos, TSeqPos end)
{
    TSeqPos add = x_AddCoverageLoc(graph, pos, end);
    x_AddCoverageZeroes(graph, add);
}


inline
void x_AddCoverage(CSeq_graph& graph,
                   CRange<TSeqPos> range,
                   CSNPDbPageIterator& page_it)
{
    TSeqPos add = x_AddCoverageLoc(graph, range.GetFrom(), range.GetToOpen());
    TSeqPos off = (range.GetFrom()-page_it.GetPagePos())/kCoverageZoom;
    x_AddCoverageValues(graph, add, page_it.GetCoverageValues().data()+off);
}


END_LOCAL_NAMESPACE;


CRef<CSeq_graph>
CSNPDbSeqIterator::GetCoverageGraph(CRange<TSeqPos> range,
                                    const SFilter& /*filter*/) const
{
    CRef<CSeq_graph> graph = x_NewCoverageGraph(*this);
    x_AdjustCoverageGraphRange(range, *this);
    for ( CSNPDbPageIterator it(*this, range, eSearchByStart); it; ++it ) {
        CRange<TSeqPos> page_range = range.IntersectionWith(it.GetPageRange());
        _ASSERT(!page_range.Empty());
        if ( page_range.GetFrom() != range.GetFrom() ) {
            x_AddEmptyCoverage(*graph, range.GetFrom(), page_range.GetFrom());
        }
        x_AddCoverage(*graph, page_range, it);
        range.SetFrom(page_range.GetToOpen());
    }
    if ( graph->GetNumval() == 0 ) {
        return null;
    }
    return graph;
}


CRef<CSeq_annot>
CSNPDbSeqIterator::GetCoverageAnnot(CRange<TSeqPos> range,
                                    const SFilter& /*filter*/,
                                    TFlags flags) const
{
    CRef<CSeq_annot> annot = x_NewAnnot();
    x_AdjustCoverageGraphRange(range, *this);
    CSeq_annot::TData::TGraph& graphs = annot->SetData().SetGraph();
    CRef<CSeq_graph> graph = x_NewCoverageGraph(*this);
    graphs.push_back(graph);
    for ( CSNPDbPageIterator it(*this, range, eSearchByStart); it; ++it ) {
        CRange<TSeqPos> page_range = range.IntersectionWith(it.GetPageRange());
        _ASSERT(!page_range.Empty());
        if ( page_range.GetFrom() != range.GetFrom() &&
             graph->GetNumval() != 0) {
            graph = x_NewCoverageGraph(*this);
            graphs.push_back(graph);
        }
        x_AddCoverage(*graph, page_range, it);
        range.SetFrom(page_range.GetToOpen());
    }
    if ( graph->GetNumval() == 0 ) {
        graphs.pop_back();
    }
    if ( graphs.empty() ) {
        return null;
    }
    return annot;
}


CRef<CSeq_annot>
CSNPDbSeqIterator::GetFeatAnnot(CRange<TSeqPos> range,
                                const SFilter& filter,
                                TFlags flags) const
{
    CRef<CSeq_annot> annot = x_NewAnnot();
    x_AdjustRange(range, *this);
    CSeq_annot::TData::TFtable& feats = annot->SetData().SetFtable();
    SSelector sel(eSearchByStart, filter);
    for ( CSNPDbFeatIterator it(*this, range, sel); it; ++it ) {
        feats.push_back(it.GetSeq_feat());
    }
    if ( feats.empty() ) {
        return null;
    }
    return annot;
}


BEGIN_LOCAL_NAMESPACE;


CRef<CSeqTable_column> x_MakeColumn(CSeqTable_column_info::EField_id id,
                                    const char* name = 0)
{
    CRef<CSeqTable_column> col(new CSeqTable_column);
    col->SetHeader().SetField_id(id);
    if ( name ) {
        col->SetHeader().SetField_name(name);
    }
    return col;
}


struct SColumn
{
    int id;
    const char* name;

    CRef<CSeqTable_column> column;

    SColumn(void)
        : id(-1),
          name(0)
        {
        }
    explicit
    SColumn(CSeqTable_column_info::EField_id id,
            const char* name = 0)
        : id(id),
          name(name)
        {
        }

    void Init(CSeqTable_column_info::EField_id id,
              const char* name = 0)
        {
            this->id = id;
            this->name = name;
        }

    CSeqTable_column* x_GetColumn(void)
        {
            if ( !column ) {
                _ASSERT(id >= 0);
                column =
                    x_MakeColumn(CSeqTable_column_info::EField_id(id), name);
            }
            return column;
        }
    CRef<CSeqTable_column> GetColumn(void)
        {
            return Ref(x_GetColumn());
        }

    void Attach(CSeq_table& table)
        {
            if ( column ) {
                table.SetColumns().push_back(column);
            }
        }

    DECLARE_OPERATOR_BOOL_REF(column);
};

struct SIntColumn : public SColumn
{
    CSeqTable_multi_data::TInt* values;

    explicit
    SIntColumn(CSeqTable_column_info::EField_id id, const char* name = 0)
        : SColumn(id, name),
          values(0)
        {
        }

    void Add(int value)
        {
            if ( !values ) {
                values = &x_GetColumn()->SetData().SetInt();
            }
            values->push_back(value);
        }
};


struct SInt8Column : public SIntColumn
{
    CSeqTable_multi_data::TInt8* values8;

    explicit
    SInt8Column(CSeqTable_column_info::EField_id id, const char* name = 0)
        : SIntColumn(id, name),
          values8(0)
        {
        }

    void Add(Int8 value)
        {
            if ( !values8 && int(value) == value ) {
                SIntColumn::Add(int(value));
            }
            else {
                if ( !values8 ) {
                    CSeqTable_column* col = x_GetColumn();
                    if ( col->IsSetData() ) {
                        col->SetData().ChangeToInt8();
                    }
                    values8 = &col->SetData().SetInt8();
                }
                values8->push_back(value);
            }
        }
};


struct SSparseIndex
{
    SColumn& column;
    CSeqTable_sparse_index::TIndexes* indexes;
    int size;

    SSparseIndex(SColumn& column)
        : column(column),
          indexes(0),
          size(0)
        {
        }
    
    void Add(int index)
        {
            if ( index != size ) {
                indexes = &column.x_GetColumn()->SetSparse().SetIndexes();
                for ( int i = 0; i < size; ++i ) {
                    indexes->push_back(i);
                }
            }
            if ( indexes ) {
                indexes->push_back(index);
            }
            ++size;
        }
};


struct SCommonStrings : public SColumn
{
    CCommonString_table::TStrings* values;
    CCommonString_table::TIndexes* indexes;
    typedef unordered_map<CTempString, int> TIndex;
    TIndex index;

    SCommonStrings(void)
        : values(0),
          indexes(0)
        {
        }

    void Add(CTempString val)
        {
            if ( !values ) {
                CSeqTable_column* col = x_GetColumn();
                values = &col->SetData().SetCommon_string().SetStrings();
                indexes = &col->SetData().SetCommon_string().SetIndexes();
            }
            int ind;
            TIndex::const_iterator it = index.find(val);
            if ( it == index.end() ) {
                ind = int(values->size());
                values->push_back(val);
                val = values->back();
                index.insert(TIndex::value_type(val, ind));
            }
            else {
                ind = it->second;
            }
            indexes->push_back(ind);
        }

    void Attach(CSeq_table& table)
        {
            if ( values && values->size() == 1 ) {
                CSeqTable_column* col = x_GetColumn();
                col->SetDefault().SetString().swap(values->front());
                col->ResetData();
                values = 0;
                indexes = 0;
            }
            SColumn::Attach(table);
        }
};


struct SCommon8Bytes : public SColumn
{
    CCommonBytes_table::TBytes* values;
    CCommonBytes_table::TIndexes* indexes;
    typedef map<Uint8, size_t> TIndex;
    TIndex index;

    explicit
    SCommon8Bytes(CSeqTable_column_info::EField_id id,
                  const char* name = 0)
        : SColumn(id, name),
          values(0),
          indexes(0)
        {
        }

    void Add(Uint8 val)
        {
            if ( !values ) {
                CSeqTable_column* col = x_GetColumn();
                values = &col->SetData().SetCommon_bytes().SetBytes();
                indexes = &col->SetData().SetCommon_bytes().SetIndexes();
            }
            pair<TIndex::iterator, bool> ins =
                index.insert(TIndex::value_type(val, 0));
            if ( ins.second ) {
                ins.first->second = values->size();
                vector<char>* data = 
                    new vector<char>(reinterpret_cast<const char*>(&val),
                                     reinterpret_cast<const char*>(&val+1));
                values->push_back(data);
            }
            indexes->push_back(ins.first->second);
        }

    void Attach(CSeq_table& table)
        {
            if ( values && values->size() == 1 ) {
                CSeqTable_column* col = x_GetColumn();
                col->SetDefault().SetBytes().swap(*values->front());
                col->ResetData();
                values = 0;
                indexes = 0;
            }
            SColumn::Attach(table);
        }
};


static const int kMaxTableAlleles = 4;


struct SSeqTableContent
{
    SSeqTableContent(void);

    void Add(const CSNPDbFeatIterator& it);

    CRef<CSeq_annot> GetAnnot(CSeq_id& seq_id);

    int m_TableSize;

    // columns
    SIntColumn col_from;
    SIntColumn col_to;
    SSparseIndex ind_to;

    SCommonStrings col_alleles[kMaxTableAlleles];

    SCommon8Bytes col_qa;

    SInt8Column col_dbxref;

    static void AddFixedString(CSeq_table& table,
                               CSeqTable_column_info::EField_id id,
                               const string& value)
        {
            CRef<CSeqTable_column> col = x_MakeColumn(id);
            col->SetDefault().SetString(value);
            table.SetColumns().push_back(col);
        }

    static void AddFixedSeq_id(CSeq_table& table,
                               CSeqTable_column_info::EField_id id,
                               CSeq_id& value)
        {
            CRef<CSeqTable_column> col = x_MakeColumn(id);
            col->SetDefault().SetId(value);
            table.SetColumns().push_back(col);
        }
};


SSeqTableContent::SSeqTableContent(void)
    : m_TableSize(0),
      col_from(CSeqTable_column_info::eField_id_location_from),
      col_to(CSeqTable_column_info::eField_id_location_to),
      ind_to(col_to),
      col_qa(CSeqTable_column_info::eField_id_ext, "E.QualityCodes"),
      col_dbxref(CSeqTable_column_info::eField_id_dbxref, "D.dbSNP")
{
    for ( int i = 0; i < kMaxTableAlleles; ++i ) {
        col_alleles[i].Init(CSeqTable_column_info::eField_id_qual,
                            "Q.replace");
    }
}


inline
void SSeqTableContent::Add(const CSNPDbFeatIterator& it)
{
    TSeqPos from = it.GetSNPPosition();
    TSeqPos len = it.GetSNPLength();
    CTempString alleles = it.GetAlleles();

    col_from.Add(from);
    if ( len != 1 ) {
        col_to.Add(from + len - 1);
        ind_to.Add(m_TableSize);
    }

    SIZE_TYPE pos = 0;
    for ( int i = 0; ; ++i ) {
        _ASSERT(i < kMaxTableAlleles);
        SIZE_TYPE end = alleles.find(kAlleleSeparator, pos);
        SIZE_TYPE len = (end == NPOS? alleles.size(): end) - pos;
        col_alleles[i].Add(alleles.substr(pos, len));
        if ( end == NPOS ) {
            break;
        }
        pos = end+1;
    }

    col_qa.Add(it.GetQualityCodes());

    col_dbxref.Add(it.GetFeatId());

    ++m_TableSize;
}


CRef<CSeq_annot> SSeqTableContent::GetAnnot(CSeq_id& seq_id)
{
    if ( !m_TableSize ) {
        return null;
    }
    
    CRef<CSeq_annot> table_annot = x_NewAnnot();
    
    CSeq_table& table = table_annot->SetData().SetSeq_table();
    table.SetFeat_type(CSeqFeatData::e_Imp);
    table.SetFeat_subtype(CSeqFeatData::eSubtype_variation);
    table.SetNum_rows(m_TableSize);

    AddFixedString(table,
                   CSeqTable_column_info::eField_id_data_imp_key,
                   "variation");

    AddFixedSeq_id(table,
               CSeqTable_column_info::eField_id_location_id,
               seq_id);

    col_from.Attach(table);
    col_to.Attach(table);
    for ( int i = 0; i < kMaxTableAlleles; ++i ) {
        col_alleles[i].Attach(table);
    }

    if ( col_qa ) {
        AddFixedString(table,
                       CSeqTable_column_info::eField_id_ext_type,
                       "dbSnpQAdata");
        col_qa.Attach(table);
    }

    col_dbxref.Attach(table);

    return table_annot;
}


struct SSeqTableConverter
{
    SSeqTableConverter(const CSNPDbSeqIterator& it);
    
    bool AddToTable(const CSNPDbFeatIterator& it);

    SSeqTableContent m_Tables[2][kMaxTableAlleles];

    void Add(const CSNPDbFeatIterator& it);

    vector< CRef<CSeq_annot> > GetAnnots(void);

    CRef<CSeq_id> m_Seq_id;
    CRef<CSeq_annot> m_RegularAnnot;
};


SSeqTableConverter::SSeqTableConverter(const CSNPDbSeqIterator& it)
    : m_Seq_id(it.GetSeqId())
{
}


vector< CRef<CSeq_annot> > SSeqTableConverter::GetAnnots(void)
{
    vector< CRef<CSeq_annot> > ret;
    for ( int k = 0; k < 2; ++k ) {
        for ( int i = 0; i < kMaxTableAlleles; ++i ) {
            if ( CRef<CSeq_annot> annot = m_Tables[k][i].GetAnnot(*m_Seq_id) ) {
                ret.push_back(annot);
            }
        }
    }
    if ( m_RegularAnnot ) {
        ret.push_back(m_RegularAnnot);
    }
    return ret;
}


inline
bool SSeqTableConverter::AddToTable(const CSNPDbFeatIterator& it)
{
    CTempString alleles = it.GetAlleles();
    int last_allele = count(alleles.begin(), alleles.end(), kAlleleSeparator);
    if ( last_allele >= kMaxTableAlleles ) {
        return false;
    }
    m_Tables[it.GetSNPLength() != 1][last_allele].Add(it);
    return true;
}


inline
void SSeqTableConverter::Add(const CSNPDbFeatIterator& it)
{
    if ( AddToTable(it) ) {
        return;
    }
    if ( !m_RegularAnnot ) {
        m_RegularAnnot = x_NewAnnot();
    }
    m_RegularAnnot->SetData().SetFtable().push_back(it.GetSeq_feat());
}


END_LOCAL_NAMESPACE;


CSNPDbSeqIterator::TAnnotSet
CSNPDbSeqIterator::GetTableFeatAnnots(CRange<TSeqPos> range,
                                      const SFilter& filter,
                                      TFlags flags) const
{
    x_AdjustRange(range, *this);
    SSeqTableConverter cvt(*this);
    SSelector sel(eSearchByStart, filter);
    for ( CSNPDbFeatIterator it(*this, range, sel); it; ++it ) {
        cvt.Add(it);
    }
    return cvt.GetAnnots();
}


BEGIN_LOCAL_NAMESPACE;


inline
void x_InitSNP_Info(SSNP_Info& info)
{
    info.m_Flags = info.fQualityCodesOs | info.fAlleleReplace;
    info.m_CommentIndex = info.kNo_CommentIndex;
    info.m_Weight = 0;
    info.m_ExtraIndex = info.kNo_ExtraIndex;
}


inline
bool x_ParseSNP_Info(SSNP_Info& info,
                     const CSNPDbFeatIterator& it,
                     CSeq_annot_SNP_Info& packed)
{
    TSeqPos len = it.GetSNPLength();
    if ( len > info.kMax_PositionDelta+1 ) {
        return false;
    }
    info.m_PositionDelta = len-1;
    info.m_ToPosition = it.GetSNPPosition()+len-1;

    CTempString all_alleles = it.GetAlleles();
    size_t index = 0;
    for ( ; !all_alleles.empty(); ++index ) {
        if ( index == info.kMax_AllelesCount ) {
            return false;
        }

        CTempString allele;
        SIZE_TYPE div = all_alleles.find(kAlleleSeparator);
        if ( div == NPOS ) {
            allele = all_alleles;
            all_alleles.clear();
        }
        else {
            allele = all_alleles.substr(0, div);
            all_alleles = all_alleles.substr(div+1);
        }
        if ( allele.size() > kMax_AlleleLength ) {
            return false;
        }
        SSNP_Info::TAlleleIndex a_index = packed.x_GetAlleleIndex(allele);
        if ( a_index == info.kNo_AlleleIndex ) {
            return false;
        }
        info.m_AllelesIndices[index] = a_index;
    }
    for ( ; index < info.kMax_AllelesCount; ++index ) {
        info.m_AllelesIndices[index] = info.kNo_AlleleIndex;
    }

    vector<char> q;
    it.GetQualityCodes(q);
    info.m_QualityCodesIndex = packed.x_GetQualityCodesIndex(q);
    if ( info.m_QualityCodesIndex == info.kNo_QualityCodesIndex ) {
        return false;
    }

    info.m_SNP_Id = it.GetFeatId();

    packed.x_AddSNP(info);
    return true;
}        


END_LOCAL_NAMESPACE;


CSNPDbSeqIterator::TPackedAnnot
CSNPDbSeqIterator::GetPackedFeatAnnot(CRange<TSeqPos> range,
                                      const SFilter& filter,
                                      TFlags flags) const
{
    x_AdjustRange(range, *this);
    CRef<CSeq_annot> annot = x_NewAnnot();
    CRef<CSeq_annot_SNP_Info> packed(new CSeq_annot_SNP_Info);
    CSeq_annot::TData::TFtable& feats = annot->SetData().SetFtable();

    SSNP_Info info;
    x_InitSNP_Info(info);
    SSelector sel(eSearchByStart, filter);
    for ( CSNPDbFeatIterator it(*this, range, sel); it; ++it ) {
        if ( !x_ParseSNP_Info(info, it, *packed) ) {
            feats.push_back(it.GetSeq_feat());
        }
    }
    if ( packed->empty() ) {
        packed = null;
        if ( feats.empty() ) {
            annot = null;
        }
    }
    else {
        packed->SetSeq_id(*GetSeqId());
    }
    return TPackedAnnot(annot, packed);
}


/////////////////////////////////////////////////////////////////////////////
// CSNPDbPageIterator
/////////////////////////////////////////////////////////////////////////////


void CSNPDbPageIterator::Reset(void)
{
    if ( m_Cur ) {
        GetDb().Put(m_Cur, m_CurrPageRowId);
        _ASSERT(!m_Cur);
    }
    m_SeqIter.Reset();
    m_CurrPagePos = kInvalidSeqPos;
}


CSNPDbPageIterator::CSNPDbPageIterator(void)
    : m_CurrPageSet(0),
      m_CurrPageRowId(0),
      m_CurrPagePos(kInvalidSeqPos),
      m_SearchMode(eSearchByOverlap)
{
}


CSNPDbPageIterator::CSNPDbPageIterator(const CSNPDb& db,
                                       const CSeq_id_Handle& ref_id,
                                       TSeqPos ref_pos,
                                       TSeqPos window,
                                       ESearchMode search_mode)
    : m_SeqIter(db, ref_id)
{
    TSeqPos ref_end = window? ref_pos+window: kInvalidSeqPos;
    Select(COpenRange<TSeqPos>(ref_pos, ref_end), search_mode);
}


CSNPDbPageIterator::CSNPDbPageIterator(const CSNPDb& db,
                                       const CSeq_id_Handle& ref_id,
                                       COpenRange<TSeqPos> range,
                                       ESearchMode search_mode)
    : m_SeqIter(db, ref_id)
{
    Select(range, search_mode);
}


CSNPDbPageIterator::CSNPDbPageIterator(const CSNPDbSeqIterator& seq,
                                       COpenRange<TSeqPos> range,
                                       ESearchMode search_mode)
    : m_SeqIter(seq)
{
    Select(range, search_mode);
}


CSNPDbPageIterator::CSNPDbPageIterator(const CSNPDbPageIterator& iter)
{
    *this = iter;
}


CSNPDbPageIterator&
CSNPDbPageIterator::operator=(const CSNPDbPageIterator& iter)
{
    if ( this != &iter ) {
        Reset();
        m_SeqIter = iter.m_SeqIter;
        m_Cur = iter.m_Cur;
        m_SearchRange = iter.m_SearchRange;
        m_CurrPageSet = iter.m_CurrPageSet;
        m_CurrPageRowId = iter.m_CurrPageRowId;
        m_CurrPagePos = iter.m_CurrPagePos;
        m_SearchMode = iter.m_SearchMode;
    }
    return *this;
}


CSNPDbPageIterator::~CSNPDbPageIterator(void)
{
    Reset();
}


CSNPDbPageIterator&
CSNPDbPageIterator::Select(COpenRange<TSeqPos> ref_range,
                           ESearchMode search_mode)
{
    m_SearchRange = ref_range;
    m_SearchMode = search_mode;

    if ( !m_SeqIter || ref_range.Empty() ) {
        m_CurrPagePos = kInvalidSeqPos;
        return *this;
    }
    
    if ( !m_Cur ) {
        m_Cur = GetDb().SNP(m_CurrPageRowId);
    }

    TSeqPos pos = ref_range.GetFrom();
    if ( m_SearchMode == eSearchByOverlap ) {
        // SNP may start before requested position
        pos = pos < kMaxSNPLength? 0: pos - (kMaxSNPLength-1);
    }

    const CSNPDb_Impl::SSeqInfo::TPageSets& psets = m_SeqIter->m_PageSets;
    for ( m_CurrPageSet = 0; m_CurrPageSet < psets.size(); ++m_CurrPageSet ) {
        const CSNPDb_Impl::SSeqInfo::SPageSet& pset = psets[m_CurrPageSet];
        TSeqPos skip = pos<pset.m_SeqPos? 0: (pos-pset.m_SeqPos)/kPageSize;
        if ( skip < pset.m_PageCount ) {
            m_CurrPageRowId = pset.m_RowId + skip;
            m_CurrPagePos = pset.m_SeqPos + skip * kPageSize;
            return *this;
        }
    }
    m_CurrPageRowId = TVDBRowId(-1);
    m_CurrPagePos = kInvalidSeqPos;
    return *this;
}


void CSNPDbPageIterator::x_Next(void)
{
    x_CheckValid("CSNPDbPageIterator::operator++");

    const CSNPDb_Impl::SSeqInfo::TPageSets& psets = m_SeqIter->m_PageSets;
    if ( ++m_CurrPageRowId < psets[m_CurrPageSet].GetRowIdEnd() ) {
        // next page in the set
        m_CurrPagePos += kPageSize;
        return;
    }
    
    // no more pages in the set, next page set
    if ( ++m_CurrPageSet < psets.size() ) {
        // first page in the next set
        m_CurrPageRowId = psets[m_CurrPageSet].m_RowId;
        m_CurrPagePos = psets[m_CurrPageSet].m_SeqPos;
        return;
    }
    
    // no more page sets
    m_CurrPagePos = kInvalidSeqPos;
}


CRange<TSeqPos> CSNPDbPageIterator::GetPageRange(void) const
{
    TSeqPos pos = GetPagePos();
    return COpenRange<TSeqPos>(pos, pos+GetPageSize());
}


void CSNPDbPageIterator::x_ReportInvalid(const char* method) const
{
    NCBI_THROW_FMT(CSraException, eInvalidState,
                   "CSNPDbPageIterator::"<<method<<"(): "
                   "Invalid iterator state");
}


Uint4 CSNPDbPageIterator::GetFeatCount(void) const
{
    x_CheckValid("CSNPDbPageIterator::GetFeatCount");
    return *Cur().FEAT_COUNT(GetPageRowId());
}


CTempString CSNPDbPageIterator::GetFeatType(void) const
{
    x_CheckValid("CSNPDbPageIterator::GetFeatType");
    return *Cur().FEAT_TYPE(GetPageRowId());
}


CVDBValueFor<Uint4> CSNPDbPageIterator::GetCoverageValues(void) const
{
    x_CheckValid("CSNPDbPageIterator::GetCoverageValues");
    return Cur().FEAT_ZOOM_COUNT(GetPageRowId());
}


/////////////////////////////////////////////////////////////////////////////
// CSNPDbFeatIterator
/////////////////////////////////////////////////////////////////////////////


void CSNPDbFeatIterator::Reset(void)
{
    if ( m_Extra ) {
        GetDb().Put(m_Extra, m_ExtraRowId);
    }
    m_PageIter.Reset();
    m_CurrFeatId = m_FirstBadFeatId = 0;
}


inline
void CSNPDbFeatIterator::x_InitPage(void)
{
    m_CurrFeatId = 0;
    m_FirstBadFeatId = m_PageIter? *Cur().FEAT_COUNT(GetPageRowId()): 0;
}


CSNPDbFeatIterator::CSNPDbFeatIterator(void)
    : m_CurrFeatId(0),
      m_FirstBadFeatId(0)
{
}


CSNPDbFeatIterator::CSNPDbFeatIterator(const CSNPDb& db,
                                       const CSeq_id_Handle& ref_id,
                                       TSeqPos ref_pos,
                                       TSeqPos window,
                                       const SSelector& sel)
    : m_PageIter(db, ref_id, ref_pos, window, sel.m_SearchMode)
{
    x_SetFilter(sel);
    x_InitPage();
    x_Settle();
}


CSNPDbFeatIterator::CSNPDbFeatIterator(const CSNPDb& db,
                                       const CSeq_id_Handle& ref_id,
                                       COpenRange<TSeqPos> range,
                                       const SSelector& sel)
    : m_PageIter(db, ref_id, range, sel.m_SearchMode)
{
    x_SetFilter(sel);
    x_InitPage();
    x_Settle();
}


CSNPDbFeatIterator::CSNPDbFeatIterator(const CSNPDbSeqIterator& seq,
                                       COpenRange<TSeqPos> range,
                                       const SSelector& sel)
    : m_PageIter(seq, range, sel.m_SearchMode)
{
    x_SetFilter(sel);
    x_InitPage();
    x_Settle();
}


CSNPDbFeatIterator::CSNPDbFeatIterator(const CSNPDbFeatIterator& iter)
{
    *this = iter;
}


CSNPDbFeatIterator&
CSNPDbFeatIterator::operator=(const CSNPDbFeatIterator& iter)
{
    if ( this != &iter ) {
        Reset();
        m_PageIter = iter.m_PageIter;
        m_Extra = iter.m_Extra;
        m_ExtraRowId = iter.m_ExtraRowId;
        m_Filter = iter.m_Filter;
        m_CurRange = iter.m_CurRange;
        m_CurrFeatId = iter.m_CurrFeatId;
        m_FirstBadFeatId = iter.m_FirstBadFeatId;
    }
    return *this;
}


CSNPDbFeatIterator::~CSNPDbFeatIterator(void)
{
    Reset();
}


void CSNPDbFeatIterator::x_SetFilter(const SSelector& sel)
{
    m_Filter = sel.m_Filter;
    m_Filter.Normalize();
}


CSNPDbFeatIterator&
CSNPDbFeatIterator::Select(COpenRange<TSeqPos> ref_range,
                           const SSelector& sel)
{
    m_PageIter.Select(ref_range, sel.m_SearchMode);
    x_SetFilter(sel);
    x_InitPage();
    x_Settle();
    return *this;
}


TSeqPos CSNPDbFeatIterator::x_GetFrom(void) const
{
    return m_PageIter.GetPagePos()+Cur().FROM(GetPageRowId())[m_CurrFeatId];
}


TSeqPos CSNPDbFeatIterator::x_GetLength(void) const
{
    return Cur().LEN(GetPageRowId())[m_CurrFeatId];
}


inline
CSNPDbFeatIterator::EExcluded CSNPDbFeatIterator::x_Excluded(void)
{
    TSeqPos ref_pos = x_GetFrom();
    if ( ref_pos >= GetSearchRange().GetToOpen() ) {
        // no more
        return ePassedTheRegion;
    }
    TSeqPos ref_len = x_GetLength();
    TSeqPos ref_end = ref_pos + ref_len;
    if ( ref_end <= GetSearchRange().GetFrom() ) {
        return eExluded;
    }
    if ( m_Filter.IsSet() ) {
        if ( !m_Filter.Matches(GetQualityCodes()) ) {
            return eExluded;
        }
    }
    m_CurRange.SetFrom(ref_pos);
    m_CurRange.SetToOpen(ref_end);
    return eIncluded;
}


void CSNPDbFeatIterator::x_Settle(void)
{
    while ( m_PageIter ) {
        while ( m_CurrFeatId < m_FirstBadFeatId ) {
            EExcluded exc = x_Excluded();
            if ( exc == eIncluded ) {
                // found
                return;
            }
            if ( exc == ePassedTheRegion ) {
                // passed the region
                break;
            }
            // next feat in page
            ++m_CurrFeatId;
        }

        ++m_PageIter;
        x_InitPage();
    }
}


void CSNPDbFeatIterator::x_Next(void)
{
    x_CheckValid("CSNPDbFeatIterator::operator++");
    ++m_CurrFeatId;
    x_Settle();
}


void CSNPDbFeatIterator::x_ReportInvalid(const char* method) const
{
    NCBI_THROW_FMT(CSraException, eInvalidState,
                   "CSNPDbFeatIterator::"<<method<<"(): "
                   "Invalid iterator state");
}


Uint4 CSNPDbFeatIterator::GetFeatIdPrefix(void) const
{
    x_CheckValid("CSNPDbFeatIterator::GetFeatIdPrefix");
    return Cur().FEAT_ID_PREFIX(GetPageRowId())[m_CurrFeatId];
}


Uint8 CSNPDbFeatIterator::GetFeatId(void) const
{
    x_CheckValid("CSNPDbFeatIterator::GetFeatId");
    return Cur().FEAT_ID_VALUE(GetPageRowId())[m_CurrFeatId];
}


TVDBRowId CSNPDbFeatIterator::GetExtraRowId(void) const
{
    x_CheckValid("CSNPDbFeatIterator::GetExtraRowId");
    return Cur().EXTRA_ROW_NUM(GetPageRowId())[m_CurrFeatId];
}


CTempString CSNPDbFeatIterator::GetAlleles(void) const
{
    x_CheckValid("CSNPDbFeatIterator::GetAlleles");
    m_ExtraRowId = GetExtraRowId();
    if ( !m_Extra ) {
        m_Extra = GetDb().Extra(m_ExtraRowId);
        if ( !m_Extra ) {
            return CTempString();
        }
    }
    return *m_Extra->RS_ALLELES(m_ExtraRowId);
}


Uint8 CSNPDbFeatIterator::GetQualityCodes(void) const
{
    x_CheckValid("CSNPDbFeatIterator::GetQualityCodes");
    return Cur().BIT_FLAGS(GetPageRowId())[m_CurrFeatId];
}


void CSNPDbFeatIterator::GetQualityCodes(vector<char>& codes) const
{
    Uint8 data = GetQualityCodes();
    codes.assign(reinterpret_cast<const char*>(&data),
                 reinterpret_cast<const char*>(&data+1));
}


template<size_t ValueSize>
static inline
bool x_IsStringConstant(const string& str, const char (&value)[ValueSize])
{
    return str.size() == ValueSize-1 && str == value;
}

#define x_SetStringConstant(obj, Field, value)                          \
    if ( !(obj).NCBI_NAME2(IsSet,Field)() ||                            \
         !x_IsStringConstant((obj).NCBI_NAME2(Get,Field)(), value) ) {  \
        (obj).NCBI_NAME2(Set,Field)((value));                           \
    }


template<class T>
static inline
T& x_GetPrivate(CRef<T>& ref)
{
    T* ptr = ref.GetPointerOrNull();
    if ( !ptr || !ptr->ReferencedOnlyOnce() ) {
        ref = ptr = new T;
    }
    return *ptr;
}


struct CSNPDbFeatIterator::SCreateCache {
    CRef<CSeq_feat> m_Feat;
    CRef<CImp_feat> m_Imp;
    CRef<CSeq_interval> m_LocInt;
    CRef<CSeq_point> m_LocPnt;
    CRef<CGb_qual> m_Allele[4];
    CRef<CDbtag> m_Dbtag;
    CRef<CUser_object> m_Ext;
    CRef<CObject_id> m_ObjectIdQAdata;
    CRef<CObject_id> m_ObjectIdQualityCodes;
    CRef<CUser_field> m_QualityCodes;

#define ALLELE_CACHE
#ifdef ALLELE_CACHE
    CRef<CGb_qual> m_AlleleCache_empty;
    CRef<CGb_qual> m_AlleleCache_minus;
    CRef<CGb_qual> m_AlleleCacheA;
    CRef<CGb_qual> m_AlleleCacheC;
    CRef<CGb_qual> m_AlleleCacheG;
    CRef<CGb_qual> m_AlleleCacheT;
#endif

    CGb_qual& x_GetCommonAllele(CRef<CGb_qual>& cache, CTempString val)
        {
            CGb_qual* qual = cache.GetPointerOrNull();
            if ( !qual ) {
                cache = qual = new CGb_qual;
                qual->SetQual("replace");
                qual->SetVal(val);
            }
            return *qual;
        }
    CGb_qual& x_GetCachedAllele(CRef<CGb_qual>& cache, CTempString val)
        {
            CGb_qual& qual = x_GetPrivate(cache);
            x_SetStringConstant(qual, Qual, "replace");
            qual.SetVal(val);
            return qual;
        }
    CGb_qual& GetAllele(CRef<CGb_qual>& cache, CTempString val)
        {
#ifdef ALLELE_CACHE
            if ( val.size() == 1 ) {
                switch ( val[0] ) {
                case 'A': return x_GetCommonAllele(m_AlleleCacheA, val);
                case 'C': return x_GetCommonAllele(m_AlleleCacheC, val);
                case 'G': return x_GetCommonAllele(m_AlleleCacheG, val);
                case 'T': return x_GetCommonAllele(m_AlleleCacheT, val);
                case '-': return x_GetCommonAllele(m_AlleleCache_minus, val);
                default: break;
                }
            }
            if ( val.size() == 0 ) {
                return x_GetCommonAllele(m_AlleleCache_empty, val);
            }
#endif
            return x_GetCachedAllele(cache, val);
        }
};


inline
CSNPDbFeatIterator::SCreateCache& CSNPDbFeatIterator::x_GetCreateCache(void) const
{
    if ( !m_CreateCache ) {
        m_CreateCache = new SCreateCache;
    }
    return *m_CreateCache;
}


static inline
CObject_id& x_GetObject_id(CRef<CObject_id>& cache, const char* name)
{
    if ( !cache ) {
        cache = new CObject_id();
        cache->SetStr(name);
    }
    return *cache;
}


CRef<CSeq_feat> CSNPDbFeatIterator::GetSeq_feat(TFlags flags) const
{
    x_CheckValid("CSNPDbFeatIterator::GetSeq_feat");

    if ( !(flags & fUseSharedObjects) ) {
        m_CreateCache.reset();
    }
    SCreateCache& cache = x_GetCreateCache();
    CSeq_feat& feat = x_GetPrivate(cache.m_Feat);
    {{
        CSeqFeatData& data = feat.SetData();
        data.Reset();
        CImp_feat& imp = x_GetPrivate(cache.m_Imp);
        x_SetStringConstant(imp, Key, "variation");
        imp.ResetLoc();
        imp.ResetDescr();
        data.SetImp(imp);
    }}
    {{
        CSeq_loc& loc = feat.SetLocation();
        TSeqPos len = GetSNPLength();
        loc.Reset();
        if ( len == 1 ) {
            CSeq_point& loc_pnt = x_GetPrivate(cache.m_LocPnt);
            loc_pnt.SetId(*GetSeqId());
            TSeqPos pos = GetSNPPosition();
            loc_pnt.SetPoint(pos);
            loc.SetPnt(loc_pnt);
        }
        else {
            CSeq_interval& loc_int = x_GetPrivate(cache.m_LocInt);
            loc_int.SetId(*GetSeqId());
            TSeqPos pos = GetSNPPosition();
            loc_int.SetFrom(pos);
            loc_int.SetTo(pos+len-1);
            loc.SetInt(loc_int);
        }
    }}
    if ( flags & fIncludeAlleles ) {
        CSeq_feat::TQual& quals = feat.SetQual();
        NON_CONST_ITERATE ( CSeq_feat::TQual, it, quals ) {
            *it = null; // release old references to allow caching
        }
        CTempString all_alleles = GetAlleles();
        size_t index = 0;
        while ( !all_alleles.empty() ) {
            CTempString allele;
            SIZE_TYPE div = all_alleles.find(kAlleleSeparator);
            if ( div == NPOS ) {
                allele = all_alleles;
                all_alleles.clear();
            }
            else {
                allele = all_alleles.substr(0, div);
                all_alleles = all_alleles.substr(div+1);
            }
            size_t cache_index = min(index, ArraySize(cache.m_Allele)-1);
            CGb_qual& qual = cache.GetAllele(cache.m_Allele[cache_index], allele);
            if ( index < quals.size() ) {
                quals[index] = &qual;
            }
            else {
                quals.push_back(Ref(&qual));
            }
            ++index;
        }
        quals.resize(index);
    }
    else {
        feat.ResetQual();
    }
    if ( flags & fIncludeRsId ) {
        CSeq_feat::TDbxref& dbxref = feat.SetDbxref();
        dbxref.resize(1);
        dbxref[0] = null;
        CDbtag& dbtag = x_GetPrivate(cache.m_Dbtag);
        x_SetStringConstant(dbtag, Db, "dbSNP");
        Uint8 feat_id = GetFeatId();
        switch ( GetFeatIdPrefix() ) {
        case eFeatIdPrefix_rs:
            dbtag.SetTag().SetStr("rs"+NStr::NumericToString(feat_id));
            break;
        case eFeatIdPrefix_ss:
            dbtag.SetTag().SetStr("ss"+NStr::NumericToString(feat_id));
            break;
        default:
            dbtag.SetTag().SetId8(feat_id);
            break;
        }
        dbxref[0] = &dbtag;
    }
    else {
        feat.ResetDbxref();
    }
    feat.ResetExt();
    if ( flags & (fIncludeQualityCodes|fIncludeNeighbors) ) {
        CUser_object& ext = x_GetPrivate(cache.m_Ext);
        ext.SetType(x_GetObject_id(cache.m_ObjectIdQAdata,
                                   "dbSnpQAdata"));
        CUser_object::TData& data = ext.SetData();
        data.clear();
        if ( flags & fIncludeNeighbors ) {
        }
        if ( flags & fIncludeQualityCodes ) {
            CUser_field& field = x_GetPrivate(cache.m_QualityCodes);
            field.SetLabel(x_GetObject_id(cache.m_ObjectIdQualityCodes,
                                          "QualityCodes"));
            GetQualityCodes(field.SetData().SetOs());
            ext.SetData().push_back(Ref(&field));
        }
        feat.SetExt(ext);
    }
    return Ref(&feat);
}


/////////////////////////////////////////////////////////////////////////////


END_NAMESPACE(objects);
END_NCBI_NAMESPACE;
