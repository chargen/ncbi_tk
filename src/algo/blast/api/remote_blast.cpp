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
* Author:  Kevin Bealer
*
* ===========================================================================
*/

/// @file remote_blast.cpp
/// Queueing and Polling code for Remote Blast API.

#include <ncbi_pch.hpp>
#include <corelib/ncbi_system.hpp>
#include <algo/blast/api/remote_blast.hpp>

#include <objects/blast/blastclient.hpp>
#include <objects/blast/blast__.hpp>
#include <objects/scoremat/scoremat__.hpp>

#if defined(NCBI_OS_UNIX)
#include <unistd.h>
#endif

/** @addtogroup AlgoBlast
 *
 * @{
 */

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(blast)


// Static functions


/// Dump an ASN.1 (datatool type) object to an ostream as text ASN.1.
/// @param os Output stream.
/// @param t ASN.1 object.
template <class T>
void
s_Output(CNcbiOstream & os, CRef<T> t)
{
    auto_ptr<CObjectOStream> x(CObjectOStream::Open(eSerial_AsnText, os));
    *x << *t;
    os.flush();
}


/// Error value type used by Blast4 ASN.1 objects.
typedef list< CRef<CBlast4_error> > TErrorList;


/// Determine whether the search is still running.
/// @param reply Reply from get-search-results request.
/// @return True if search needs more time, false if done or failed.
static bool
s_SearchPending(CRef<CBlast4_reply> reply)
{
    const list< CRef<CBlast4_error> > & errors = reply->GetErrors();
    
    TErrorList::const_iterator i;
    
    for(i = errors.begin(); i != errors.end(); i++) {
        if ((*i)->GetCode() == eBlast4_error_code_search_pending) {
            return true;
        }
    }
    return false;
}


void CRemoteBlast::x_SearchErrors(CRef<CBlast4_reply> reply)
{
    const list< CRef<CBlast4_error> > & errors = reply->GetErrors();
    
    TErrorList::const_iterator i;
    
    for(i = errors.begin(); i != errors.end(); i++) {
        string msg;
        
        if ((*i)->CanGetMessage() && (! (*i)->GetMessage().empty())) {
            msg = ": ";
            msg += (*i)->GetMessage();
        }
        
        switch((*i)->GetCode()) {
        case eBlast4_error_code_conversion_warning:
            m_Warn.push_back(string("Warning: conversion_warning") + msg);
            break;
            
        case eBlast4_error_code_internal_error:
            m_Errs.push_back(string("Error: internal_error") + msg);
            break;
            
        case eBlast4_error_code_not_implemented:
            m_Errs.push_back(string("Error: not_implemented") + msg);
            break;
            
        case eBlast4_error_code_not_allowed:
            m_Errs.push_back(string("Error: not_allowed") + msg);
            break;
            
        case eBlast4_error_code_bad_request:
            m_Errs.push_back(string("Error: bad_request") + msg);
            break;
            
        case eBlast4_error_code_bad_request_id:
            m_Errs.push_back(string("Error: bad_request_id") + msg);
            break;
        }
    }
}



// CBlast4Option methods

void CRemoteBlast::x_CheckConfig(void)
{
    // If not configured, throw an exception - the associated string
    // will contain a list of the missing pieces.
    
    if (0 != m_NeedConfig) {
        string cfg("Configuration required:");
        
        if (eProgram & m_NeedConfig) {
            cfg += " <program>";
        }
        
        if (eService & m_NeedConfig) {
            cfg += " <service>";
        }
        
        if (eQueries & m_NeedConfig) {
            cfg += " <queries>";
        }
        
        if (eSubject & m_NeedConfig) {
            cfg += " <subject>";
        }
        
        NCBI_THROW(CBlastException, eInternal, cfg.c_str());
    }
}

CRef<CBlast4_reply>
CRemoteBlast::x_SendRequest(CRef<CBlast4_request_body> body)
{
    // If not configured, throw.
    x_CheckConfig();
    
    // Create the request; optionally echo it
    
    CRef<CBlast4_request> request(new CBlast4_request);
    request->SetBody(*body);
    
    if (eDebug == m_Verbose) {
        s_Output(NcbiCout, request);
    }
    
    // submit to server, get reply; optionally echo it
    
    CRef<CBlast4_reply> reply(new CBlast4_reply);
    
    try {
        //throw some_kind_of_nothing();
        CBlast4Client().Ask(*request, *reply);
    }
    catch(const CEofException&) {
        ERR_POST(Error << "No response from server, cannot "
                 "complete request.");
        
#if defined(NCBI_OS_UNIX)
        // Use _exit() to avoid coredump.
        _exit(-1);
#else
        exit(-1);
#endif
    }
    
    if (eDebug == m_Verbose) {
        s_Output(NcbiCout, reply);
    }
    
    return reply;
}

CRef<CBlast4_reply>
CRemoteBlast::x_GetSearchResults(void)
{
    CRef<CBlast4_get_search_results_request>
        gsrr(new CBlast4_get_search_results_request);
    
    gsrr->SetRequest_id(m_RID);
    
    CRef<CBlast4_request_body> body(new CBlast4_request_body);
    body->SetGet_search_results(*gsrr);
    
    return x_SendRequest(body);
}

// Pre:  start, wait, or done
// Post: failed or done

// Returns: true if done

bool CRemoteBlast::SubmitSync(int seconds)
{
    // eFailed: no work to do, already an error.
    // eDone:   already done, just return.
    
    EImmediacy immed = ePollAsync;
    
    switch(x_GetState()) {
    case eStart:
        x_SubmitSearch();
        if (! m_Errs.empty()) {
            break;
        }
        immed = ePollImmed;
        // fall through
        
    case eWait:
        x_PollUntilDone(immed, seconds);
        break;
    }
    
    return (x_GetState() == eDone);
}



// Pre:  start
// Post: failed, wait or done

// Returns: true if no error so far

bool CRemoteBlast::Submit(void)
{
    switch(x_GetState()) {
    case eStart:
        x_SubmitSearch();
    }
    
    return m_Errs.empty();
}

// Pre:  start, wait or done
// Post: wait or done

// Returns: true if done

bool CRemoteBlast::CheckDone(void)
{
    switch(x_GetState()) {
    case eFailed:
    case eDone:
        break;
        
    case eStart:
        Submit();
        break;
        
    case eWait:
        x_CheckResults();
    }
    
    return (x_GetState() == eDone);
}

CRemoteBlast::TGSRR * CRemoteBlast::x_GetGSRR(void)
{
    TGSRR* rv = NULL;
    
    if (SubmitSync() &&
        m_Reply.NotEmpty() &&
        m_Reply->CanGetBody() &&
        m_Reply->GetBody().IsGet_search_results()) {
        
        rv = & (m_Reply->SetBody().SetGet_search_results());
    }
    
    return rv;
}

CRef<CSeq_align_set> CRemoteBlast::GetAlignments(void)
{
    CRef<CSeq_align_set> rv;
    
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetAlignments()) {
        rv = & (gsrr->SetAlignments());
    }
    
    return rv;
}

CRef<CBlast4_phi_alignments> CRemoteBlast::GetPhiAlignments(void)
{
    CRef<CBlast4_phi_alignments> rv;
    
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetPhi_alignments()) {
        rv = & (gsrr->SetPhi_alignments());
    }
    
    return rv;
}

CRef<CBlast4_mask> CRemoteBlast::GetMask(void)
{
    CRef<CBlast4_mask> rv;
    
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetMask()) {
        rv = & (gsrr->SetMask());
    }
    
    return rv;
}

list< CRef<CBlast4_ka_block > > CRemoteBlast::GetKABlocks(void)
{ 
    list< CRef<CBlast4_ka_block > > rv;
        
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetKa_blocks()) {
        rv = (gsrr->SetKa_blocks());
    }
    
    return rv;
}

list< string > CRemoteBlast::GetSearchStats(void)
{
    list< string > rv;
    
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetSearch_stats()) {
        rv = (gsrr->SetSearch_stats());
    }
    
    return rv;
}

CRef<CScore_matrix_parameters> CRemoteBlast::GetPSSM(void)
{
    CRef<CScore_matrix_parameters> rv;
    
    TGSRR * gsrr = x_GetGSRR();
    
    if (gsrr && gsrr->CanGetPssm()) {
        rv = & (gsrr->SetPssm());
    }
    
    return rv;
}


// Internal CRemoteBlast methods

CRemoteBlast::EState CRemoteBlast::x_GetState(void)
{
    // CBlast4Option states:
    
    // 0. start  (no rid, no errors)
    // 1. failed (errors)
    // 2. wait   (has rid, no errors, still pending)
    // 3. done   (has rid, no errors, not pending)
    
    EState rv = eDone;
    
    if (! m_Errs.empty()) {
        rv = eFailed;
    } else if (m_RID.empty()) {
        rv = eStart;
    } else if (m_Pending) {
        rv = eWait;
    }
    
    return rv;
}

void CRemoteBlast::x_SubmitSearch(void)
{
    if (m_QSR.Empty()) {
        m_Errs.push_back("No request exists and no RID was specified.");
        return;
    }
    
    x_SetAlgoOpts();
    
    CRef<CBlast4_request_body> body(new CBlast4_request_body);
    body->SetQueue_search(*m_QSR);
    
    CRef<CBlast4_reply> reply;
    
    try {
        reply = x_SendRequest(body);
    }
    catch(const CEofException&) {
        m_Errs.push_back("No response from server, cannot complete request.");
        return;
    }
    
    if (reply->CanGetBody()  &&
        reply->GetBody().GetQueue_search().CanGetRequest_id()) {
        
        m_RID = reply->GetBody().GetQueue_search().GetRequest_id();
    }
    
    x_SearchErrors(reply);
    
    if (m_Errs.empty()) {
        m_Pending = true;
    }
}

void CRemoteBlast::x_CheckResults(void)
{
    if (! m_Errs.empty()) {
        m_Pending = false;
    }
    
    if (! m_Pending) {
        return;
    }
    
    CRef<CBlast4_reply> r;
    
    bool try_again = true;
    
    while(try_again) {
        try {
            r = x_GetSearchResults();
            m_Pending = s_SearchPending(r);
            try_again = false;
        }
        catch(const CEofException&) {
            --m_ErrIgn;
            
            if (m_ErrIgn == 0) {
                m_Errs.push_back("No response from server, "
                                 "cannot complete request.");
                return;
            }
            
            SleepSec(10);
        }
    }
    
    if (! m_Pending) {
        x_SearchErrors(r);
        
        if (! m_Errs.empty()) {
            return;
        } else if (r->CanGetBody() && r->GetBody().IsGet_search_results()) {
            m_Reply = r;
        } else {
            m_Errs.push_back("Results were not a get-search-results reply");
        }
    }
}

// The input here is a hint as to whether the request might be ready.
// If the flag is true, then we are polling immediately after
// submission.  In this case, the results will not be ready, and so we
// skip the first results check to reduce net traffic.  If the flag is
// false, then the user is using the asynchronous interface, and we do
// not know how long it has been since the request was submitted.  In
// this case, we check the results before sleeping.
//
// If this was always set to 'true' then async mode would -always-
// sleep.  This is undesireable in the case where (for example) 100
// requests are batched together - the mandatory sleeps would add to a
// total of 1000 seconds, more than a quarter hour.
//
// If it were always specified as 'false', then synchronous mode would
// shoot off an immediate 'check results' as soon as the "submit"
// returned, which creates unnecessary traffic.
//
// Futher optimizations are no doubt possible.

void CRemoteBlast::x_PollUntilDone(EImmediacy immed, int timeout)
{
    if (eDebug == m_Verbose)
        cout << "polling " << 0 << endl;
    
    // Configuration - internal for now
    
    double start_sec = 10.0;
    double increment = 1.30;
    double max_sleep = 300.0;
    double max_time  = timeout;
    
    if (eDebug == m_Verbose)
        cout << "polling " << start_sec << "/" << increment << "/" << max_sleep << "/" << max_time << "/" << endl;
    
    // End config
    
    double sleep_next = start_sec;
    double sleep_totl = 0.0;
    
    if (eDebug == m_Verbose)
        cout << "line " << __LINE__ << " sleep next " << sleep_next << " sleep totl " << sleep_totl << endl;
    
    if (ePollAsync == immed) {
        x_CheckResults();
    }
    
    while (m_Pending && (sleep_totl < max_time)) {
        if (eDebug == m_Verbose)
            cout << " about to sleep " << sleep_next << endl;
        
        double max_left = max_time - sleep_totl;
        
        // Don't oversleep
        if (sleep_next > max_left) {
            sleep_next = max_left;
            
            // But never sleep less than 2
            if (sleep_next < 2.0)
                sleep_next = 2.0;
        }
        
        SleepSec(int(sleep_next));
        sleep_totl += sleep_next;
        
        if (eDebug == m_Verbose)
            cout << " done, total = " << sleep_totl << endl;
        
        if (sleep_next < max_sleep) {
            sleep_next *= increment;
            if (sleep_next > max_sleep) {
                sleep_next = max_sleep;
            }
        }
        
        if (eDebug == m_Verbose)
            cout << " next sleep time = " << sleep_next << endl;
        
        x_CheckResults();
    }
}

void CRemoteBlast::x_Init(CBlastOptionsHandle * opts_handle,
                          const char          * program,
                          const char          * service)
{
    if (! (opts_handle && program && service)) {
        if (! opts_handle) {
            NCBI_THROW(CBlastException, eBadParameter,
                       "NULL argument specified: options handle");
        }
        if (! program) {
            NCBI_THROW(CBlastException, eBadParameter,
                       "NULL argument specified: program");
        }
        NCBI_THROW(CBlastException, eBadParameter,
                   "NULL argument specified: service");
    }
    
    m_CBOH.Reset( opts_handle );
    m_ErrIgn     = 5;
    m_Pending    = false;
    m_Verbose    = eSilent;
    m_NeedConfig = eNeedAll;
    
    m_QSR.Reset(new CBlast4_queue_search_request);
    
    m_QSR->SetProgram(program);
    m_QSR->SetService(service);
    
    m_NeedConfig = ENeedConfig(m_NeedConfig & ~(eProgram | eService));
    
    if (! (opts_handle && opts_handle->SetOptions().GetBlast4AlgoOpts())) {
        // This happens if you do not specify eRemote for the
        // CBlastOptions subclass constructor.
        
        NCBI_THROW(CBlastException, eBadParameter,
                   "CRemoteBlast: No remote API options.");
    }
}

void CRemoteBlast::x_Init(const string & RID)
{
    if (RID.empty()) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "Empty RID string specified");
    }
    
    m_RID        = RID;
    m_ErrIgn     = 5;
    m_Pending    = true;
    m_Verbose    = eSilent;
    m_NeedConfig = eNoConfig;
}

void CRemoteBlast::x_SetAlgoOpts(void)
{
    CBlast4_parameters * algo_opts =
        m_CBOH->SetOptions().GetBlast4AlgoOpts();
    
    m_QSR->SetAlgorithm_options().Set() = *algo_opts;
}

// the "int" version is not actually used (no program options need it.)
void CRemoteBlast::x_SetOneParam(const char * name, const int * x)
{
    CRef<CBlast4_value> v(new CBlast4_value);
    v->SetInteger(*x);
        
    CRef<CBlast4_parameter> p(new CBlast4_parameter);
    p->SetName(name);
    p->SetValue(*v);
        
    m_QSR->SetProgram_options().Set().push_back(p);
}

void CRemoteBlast::x_SetOneParam(const char * name, const list<int> * x)
{
    CRef<CBlast4_value> v(new CBlast4_value);
    v->SetInteger_list() = *x;
        
    CRef<CBlast4_parameter> p(new CBlast4_parameter);
    p->SetName(name);
    p->SetValue(*v);
        
    m_QSR->SetProgram_options().Set().push_back(p);
}

void CRemoteBlast::x_SetOneParam(const char * name, const char ** x)
{
    CRef<CBlast4_value> v(new CBlast4_value);
    v->SetString().assign((x && (*x)) ? (*x) : "");
        
    CRef<CBlast4_parameter> p(new CBlast4_parameter);
    p->SetName(name);
    p->SetValue(*v);
        
    m_QSR->SetProgram_options().Set().push_back(p);
}

void CRemoteBlast::x_SetOneParam(const char * name, CScore_matrix_parameters * matrix)
{
    CRef<CBlast4_value> v(new CBlast4_value);
    v->SetMatrix(*matrix);
        
    CRef<CBlast4_parameter> p(new CBlast4_parameter);
    p->SetName(name);
    p->SetValue(*v);
        
    m_QSR->SetProgram_options().Set().push_back(p);
}

void CRemoteBlast::SetQueries(CRef<CBioseq_set> bioseqs)
{
    if (bioseqs.Empty()) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "Empty reference for query.");
    }
    
    CRef<CBlast4_queries> queries_p(new CBlast4_queries);
    queries_p->SetBioseq_set(*bioseqs);
    
    m_QSR->SetQueries(*queries_p);
    m_NeedConfig = ENeedConfig(m_NeedConfig & (~ eQueries));
}

void CRemoteBlast::SetQueries(list< CRef<CSeq_loc> > & seqlocs)
{
    if (seqlocs.empty()) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "Empty list for query.");
    }
    
    CRef<CBlast4_queries> queries_p(new CBlast4_queries);
    queries_p->SetSeq_loc_list() = seqlocs;
    
    m_QSR->SetQueries(*queries_p);
    m_NeedConfig = ENeedConfig(m_NeedConfig & (~ eQueries));
}

void CRemoteBlast::SetQueries(CRef<CScore_matrix_parameters> pssm)
{
    if (pssm.Empty()) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "Empty reference for query pssm.");
    }
    
    if (! pssm->GetMatrix().CanGetQuery()) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "Empty reference for pssm component pssm.matrix.query.");
    }
    
    string psi_program("blastp");
    string old_service("plain");
    string new_service("psi");
    
    if (m_QSR->GetProgram() != psi_program) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "PSI-Blast is only supported for blastp.");
    }
    
    if (m_QSR->GetService().empty()) {
        NCBI_THROW(CBlastException, eInternal,
                   "Internal error: service is not set.");
    }
    
    if ((m_QSR->GetService() != old_service) &&
        (m_QSR->GetService() != new_service)) {
        
        // Allowing "psi" allows the matrix to be set, then replaced.
        
        NCBI_THROW(CBlastException, eBadParameter,
                   string("PSI-Blast cannot also be ") +
                   m_QSR->GetService() + ".");
    }
    
    CRef<CBlast4_queries> queries_p(new CBlast4_queries);
    queries_p->SetPssm(*pssm);
    
    m_QSR->SetQueries(*queries_p);
    m_NeedConfig = ENeedConfig(m_NeedConfig & (~ eQueries));
    
    m_QSR->SetService(new_service);
}

void CRemoteBlast::SetDatabase(const char * x)
{
    if (!x) {
        NCBI_THROW(CBlastException, eBadParameter,
                   "NULL specified for database.");
    }
        
    CRef<CBlast4_subject> subject_p(new CBlast4_subject);
    subject_p->SetDatabase(x);
    m_QSR->SetSubject(*subject_p);
    m_NeedConfig = ENeedConfig(m_NeedConfig & (~ eSubject));
}

string CRemoteBlast::GetErrors(void)
{
    if (m_Errs.empty()) {
        return string();
    }
    
    string rvalue = m_Errs[0];
    
    for(unsigned i = 1; i<m_Errs.size(); i++) {
        rvalue += "\n";
        rvalue += m_Errs[i];
    }
    
    return rvalue;
}

string CRemoteBlast::GetWarnings(void)
{
    if (m_Warn.empty()) {
        return string();
    }
    
    string rvalue = m_Warn[0];
    
    for(unsigned i = 1; i<m_Warn.size(); i++) {
        rvalue += "\n";
        rvalue += m_Warn[i];
    }
    
    return rvalue;
}

END_SCOPE(blast)
END_NCBI_SCOPE

/* @} */

/*
* ===========================================================================
*
* $Log$
* Revision 1.16  2004/06/21 16:34:09  bealer
* - Make scope usage more consistent for doxygen's sake.
*
* Revision 1.15  2004/06/09 15:56:06  bealer
* - Fix return type of x_GetState
*
* Revision 1.14  2004/06/08 19:50:18  bealer
* - Add doxygen comments.
*
* Revision 1.13  2004/05/21 21:41:02  gorelenk
* Added PCH ncbi_pch.hpp
*
* Revision 1.12  2004/05/12 19:26:49  bealer
* - Additional checking for PSSM queries.
* - PSSM query now implies service="psi".
*
* Revision 1.11  2004/05/10 15:10:08  bealer
* - Error processing problem: messages should be treated as optional.
*
* Revision 1.10  2004/05/05 17:39:46  dicuccio
* Fixed syntax error on MSVC6
*
* Revision 1.9  2004/05/05 15:35:30  bealer
* - Features:
*   - Add PSSM queries (for PSI-Blast) and seq-loc-list.
*   - Add GetWarnings() mechanism.
*   - Add PSSM queries (for PSI-Blast).
*   - Add seq-loc-list queries (allows multiple identifier base queries, or
*     one query based on identifier plus interval.
*   - Add GetPSSM() to retrieve results of PSI-Blast run.
*
* - Other changes:
*   - Move some static functions into class.
*   - Rework error processing to split out warnings.
*   - Changes to error text formats.
*   - Seperate some common code into x_GetGSSR() util method.
*   - De-inlined several methods.
*
* Revision 1.8  2004/04/12 16:35:25  bealer
* - Fix CheckDone problem in CRemoteBlast.
* - Add more parameter checking and exception throwing.
*
* Revision 1.7  2004/03/23 22:29:42  bealer
* - Verify that CRemoteBlast objects are configured properly.
*
* Revision 1.6  2004/03/19 19:22:55  camacho
* Move to doxygen group AlgoBlast, add missing CVS logs at EOF
*
* Revision 1.5  2004/03/12 22:07:03  camacho
* Remove unused variables
*
* Revision 1.4  2004/02/26 22:24:46  gorelenk
* Include for <unistd.h> moved to be after
* #include <corelib/ncbi_system.hpp>.
*
* Revision 1.3  2004/02/26 17:07:40  gorelenk
* Added #if defined(NCBI_OS_UNIX) for #include <unistd.h>.
*
* Revision 1.2  2004/02/18 18:28:51  bealer
* - Fix verbosity tests.
*
* Revision 1.1  2004/02/18 17:30:57  bealer
* - Change blast4_options.* to remote_blast.*, plus changes.
*
* Revision 1.6  2004/02/09 22:35:37  bealer
* - Delay examination of CBlastOptionsHandle object until Submit() action.
*
* Revision 1.5  2004/02/06 00:16:39  bealer
* - Add RID capability.
* - Detect lack of eRemote flag.
*
* Revision 1.4  2004/02/05 19:20:39  bealer
* - Add retry capability to API code.
*
* Revision 1.3  2004/02/05 00:37:43  bealer
* - Polling optimization.
*
* Revision 1.2  2004/02/04 22:31:14  bealer
* - Add async interface to Blast4 API.
* - Clean up, simplify code and interfaces.
* - Add state-based logic to promote robustness.
*
* Revision 1.1  2004/01/16 20:37:55  bealer
* - Add CBlast4Options class (Blast 4 API)
*
*
* ===========================================================================
*/
