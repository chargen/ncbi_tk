static char const rcsid[] = "$Id$";

/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
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
* ===========================================================================*/

/*****************************************************************************

File name: rps.cpp

Author: Jason Papadopoulos

Contents: Use RPS blast to find domain hits

******************************************************************************/

#include <ncbi_pch.hpp>
#include <algo/blast/api/blast_rps_options.hpp>
#include <algo/blast/api/db_blast.hpp>
#include <algo/blast/api/seqsrc_seqdb.hpp>
#include <algo/cobalt/cobalt.hpp>

/// @file rps.cpp
/// Use RPS blast to find domain hits

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(cobalt)

USING_SCOPE(blast);

/// Given an RPS blast database, load a list of block offsets
/// for each database sequence. The list is resident in a text
/// file, where each line is as follows
/// <pre>
/// [seq ID] [oid of block] [start block offset] [end block offset]
/// </pre>
/// Note that block offsets are zero-based
/// @param blockfile Name of file containing list of offsets [in]
/// @param blocklist the list of offsets read from file [out]
///
void
CMultiAligner::x_LoadBlockBoundaries(string blockfile,
                      vector<SSegmentLoc>& blocklist)
{
    CNcbiIfstream blockstream(blockfile.c_str());
    if (blockstream.bad() || blockstream.fail())
        NCBI_THROW(CBlastException, eInvalidArgument,
                   "Cannot open RPS blockfile");

    char buf[64];
    SSegmentLoc tmp;
    int oid = 0;
    int block_idx;
    int start, end;

    blockstream >> buf;
    blockstream >> block_idx;
    blockstream >> start;
    blockstream >> end;
    blocklist.push_back(SSegmentLoc(oid, start, end));

    while (!blockstream.eof()) {
        blockstream >> buf;
        blockstream >> block_idx;
        blockstream >> start;
        blockstream >> end;

        if (block_idx == 0)
            oid++;

        blocklist.push_back(SSegmentLoc(oid, start, end));
    }
}


void
CMultiAligner::x_RealignBlocks(CHitList& rps_hits,
                               vector<SSegmentLoc>& blocklist,
                               CProfileData& profile_data)
{
    m_Aligner.SetWg(kRpsScaleFactor * m_GapOpen);
    m_Aligner.SetWs(kRpsScaleFactor * m_GapExtend);
    m_Aligner.SetStartWg(kRpsScaleFactor / 2 * m_GapOpen);
    m_Aligner.SetStartWs(kRpsScaleFactor * m_GapExtend);
    m_Aligner.SetEndWg(kRpsScaleFactor / 2 * m_GapOpen);
    m_Aligner.SetEndWs(kRpsScaleFactor * m_GapExtend);
    m_Aligner.SetEndSpaceFree(false, false, false, false);

    for (int i = 0; i < rps_hits.Size(); i++) {

        CHit *hit = rps_hits.GetHit(i);
        CSequence& query = m_QueryData[hit->m_SeqIndex1];
        int db_seq = hit->m_SeqIndex2;
        int *db_seq_offsets = profile_data.GetSeqOffsets();
        int **pssm = profile_data.GetPssm() + db_seq_offsets[db_seq];
        int db_seq_length = db_seq_offsets[db_seq + 1] - db_seq_offsets[db_seq];
        int last_fudge = 0;

        _ASSERT(!(hit->HasSubHits()));

        // ignore this alignment if its extent is less than
        // 60% of the extent of query and DB sequence

        if ((hit->m_SeqRange1.GetLength() < 0.6 * query.GetLength()) &&
            (hit->m_SeqRange2.GetLength() < 0.6 * db_seq_length)) {
            rps_hits.SetKeepHit(i, false);
            continue;
        }

        SSegmentLoc target(db_seq, hit->m_SeqRange2.GetFrom(), 
                           hit->m_SeqRange2.GetTo());
    
        // locate the first block in the subject sequence
        // that contains a piece of the HSP
    
        vector<SSegmentLoc>::iterator 
                    itr = lower_bound(blocklist.begin(), blocklist.end(),
                                      target, compare_sseg_db_idx());

        _ASSERT(itr != blocklist.end() &&
               target.seq_index == itr->seq_index);
        while (itr != blocklist.end() &&
               itr->seq_index == target.seq_index &&
               itr->GetTo() < target.GetFrom()) {
            itr++;
        }

        vector<SSegmentLoc>::iterator prev_itr(itr);
        vector<SSegmentLoc>::iterator next_itr(itr);
        prev_itr--;
        next_itr++;
    
        // for each block that contains a portion of the
        // original alignment
    
        while (itr->seq_index == db_seq && itr->GetFrom() < target.GetTo()) {
    
            const int kMaxFudge = 6;
            TRange q_range, new_s_range; 
            TRange tback_range;

            // calculate the offsets into the subject sequence
            // that correspond to the current block
    
            TRange s_range(itr->range.IntersectionWith(target.range));
            _ASSERT(!s_range.Empty() && itr->range.Contains(s_range));
    
            int left_fudge, right_fudge;

            if (itr == blocklist.begin() || 
                prev_itr == blocklist.begin() ||
                prev_itr->seq_index != db_seq) {
                left_fudge = 0;
            }
            else {
                left_fudge = s_range.GetFrom() - 
                        prev_itr->GetTo() - last_fudge - 1;
                left_fudge = min(left_fudge, kMaxFudge);
            }

            if (itr == blocklist.end() || 
                next_itr == blocklist.end() ||
                next_itr->seq_index != db_seq) {
                right_fudge = 0;
            }
            else {
                right_fudge = (next_itr->GetFrom() - s_range.GetTo() - 1) / 2;
                right_fudge = min(right_fudge, kMaxFudge);
            }

            last_fudge = right_fudge;

            // compute the start and stop offsets into the
            // query sequence that correspond to the subject range
            // specified by the current block.
    
            hit->GetRangeFromSeq2(s_range, q_range, new_s_range, tback_range);

            itr++;
            prev_itr++;
            next_itr++;
    
            // Throw away alignments whose query range is two
            // letters or less
    
            if (q_range.GetLength() <= CHit::kMinHitSize)
                continue;
    
            if (s_range.GetLength() > 3 * q_range.GetLength() / 2) {
                printf("ignore aligning query %d %d-%d db %d block %d-%d\n",
                       hit->m_SeqIndex1, q_range.GetFrom(), q_range.GetTo(),
                       db_seq, s_range.GetFrom(), s_range.GetTo());
                continue;
            }

            q_range.SetFrom(max(hit->m_SeqRange1.GetFrom(),
                                q_range.GetFrom() - left_fudge));
            q_range.SetTo(min(hit->m_SeqRange1.GetTo(),
                              q_range.GetTo() + right_fudge));

            // Now realign the block to the query sequence
            m_Aligner.SetSequences((const int **)(pssm + s_range.GetFrom()),
                          s_range.GetLength(),
                          (const char *)query.GetSequence() + q_range.GetFrom(),
                          q_range.GetLength());
    
            int score = m_Aligner.Run();
            const CNWAligner::TTranscript tback(m_Aligner.GetTranscript(false));
            int tback_size = tback.size();
            CEditScript final_script;

            if ((tback[0] == CNWAligner::eTS_Delete &&
                 tback[tback_size-1] == CNWAligner::eTS_Insert) ||
                (tback[0] == CNWAligner::eTS_Insert &&
                 tback[tback_size-1] == CNWAligner::eTS_Delete)) {

                // The query region falls outside the DB region.
                // Throw away the alignment and reuse the original one.

                hit->GetRangeFromSeq2(s_range, q_range, s_range, tback_range);

                // throw away alignments that are mostly gaps

                if (q_range.GetLength() <= CHit::kMinHitSize || 
                    s_range.GetLength() <= CHit::kMinHitSize)
                    continue;
                score = hit->GetEditScript().GetScore(
                               tback_range, 
                               TOffsetPair(hit->m_SeqRange1.GetFrom(),
                                           hit->m_SeqRange2.GetFrom()),
                               query, pssm,
                               m_Aligner.GetWg(), m_Aligner.GetWs());
                final_script = hit->GetEditScript().MakeEditScript(tback_range);
            }
            else {

                // strip off leading and trailing gaps in the
                // database sequence. Modify the alignment score
                // accordingly

                int first_tback = 0;
                int last_tback = tback_size - 1;
                int q_start = q_range.GetFrom();
                int q_stop = q_range.GetTo();
                int s_start = s_range.GetFrom();
                int s_stop = s_range.GetTo();

                for (int k = 0; k < tback_size && 
                            tback[k] != CNWAligner::eTS_Match; k++) {
                    first_tback++;
                    if (tback[k] == CNWAligner::eTS_Delete)
                        s_start++;
                    else if (tback[k] == CNWAligner::eTS_Insert)
                        q_start++;

                    score -= m_Aligner.GetWs();
                    if (k == 0)
                        score -= m_Aligner.GetEndWg();
                    else if (tback[k] != tback[k-1])
                        score -= m_Aligner.GetWg();
                }

                for (int k = tback_size - 1; k >= 0 && 
                                     tback[k] != CNWAligner::eTS_Match; k--) {
                    last_tback--;
                    if (tback[k] == CNWAligner::eTS_Delete)
                        s_stop--;
                    else if (tback[k] == CNWAligner::eTS_Insert)
                        q_stop--;

                    score -= m_Aligner.GetWs();
                    if (k == tback_size - 1)
                        score -= m_Aligner.GetEndWg();
                    else if (tback[k] != tback[k+1])
                        score -= m_Aligner.GetWg();
                }

                q_range.Set(q_start, q_stop);
                s_range.Set(s_start, s_stop);
                if (q_range.GetLength() <= CHit::kMinHitSize || 
                    s_range.GetLength() <= CHit::kMinHitSize)
                    continue;

                _ASSERT(tback[first_tback] == CNWAligner::eTS_Match);
                _ASSERT(tback[last_tback] == CNWAligner::eTS_Match);

                final_script = CEditScript::MakeEditScript(tback, 
                                 TRange(first_tback, last_tback));
            }

            hit->InsertSubHit(new CHit(hit->m_SeqIndex1,
                                       hit->m_SeqIndex2,
                                       q_range, s_range,
                                       score, final_script));
        }

        if (hit->HasSubHits()) {
            hit->ResolveSubHitConflicts(query, pssm,
                                        m_Aligner.GetWg(), 
                                        m_Aligner.GetWs());
            hit->AddUpSubHits();
        }
        else {
            rps_hits.SetKeepHit(i, false);
        }
    }

    rps_hits.PurgeUnwantedHits();
    m_Aligner.SetWg(m_GapOpen);
    m_Aligner.SetWs(m_GapExtend);
    m_Aligner.SetStartWg(m_EndGapOpen);
    m_Aligner.SetStartWs(m_EndGapExtend);
    m_Aligner.SetEndWg(m_EndGapOpen);
    m_Aligner.SetEndWs(m_EndGapExtend);
}


void
CMultiAligner::x_FindRPSHits(CHitList& rps_hits)
{
    int num_queries = m_tQueries.size();

    CBlastRPSOptionsHandle rps_opts;

    // deliberately set the cutoff e-value too high
    rps_opts.SetEvalueThreshold(max(m_RPSEvalue, 10.0));
    rps_opts.SetSegFiltering(false);

    CBlastSeqSrc seq_src(SeqDbBlastSeqSrcInit(m_RPSdb.c_str(), TRUE));
    CDbBlast blaster(m_tQueries, seq_src, rps_opts);
    blaster.PartialRun();

    BlastHSPResults *blast_results = blaster.GetResults();
    _ASSERT(blast_results);
    for (int i = 0; i < num_queries; i++) {
        BlastHitList *hitlist = blast_results->hitlist_array[i];
        if (hitlist == NULL)
            continue;

        for (int j = 0; j < hitlist->hsplist_count; j++) {
            BlastHSPList *hsplist = hitlist->hsplist_array[j];
            _ASSERT(hsplist != NULL);
            
            for (int k = 0; k < hsplist->hspcnt; k++) {

                // delete HSPs whose e-value exceeds the cutoff
                // that was specified. We do *not* give this cutoff
                // directly to the blast engine because sometimes 
                // the ungapped version of an alignment does not
                // exceed the ungapped cutoff score, even though
                // the gapped version of the alignment would

                BlastHSP *hsp = hsplist->hsp_array[k];
                if (hsp->evalue <= m_RPSEvalue) {
                    rps_hits.AddToHitList(new CHit(i, hsplist->oid, hsp));
                }
            }
        }
    }

    //-------------------------------------------------------
    if (m_Verbose) {
        printf("RPS hits:\n");
        for (int i = 0; i < rps_hits.Size(); i++) {
            CHit *hit = rps_hits.GetHit(i);
            printf("query %d %4d - %4d db %d %4d - %4d score %d\n",
                         hit->m_SeqIndex1, 
                         hit->m_SeqRange1.GetFrom(), 
                         hit->m_SeqRange1.GetTo(),
                         hit->m_SeqIndex2,
                         hit->m_SeqRange2.GetFrom(), 
                         hit->m_SeqRange2.GetTo(),
                         hit->m_Score);
        }
        printf("\n\n");
    }
    //-------------------------------------------------------
}


void 
CMultiAligner::x_AssignRPSResFreqs(CHitList& rps_hits,
                                   CProfileData& profile_data)
{
    if (rps_hits.Empty()) {
        return;
    }

    rps_hits.SortByScore();

    for (int i = 0; i < rps_hits.Size(); i++) {
        CHit *hit = rps_hits.GetHit(i);

        _ASSERT(hit->HasSubHits());

        // skip hit i if it overlaps on the query sequence
        // with a higher-scoring HSP.

        int j;
        for (j = 0; j < i; j++) {
            CHit *better_hit = rps_hits.GetHit(j);

            if (better_hit->m_SeqIndex1 != hit->m_SeqIndex1)
                continue;

            if (rps_hits.GetKeepHit(j) == true &&
                better_hit->m_SeqRange1.IntersectingWith(hit->m_SeqRange1))
                break;
        }
        if (j < i) {
            continue;
        }

        // The hit does not conflict; use the traceback of each block
        // to locate each position where a substitution occurs,
        // and assign the appropriate column of residue frequencies
        // at that position

        CSequence& query = m_QueryData[hit->m_SeqIndex1];
        CSequence::TFreqMatrix& matrix = query.GetFreqs();

        double **ref_freqs = profile_data.GetResFreqs() + 
                             (profile_data.GetSeqOffsets())[hit->m_SeqIndex2];

        NON_CONST_ITERATE(vector<CHit *>, itr, hit->GetSubHit()) {
            CHit *subhit = *itr;
            vector<TOffsetPair> sub_list(
                         subhit->GetEditScript().ListMatchRegions(
                                 TOffsetPair(subhit->m_SeqRange1.GetFrom(),
                                             subhit->m_SeqRange2.GetFrom()) ));

            for (j = 0; j < (int)sub_list.size(); j += 2) {
                TOffsetPair& start_pair(sub_list[j]);
                TOffsetPair& stop_pair(sub_list[j+1]);
                int q = start_pair.first;
                int s = start_pair.second;

                _ASSERT(stop_pair.second - stop_pair.first ==
                       start_pair.second - start_pair.first);
                _ASSERT(stop_pair.first-1 < query.GetLength());

                for (int k = 0; k < stop_pair.first - start_pair.first; k++) {
                    for (int m = 0; m < kAlphabetSize; m++) {
                        matrix(q+k, m) = 
                              (1 - m_DomainResFreqBoost) * ref_freqs[s+k][m];
                    }
                    matrix(q+k, query.GetLetter(q+k)) += m_DomainResFreqBoost; 
                }
            }
        }
    }
}


void
CMultiAligner::FindDomainHits()
{
    if (m_RPSdb.empty() || 
        m_Blockfile.empty() || 
        m_Freqfile.empty()) {
        return;
    }

    CHitList rps_hits;
    x_FindRPSHits(rps_hits);
        
    vector<SSegmentLoc> blocklist;
    x_LoadBlockBoundaries(m_Blockfile, blocklist);

    CProfileData profile_data;
    profile_data.Load(CProfileData::eGetPssm, m_RPSdb);

    x_RealignBlocks(rps_hits, blocklist, profile_data);
    blocklist.clear();
    profile_data.Clear();

    //-------------------------------------------------------
    if (m_Verbose) {
        printf("\n\nBlock alignments with conflicts resolved:\n");
        for (int i = 0; i < rps_hits.Size(); i++) {
            CHit *hit = rps_hits.GetHit(i);
            NON_CONST_ITERATE(vector<CHit *>, itr, hit->GetSubHit()) {
                CHit *subhit = *itr;
                printf("query %d %4d - %4d db %d %4d - %4d score %d ",
                     subhit->m_SeqIndex1, 
                     subhit->m_SeqRange1.GetFrom(), 
                     subhit->m_SeqRange1.GetTo(),
                     subhit->m_SeqIndex2, 
                     subhit->m_SeqRange2.GetFrom(), 
                     subhit->m_SeqRange2.GetTo(),
                     subhit->m_Score);
    
                printf("\n");
            }
        }
        printf("\n\n");
    }
    //-------------------------------------------------------

    profile_data.Load(CProfileData::eGetResFreqs, m_RPSdb, m_Freqfile);
    x_AssignRPSResFreqs(rps_hits, profile_data);
    profile_data.Clear();

    rps_hits.MatchOverlappingSubHits(m_DomainHits);

    const int kRpsScale = CMultiAligner::kRpsScaleFactor;
    for (int i = 0; i < m_DomainHits.Size(); i++) {
        CHit *hit = m_DomainHits.GetHit(i);
        hit->m_Score = (hit->m_Score + kRpsScale/2) / kRpsScale;
        NON_CONST_ITERATE(CHit::TSubHit, subitr, hit->GetSubHit()) {
            CHit *subhit = *subitr;
            subhit->m_Score = (subhit->m_Score + kRpsScale/2) / kRpsScale;
        }
    }

    //-------------------------------------------------------
    if (m_Verbose) {
        printf("\n\nMatched block alignments:\n");
        for (int i = 0; i < m_DomainHits.Size(); i++) {
            CHit *hit = m_DomainHits.GetHit(i);
            NON_CONST_ITERATE(vector<CHit *>, itr, hit->GetSubHit()) {
                CHit *subhit = *itr;
                printf("query %d %4d - %4d query %d %4d - %4d score %d\n",
                         subhit->m_SeqIndex1, 
                         subhit->m_SeqRange1.GetFrom(), 
                         subhit->m_SeqRange1.GetTo(),
                         subhit->m_SeqIndex2, 
                         subhit->m_SeqRange2.GetFrom(), 
                         subhit->m_SeqRange2.GetTo(),
                         subhit->m_Score);
            }
        }
        printf("\n\n");
    }
    //-------------------------------------------------------
}

END_SCOPE(cobalt)
END_NCBI_SCOPE

/*--------------------------------------------------------------------
  $Log$
  Revision 1.5  2005/11/10 15:39:14  papadopo
  Make local functions into members of CMultiAligner

  Revision 1.4  2005/11/08 19:49:19  papadopo
  fix solaris compile warnings

  Revision 1.3  2005/11/08 18:42:16  papadopo
  assert -> _ASSERT

  Revision 1.2  2005/11/08 17:55:02  papadopo
  ASSERT -> assert

  Revision 1.1  2005/11/07 18:14:00  papadopo
  Initial revision

--------------------------------------------------------------------*/
