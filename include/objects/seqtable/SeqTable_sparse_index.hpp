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
 */

/// @file SeqTable_sparse_index.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'seqtable.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: SeqTable_sparse_index_.hpp


#ifndef OBJECTS_SEQTABLE_SEQTABLE_SPARSE_INDEX_HPP
#define OBJECTS_SEQTABLE_SEQTABLE_SPARSE_INDEX_HPP


// generated includes
#include <objects/seqtable/SeqTable_sparse_index_.hpp>

#include <util/bitset/ncbi_bitset.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class NCBI_SEQ_EXPORT CSeqTable_sparse_index : public CSeqTable_sparse_index_Base
{
    typedef CSeqTable_sparse_index_Base Tparent;
public:
    // constructor
    CSeqTable_sparse_index(void);
    // destructor
    ~CSeqTable_sparse_index(void);

    enum {
        kSkipped = ~size_t(0),
        kInvalidIndex = ~size_t(0)
    };

    // Overload base setters to reset extra data fields
    void Reset(void) {
        m_BitCount.reset();
        m_BitVector.reset();
        CSeqTable_sparse_index_Base::Reset();
    }

    // return index in compacted array of the argument row index
    // return kSkipped if the row doesn't have value
    size_t GetIndexAt(size_t index) const;

    // return true if the row has value in compacted array
    bool IsSelectedAt(size_t index) const;

    // set sparse index from bvector
    // the ownership of the bvector object is taken,
    // and the bvector should not be changed outside
    void SetBit_set_bvector(const bm::bvector<>* bv);

    // const_iterator iterates only set bits
    class const_iterator {
    public:
        const_iterator(void)
            : m_Index(kInvalidIndex),
              m_SetBitIndex(0)
            {
            }
        
        DECLARE_OPERATOR_BOOL(m_Index != kInvalidIndex);

        bool operator==(const const_iterator& iter) const {
            return m_Index == iter.m_Index;
        }

        bool operator!=(const const_iterator& iter) const {
            return m_Index != iter.m_Index;
        }

        // current bit is always set
        bool operator*(void) const {
            return true;
        }

        size_t GetIndex(void) const {
            return m_Index;
        }

        // go to the next set bit
        const_iterator& operator++(void) {
            m_Index = m_Obj->x_GetNextSetBitIndex(m_Index, m_SetBitIndex);
            ++m_SetBitIndex;
            return *this;
        }

    protected:
        friend class CSeqTable_sparse_index;

        const_iterator(const CSeqTable_sparse_index* obj,
                       size_t index,
                       size_t set_bit_index = 0)
            : m_Obj(obj),
              m_Index(index),
              m_SetBitIndex(set_bit_index)
            {
            }

    private:
        CConstRef<CSeqTable_sparse_index> m_Obj;
        size_t m_Index, m_SetBitIndex;
    };

    // return number of all bits (both 0 and 1)
    size_t size(void) const;

    const_iterator begin(void) const {
        return const_iterator(this, x_GetFirstSetBitIndex());
    }
    const_iterator end(void) const {
        return const_iterator();
    }

    
    // change the representation of index
    void ChangeTo(E_Choice type);
    void ChangeToIndexes(void);
    void ChangeToIndexes_delta(void);
    void ChangeToBit_set(void);
    void ChangeToBit_set_bvector(void);

protected:
    friend class const_iterator;

    void x_Preprocess(void) const;
    void x_EnsurePreprocessed(void) const {
        E_Choice type = Which();
        if ( (type == e_Indexes_delta) ||
             (type == e_Bit_set_bvector && !m_BitVector) ) {
            x_Preprocess();
        }
    }
    size_t x_GetBytesBitCount(size_t byte_count) const;
    struct SBitCount {
        size_t m_BlockBitCountSize;
        AutoArray<size_t> m_BlockBitCount;
        size_t m_CacheBlock;
        AutoArray<size_t> m_CacheBlockBitCount;
    };
    mutable AutoPtr<SBitCount> m_BitCount;
    mutable AutoPtr<const bm::bvector<> > m_BitVector;

    size_t x_GetFirstSetBitIndex(void) const;
    size_t x_GetNextSetBitIndex(size_t index, size_t set_bit_index) const;

private:
    // Prohibit copy constructor and assignment operator
    CSeqTable_sparse_index(const CSeqTable_sparse_index& value);
    CSeqTable_sparse_index& operator=(const CSeqTable_sparse_index& value);
};

/////////////////// CSeqTable_sparse_index inline methods

// constructor
inline
CSeqTable_sparse_index::CSeqTable_sparse_index(void)
{
}


/////////////////// end of CSeqTable_sparse_index inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_SEQTABLE_SEQTABLE_SPARSE_INDEX_HPP
/* Original file checksum: lines: 86, chars: 2645, CRC32: cdd5d50f */
