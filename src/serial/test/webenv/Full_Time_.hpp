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

/// @file Full_Time_.hpp
/// Data storage class.
///
/// This file was generated by application DATATOOL
/// using the following specifications:
/// 'twebenv.asn'.
///
/// ATTENTION:
///   Don't edit or commit this file into CVS as this file will
///   be overridden (by DATATOOL) without warning!

#ifndef FULL_TIME_BASE_HPP
#define FULL_TIME_BASE_HPP

// standard includes
#include <serial/serialbase.hpp>

// generated classes

/////////////////////////////////////////////////////////////////////////////
class CFull_Time_Base : public ncbi::CSerialObject
{
    typedef ncbi::CSerialObject Tparent;
public:
    // constructor
    CFull_Time_Base(void);
    // destructor
    virtual ~CFull_Time_Base(void);

    // type info
    DECLARE_INTERNAL_TYPE_INFO();

    // types
    typedef int TYear;
    typedef int TMonth;
    typedef int TDay;
    typedef int THour;
    typedef int TMinute;
    typedef int TSecond;

    // getters
    // setters

    /// mandatory
    /// typedef int TYear
    bool IsSetYear(void) const;
    bool CanGetYear(void) const;
    void ResetYear(void);
    TYear GetYear(void) const;
    void SetYear(TYear value);
    TYear& SetYear(void);

    /// mandatory
    /// typedef int TMonth
    bool IsSetMonth(void) const;
    bool CanGetMonth(void) const;
    void ResetMonth(void);
    TMonth GetMonth(void) const;
    void SetMonth(TMonth value);
    TMonth& SetMonth(void);

    /// mandatory
    /// typedef int TDay
    bool IsSetDay(void) const;
    bool CanGetDay(void) const;
    void ResetDay(void);
    TDay GetDay(void) const;
    void SetDay(TDay value);
    TDay& SetDay(void);

    /// mandatory
    /// typedef int THour
    bool IsSetHour(void) const;
    bool CanGetHour(void) const;
    void ResetHour(void);
    THour GetHour(void) const;
    void SetHour(THour value);
    THour& SetHour(void);

    /// mandatory
    /// typedef int TMinute
    bool IsSetMinute(void) const;
    bool CanGetMinute(void) const;
    void ResetMinute(void);
    TMinute GetMinute(void) const;
    void SetMinute(TMinute value);
    TMinute& SetMinute(void);

    /// mandatory
    /// typedef int TSecond
    bool IsSetSecond(void) const;
    bool CanGetSecond(void) const;
    void ResetSecond(void);
    TSecond GetSecond(void) const;
    void SetSecond(TSecond value);
    TSecond& SetSecond(void);

    /// Reset the whole object
    virtual void Reset(void);


private:
    // Prohibit copy constructor and assignment operator
    CFull_Time_Base(const CFull_Time_Base&);
    CFull_Time_Base& operator=(const CFull_Time_Base&);

    // data
    Uint4 m_set_State[1];
    TYear m_Year;
    TMonth m_Month;
    TDay m_Day;
    THour m_Hour;
    TMinute m_Minute;
    TSecond m_Second;
};






///////////////////////////////////////////////////////////
///////////////////// inline methods //////////////////////
///////////////////////////////////////////////////////////
inline
bool CFull_Time_Base::IsSetYear(void) const
{
    return ((m_set_State[0] & 0x3) != 0);
}

inline
bool CFull_Time_Base::CanGetYear(void) const
{
    return IsSetYear();
}

inline
void CFull_Time_Base::ResetYear(void)
{
    m_Year = 0;
    m_set_State[0] &= ~0x3;
}

inline
int CFull_Time_Base::GetYear(void) const
{
    if (!CanGetYear()) {
        ThrowUnassigned(0);
    }
    return m_Year;
}

inline
void CFull_Time_Base::SetYear(int value)
{
    m_Year = value;
    m_set_State[0] |= 0x3;
}

inline
int& CFull_Time_Base::SetYear(void)
{
#ifdef _DEBUG
    if (!IsSetYear()) {
        memset(&m_Year,ms_UnassignedByte,sizeof(m_Year));
    }
#endif
    m_set_State[0] |= 0x1;
    return m_Year;
}

inline
bool CFull_Time_Base::IsSetMonth(void) const
{
    return ((m_set_State[0] & 0xc) != 0);
}

inline
bool CFull_Time_Base::CanGetMonth(void) const
{
    return IsSetMonth();
}

inline
void CFull_Time_Base::ResetMonth(void)
{
    m_Month = 0;
    m_set_State[0] &= ~0xc;
}

inline
int CFull_Time_Base::GetMonth(void) const
{
    if (!CanGetMonth()) {
        ThrowUnassigned(1);
    }
    return m_Month;
}

inline
void CFull_Time_Base::SetMonth(int value)
{
    m_Month = value;
    m_set_State[0] |= 0xc;
}

inline
int& CFull_Time_Base::SetMonth(void)
{
#ifdef _DEBUG
    if (!IsSetMonth()) {
        memset(&m_Month,ms_UnassignedByte,sizeof(m_Month));
    }
#endif
    m_set_State[0] |= 0x4;
    return m_Month;
}

inline
bool CFull_Time_Base::IsSetDay(void) const
{
    return ((m_set_State[0] & 0x30) != 0);
}

inline
bool CFull_Time_Base::CanGetDay(void) const
{
    return IsSetDay();
}

inline
void CFull_Time_Base::ResetDay(void)
{
    m_Day = 0;
    m_set_State[0] &= ~0x30;
}

inline
int CFull_Time_Base::GetDay(void) const
{
    if (!CanGetDay()) {
        ThrowUnassigned(2);
    }
    return m_Day;
}

inline
void CFull_Time_Base::SetDay(int value)
{
    m_Day = value;
    m_set_State[0] |= 0x30;
}

inline
int& CFull_Time_Base::SetDay(void)
{
#ifdef _DEBUG
    if (!IsSetDay()) {
        memset(&m_Day,ms_UnassignedByte,sizeof(m_Day));
    }
#endif
    m_set_State[0] |= 0x10;
    return m_Day;
}

inline
bool CFull_Time_Base::IsSetHour(void) const
{
    return ((m_set_State[0] & 0xc0) != 0);
}

inline
bool CFull_Time_Base::CanGetHour(void) const
{
    return IsSetHour();
}

inline
void CFull_Time_Base::ResetHour(void)
{
    m_Hour = 0;
    m_set_State[0] &= ~0xc0;
}

inline
int CFull_Time_Base::GetHour(void) const
{
    if (!CanGetHour()) {
        ThrowUnassigned(3);
    }
    return m_Hour;
}

inline
void CFull_Time_Base::SetHour(int value)
{
    m_Hour = value;
    m_set_State[0] |= 0xc0;
}

inline
int& CFull_Time_Base::SetHour(void)
{
#ifdef _DEBUG
    if (!IsSetHour()) {
        memset(&m_Hour,ms_UnassignedByte,sizeof(m_Hour));
    }
#endif
    m_set_State[0] |= 0x40;
    return m_Hour;
}

inline
bool CFull_Time_Base::IsSetMinute(void) const
{
    return ((m_set_State[0] & 0x300) != 0);
}

inline
bool CFull_Time_Base::CanGetMinute(void) const
{
    return IsSetMinute();
}

inline
void CFull_Time_Base::ResetMinute(void)
{
    m_Minute = 0;
    m_set_State[0] &= ~0x300;
}

inline
int CFull_Time_Base::GetMinute(void) const
{
    if (!CanGetMinute()) {
        ThrowUnassigned(4);
    }
    return m_Minute;
}

inline
void CFull_Time_Base::SetMinute(int value)
{
    m_Minute = value;
    m_set_State[0] |= 0x300;
}

inline
int& CFull_Time_Base::SetMinute(void)
{
#ifdef _DEBUG
    if (!IsSetMinute()) {
        memset(&m_Minute,ms_UnassignedByte,sizeof(m_Minute));
    }
#endif
    m_set_State[0] |= 0x100;
    return m_Minute;
}

inline
bool CFull_Time_Base::IsSetSecond(void) const
{
    return ((m_set_State[0] & 0xc00) != 0);
}

inline
bool CFull_Time_Base::CanGetSecond(void) const
{
    return IsSetSecond();
}

inline
void CFull_Time_Base::ResetSecond(void)
{
    m_Second = 0;
    m_set_State[0] &= ~0xc00;
}

inline
int CFull_Time_Base::GetSecond(void) const
{
    if (!CanGetSecond()) {
        ThrowUnassigned(5);
    }
    return m_Second;
}

inline
void CFull_Time_Base::SetSecond(int value)
{
    m_Second = value;
    m_set_State[0] |= 0xc00;
}

inline
int& CFull_Time_Base::SetSecond(void)
{
#ifdef _DEBUG
    if (!IsSetSecond()) {
        memset(&m_Second,ms_UnassignedByte,sizeof(m_Second));
    }
#endif
    m_set_State[0] |= 0x400;
    return m_Second;
}

///////////////////////////////////////////////////////////
////////////////// end of inline methods //////////////////
///////////////////////////////////////////////////////////






#endif // FULL_TIME_BASE_HPP
