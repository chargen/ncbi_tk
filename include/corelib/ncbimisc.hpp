#ifndef CORELIB___NCBIMISC__HPP
#define CORELIB___NCBIMISC__HPP

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
 * Author:  Denis Vakatov, Eugene Vasilchenko
 *
 *
 */

/// @file ncbimisc.hpp
/// Miscellaneous common-use basic types and functionality


#include <corelib/ncbidbg.hpp>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if !defined(HAVE_NULLPTR)  &&  !defined(nullptr)
#  define nullptr NULL
#endif


/** @addtogroup AppFramework
 *
 * @{
 */


#ifndef NCBI_ESWITCH_DEFINED
#define NCBI_ESWITCH_DEFINED

extern "C" {

/*
 * ATTENTION!   Do not change this enumeration!
 *
 * It must always be kept in sync with its plain C counterpart defined in
 * "connect/ncbi_types.h". If you absolutely(sic!) need to alter this
 * type, please apply equivalent changes to both definitions.
 */

/** Aux. enum to set/unset/default various features.
 */
typedef enum ENcbiSwitch {
    eOff = 0,
    eOn,
    eDefault
} ESwitch;

} // extern "C"

#endif //!NCBI_ESWITCH_DEFINED


#ifndef NCBI_EOWNERSHIP_DEFINED
#define NCBI_EOWNERSHIP_DEFINED

extern "C" {

/*
 * ATTENTION!   Do not change this enumeration!
 *
 * It must always be kept in sync with its plain C counterpart defined in
 * "connect/ncbi_types.h". If you absolutely(sic!) need to alter this
 * type, please apply equivalent changes to both definitions.
 */

/** Ownership relations between objects.
 *
 * Can be used to define or transfer ownership of objects.
 * For example, specify if a CSocket object owns its underlying SOCK object.
 */
typedef enum ENcbiOwnership {
    eNoOwnership,       /** No ownership assumed                    */
    eTakeOwnership      /** An object can take ownership of another */
} EOwnership;

} // extern "C"

#endif //!NCBI_EOWNERSHIP_DEFINED


BEGIN_NCBI_NAMESPACE;


/// Whether a value is nullable.
enum ENullable {
    eNullable,          ///< Value can be null
    eNotNullable        ///< Value cannot be null
};


/// Signedness of a value.
enum ESign {
    eNegative = -1,     ///< Value is negative
    eZero     =  0,     ///< Value is zero
    ePositive =  1      ///< Value is positive
};


/// Whether to truncate/round a value.
enum ERound {
    eTrunc,             ///< Value must be truncated
    eRound              ///< Value must be rounded
};


/// Whether to follow symbolic links (also known as shortcuts or aliases)
enum EFollowLinks {
    eIgnoreLinks,       ///< Do not follow symbolic links
    eFollowLinks        ///< Follow symbolic links
};


/// Whether to normalize a path
enum ENormalizePath {
    eNormalizePath,     ///< Normalize a path
    eNotNormalizePath   ///< Do not normalize a path
};


/// Interrupt on signal mode
///
/// On UNIX some functions can be interrupted by a signal and EINTR errno
/// value. We can restart or cancel its execution.
enum EInterruptOnSignal {
    eInterruptOnSignal, ///< Cancel operation if interrupted by a signal
    eRestartOnSignal    ///< Restart operation if interrupted by a signal
};


/////////////////////////////////////////////////////////////////////////////
/// Support for safe bool operators
/////////////////////////////////////////////////////////////////////////////


/// Low level macro for declaring safe bool operator.
#define DECLARE_SAFE_BOOL_METHOD(Expr)                                  \
    struct SSafeBoolTag {                                               \
        void SafeBoolTrue(SSafeBoolTag*) {}                             \
    };                                                                  \
    typedef void (SSafeBoolTag::*TBoolType)(SSafeBoolTag*);             \
    operator TBoolType() const {                                        \
        return (Expr)? &SSafeBoolTag::SafeBoolTrue: 0;                  \
    }                                                                   \
    private:                                                            \
    bool operator==(TBoolType) const;                                   \
    bool operator!=(TBoolType) const;                                   \
    public:                                                             \
    friend struct SSafeBoolTag


/// Declaration of safe bool operator from boolean expression.
/// Actual operator declaration will be:
///    operator TBoolType(void) const;
/// where TBoolType is a typedef convertible to bool (member pointer).
#define DECLARE_OPERATOR_BOOL(Expr)             \
    DECLARE_SAFE_BOOL_METHOD(Expr)


/// Declaration of safe bool operator from pointer expression.
/// Actual operator declaration will be:
///    operator bool(void) const;
#define DECLARE_OPERATOR_BOOL_PTR(Ptr)          \
    DECLARE_OPERATOR_BOOL((Ptr) != 0)


/// Declaration of safe bool operator from CRef<>/CConstRef<> expression.
/// Actual operator declaration will be:
///    operator bool(void) const;
#define DECLARE_OPERATOR_BOOL_REF(Ref)          \
    DECLARE_OPERATOR_BOOL((Ref).NotNull())

/// Override the DECLARE_OPERATOR_BOOL, etc.
/// from an ancestor class.  This is needed because sometimes you can't just 
/// use DECLARE_OPERATOR_BOOL due to cases such as the ancestor class
/// being privately inherited from.
#define OVERRIDE_OPERATOR_BOOL(TAncestorClass, NewBoolExpr)     \
    using TAncestorClass::TBoolType;                                    \
    using TAncestorClass::SSafeBoolTag;                                 \
    operator TBoolType() const {                                \
        return (NewBoolExpr)? & SSafeBoolTag::SafeBoolTrue : 0;        \
    }

/// Template used for empty base class optimization.
/// See details in the August '97 "C++ Issue" of Dr. Dobb's Journal
/// Also available from http://www.cantrip.org/emptyopt.html
/// We store usually empty template argument class together with data member.
/// This template is much like STL's pair<>, but the access to members
/// is done though methods first() and second() returning references
/// to corresponding members.
/// First template argument is represented as private base class,
/// while second template argument is represented as private member.
/// In addition to constructor taking two arguments,
/// we add constructor for initialization of only data member (second).
/// This is useful since usually first type is empty and doesn't require
/// non-trivial constructor.
/// We do not define any comparison functions as this template is intented
/// to be used internally within another templates,
/// which themselves should provide any additional functionality.

template<class Base, class Member>
class pair_base_member : private Base
{
public:
    typedef Base base_type;
    typedef Base first_type;
    typedef Member member_type;
    typedef Member second_type;
    
    pair_base_member(void)
        : base_type(), m_Member()
        {
        }
    
    explicit pair_base_member(const member_type& member_value)
        : base_type(), m_Member(member_value)
        {
        }
    
    explicit pair_base_member(const first_type& first_value,
                              const second_type& second_value)
        : base_type(first_value), m_Member(second_value)
        {
        }
    
    const first_type& first() const
        {
            return *this;
        }
    first_type& first()
        {
            return *this;
        }

    const second_type& second() const
        {
            return m_Member;
        }
    second_type& second()
        {
            return m_Member;
        }

    void Swap(pair_base_member<first_type, second_type>& p)
        {
            if (static_cast<void*>(&first()) != static_cast<void*>(&second())) {
                // work around an IBM compiler bug which causes it to perform
                // a spurious 1-byte swap, yielding mixed-up values.
                swap(first(), p.first());
            }
            swap(second(), p.second());
        }

private:
    member_type m_Member;
};


/// Template used to replace bool type arguments with some strict equivalent.
/// This allow to prevent compiler to do an implicit casts from other types
/// to bool. "TEnum" should be an enumerated type with two values:
///   - negative (FALSE/OFF) should have value 0
//    - positive (TRUE/ON) have value 1.

template <class TEnum>
class CBoolEnum
{
public:
    // Constructors
    CBoolEnum(bool  value) : m_Value(value) {}
    CBoolEnum(TEnum value) : m_Value(value ? true : false) {}

    /// Operator bool
    operator bool() const   { return m_Value; }
    /// Operator enum
    operator TEnum () const { return TEnum(m_Value); }

private:
    bool m_Value;

private:
    // Disable implicit conversions from/to other types
    CBoolEnum(char*);
    CBoolEnum(int);
    CBoolEnum(unsigned int);
    template <class T> CBoolEnum(T);

    operator int() const;
    operator unsigned int() const;
};



#ifdef HAVE_NO_AUTO_PTR


/////////////////////////////////////////////////////////////////////////////
///
/// auto_ptr --
///
/// Define auto_ptr if needed.
///
/// Replacement of STL's std::auto_ptr for compilers with poor "auto_ptr"
/// implementation.
/// 
/// See C++ Toolkit documentation for limitations and use of auto_ptr.

template <class X>
class auto_ptr
{
    // temporary class for auto_ptr copying
    template<class Y>
    struct auto_ptr_ref
    {
        auto_ptr_ref(auto_ptr<Y>& ptr)
            : m_AutoPtr(ptr)
            {
            }
        auto_ptr<Y>& m_AutoPtr;
    };
public:
    typedef X element_type;         ///< Define element_type

    /// Explicit conversion to auto_ptr.
    explicit auto_ptr(X* p = 0) : m_Ptr(p) {}

    /// Copy constructor with implicit conversion.
    ///
    /// Note that the copy constructor parameter is not a const
    /// because it is modified -- ownership is transferred.
    auto_ptr(auto_ptr<X>& a) : m_Ptr(a.release()) {}

    /// Assignment operator.
    auto_ptr<X>& operator=(auto_ptr<X>& a) {
        if (this != &a) {
            if (m_Ptr  &&  m_Ptr != a.m_Ptr) {
                delete m_Ptr;
            }
            m_Ptr = a.release();
        }
        return *this;
    }

    auto_ptr(auto_ptr_ref<X> ref)
        : m_Ptr(ref.m_AutoPtr.release())
    {
    }
    template <typename Y>
    operator auto_ptr_ref<Y>()
    {
        return auto_ptr_ref<Y>(*this);
    }

    /// Destructor.
    ~auto_ptr(void) {
        if ( m_Ptr )
            delete m_Ptr;
    }

    /// Deference operator.
    X&  operator*(void) const { return *m_Ptr; }

    /// Reference operator.
    X*  operator->(void) const { return m_Ptr; }

    /// Equality operator.
    int operator==(const X* p) const { return (m_Ptr == p); }

    /// Get pointer value.
    X*  get(void) const { return m_Ptr; }

    /// Release pointer.
    X* release(void) {
        X* x_Ptr = m_Ptr;  m_Ptr = 0;  return x_Ptr;
    }

    /// Reset pointer.
    void reset(X* p = 0) {
        if (m_Ptr != p) {
            delete m_Ptr;
            m_Ptr = p;
        }
    }

private:
    X* m_Ptr;               ///< Internal pointer implementation.
};

#endif /* HAVE_NO_AUTO_PTR */



/// Functor template for allocating object.
template<class X>
struct Creater
{
    /// Default create function.
    static X* Create(void)
    { return new X; }
};

/// Functor template for deleting object.
template<class X>
struct Deleter
{
    /// Default delete function.
    static void Delete(X* object)
    { delete object; }
};

/// Functor template for deleting array of objects.
template<class X>
struct ArrayDeleter
{
    /// Array delete function.
    static void Delete(X* object)
    { delete[] object; }
};

/// Functor template for the C language deallocation function, free().
template<class X>
struct CDeleter
{
    /// C Language deallocation function.
    static void Delete(X* object)
    { free(object); }
};



/////////////////////////////////////////////////////////////////////////////
///
/// AutoPtr --
///
/// Define an "auto_ptr" like class that can be used inside STL containers.
///
/// The Standard auto_ptr template from STL doesn't allow the auto_ptr to be
/// put in STL containers (list, vector, map etc.).  The reason for this is
/// the absence of copy constructor and assignment operator.
/// We decided that it would be useful to have an analog of STL's auto_ptr
/// without this restriction - AutoPtr.
///
/// Due to the nature of AutoPtr its copy constructor and assignment operator
/// modify the state of the source AutoPtr object as it transfers the ownership
/// to the target AutoPtr object.  Also, we added possibility to redefine the
/// way pointer will be deleted:  the second argument of the template allows
/// pointers from "malloc" in AutoPtr, or you can use "ArrayDeleter" (see
/// above) to properly delete an array of objects using "delete[]" instead
/// of "delete".  By default, the internal pointer is deleted by the C++
/// "delete" operator.
///
/// @sa
///   Deleter(), ArrayDeleter(), CDeleter()

template< class X, class Del = Deleter<X> >
class AutoPtr
{
public:
    typedef X element_type;         ///< Define element type.
    typedef Del deleter_type;       ///< Alias for template argument.

    /// Constructor.
    AutoPtr(element_type* p = 0)
        : m_Ptr(p), m_Data(true)
    { }

    /// Constructor.
    AutoPtr(element_type* p, const deleter_type& deleter)
        : m_Ptr(p), m_Data(deleter, true)
    { }

    /// Constructor, own the pointed object if ownership == eTakeOwnership
    AutoPtr(element_type* p, EOwnership ownership)
        : m_Ptr(p), m_Data(ownership != eNoOwnership)
    { }

    /// Constructor, own the pointed object if ownership == eTakeOwnership
    AutoPtr(element_type* p, const deleter_type& deleter, EOwnership ownership)
        : m_Ptr(p), m_Data(deleter, ownership != eNoOwnership)
    { }

    /// Copy constructor.
    AutoPtr(const AutoPtr<X, Del>& p)
        : m_Ptr(0), m_Data(p.m_Data)
    {
        m_Ptr = p.x_Release();
    }

    /// Destructor.
    ~AutoPtr(void)
    {
        reset();
    }

    /// Assignment operator.
    AutoPtr<X, Del>& operator=(const AutoPtr<X, Del>& p)
    {
        if (this != &p) {
            bool owner = p.m_Data.second();
            reset(p.x_Release());
            m_Data.second() = owner;
        }
        return *this;
    }

    /// Assignment operator.
    AutoPtr<X, Del>& operator=(element_type* p)
    {
        reset(p);
        return *this;
    }

    /// Bool operator for use in if() clause.
    DECLARE_OPERATOR_BOOL_PTR(m_Ptr);

    // Standard getters.

    /// Dereference operator.
    element_type& operator* (void) const { return *m_Ptr; }

    /// Reference operator.
    element_type* operator->(void) const { return  m_Ptr; }

    /// Get pointer.
    element_type* get       (void) const { return  m_Ptr; }

    /// Release will release ownership of pointer to caller.
    element_type* release(void)
    {
        m_Data.second() = false;
        return m_Ptr;
    }

    /// Reset will delete the old pointer (if owned), set content to the new
    /// value, and assume the ownership upon the new pointer by default.
    void reset(element_type* p = 0, EOwnership ownership = eTakeOwnership)
    {
        if ( m_Ptr != p ) {
            if (m_Ptr  &&  m_Data.second()) {
                m_Data.first().Delete(release());
            }
            m_Ptr   = p;
        }
        m_Data.second() = ownership != eNoOwnership;
    }

    void Swap(AutoPtr<X, Del>& a)
    {
        swap(m_Ptr,  a.m_Ptr);
        swap(m_Data, a.m_Data);
    }

    bool IsOwned(void) const { return m_Ptr  &&  m_Data.second(); }

private:
    element_type* m_Ptr;                  ///< Internal pointer representation.
    mutable pair_base_member<deleter_type, bool> m_Data; ///< State info.

    /// Release for const object.
    element_type* x_Release(void) const
    {
        return const_cast<AutoPtr<X, Del>*>(this)->release();
    }
};


/////////////////////////////////////////////////////////////////////////////
///
/// AutoArray --
///
/// "AutoPtr" like class for using with arrays
///
/// vector<> template comes with a performance penalty, since it always
/// initializes its content.  This template is not a vector replacement,
/// it's a version of AutoPtr<> tuned for array pointers.  For convenience
/// it defines array style access operator [] and size based contructor.
///
/// @sa AutoPtr
///

template< class X, class Del = ArrayDeleter<X> >
class AutoArray
{
public:
    typedef X element_type;         ///< Define element type.
    typedef Del deleter_type;       ///< Alias for template argument.

public:

    /// Construct the array using C++ new[] operator
    /// @note In this case you should use ArrayDeleter<> or compatible
    explicit AutoArray(size_t size)
        : m_Ptr(new element_type[size]), m_Data(true)
    { }

    explicit AutoArray(element_type* p = 0)
        : m_Ptr(p), m_Data(true)
    { }

    AutoArray(element_type* p, const deleter_type& deleter)
        : m_Ptr(p), m_Data(deleter, true)
    { }

    AutoArray(const AutoArray<X, Del>& p)
        : m_Ptr(0), m_Data(p.m_Data)
    {
        m_Ptr = p.x_Release();
    }

    ~AutoArray(void)
    {
        reset();
    }

    /// Assignment operator.
    AutoArray<X, Del>& operator=(const AutoArray<X, Del>& p)
    {
        if (this != &p) {
            bool owner = p.m_Data.second();
            reset(p.x_Release());
            m_Data.second() = owner;
        }
        return *this;
    }

    /// Assignment operator.
    AutoArray<X, Del>& operator=(element_type* p)
    {
        reset(p);
        return *this;
    }

    /// Bool operator for use in if() clause.
    DECLARE_OPERATOR_BOOL_PTR(m_Ptr);

    /// Get pointer.
    element_type* get (void) const { return  m_Ptr; }

    /// Release will release ownership of pointer to caller.
    element_type* release(void)
    {
        m_Data.second() = false;
        return m_Ptr;
    }

    /// Array style dereference (returns value)
    const element_type& operator[](size_t pos) const { return m_Ptr[pos]; }

    /// Array style dereference (returns reference)
    element_type&       operator[](size_t pos)       { return m_Ptr[pos]; }

    /// Reset will delete the old pointer, set content to the new value,
    /// and assume the ownership upon the new pointer.
    void reset(element_type* p = 0)
    {
        if (m_Ptr != p) {
            if (m_Ptr  &&  m_Data.second()) {
                m_Data.first().Delete(release());
            }
            m_Ptr  = p;
        }
        m_Data.second() = true;
    }

    void Swap(AutoPtr<X, Del>& a)
    {
        swap(m_Ptr,  a.m_Ptr);
        swap(m_Data, a.m_Data);
    }

private:
    /// Release for const object.
    element_type* x_Release(void) const
    {
        return const_cast<AutoArray<X, Del>*>(this)->release();
    }

private:
    element_type* m_Ptr;
    mutable pair_base_member<deleter_type, bool> m_Data; ///< State info.
};



/////////////////////////////////////////////////////////////////////////////
///
/// CNullable --
///
/// A value whith 'unassigned' state.
///


/// Define "null" pointer value.
enum ENull {
    null = 0
};


// Default callback for null value - throws CCoreException.
NCBI_XNCBI_EXPORT NCBI_NORETURN void g_ThrowOnNull(void);

// Default callback template.
template <class TValue>
struct SThrowOnNull
{
    TValue operator()(void) const
    {
        g_ThrowOnNull();
    }
};


/// Template class allowing to store a value or null (unassigned) state.
/// TNullToValue functor can be used to perform an action when the value
/// is requested from a null object. By default CCoreException is thrown.
/// To perform other actions (e.g. provide a default value) the functor
/// must define 'TValue operator()(void) const' method.
template <class TValue, class TNullToValue = SThrowOnNull<TValue> >
class CNullable
{
public:
    /// Create an empty nullable.
    CNullable(ENull = null)
        : m_IsNull(true) {}
    /// Initialize nullable with a specific value.
    CNullable(TValue value)
        : m_IsNull(false), m_Value(value) {}

    /// Check if the object is unassigned.
    bool IsNull(void) const
    {
        return m_IsNull;
    }

    /// Get nullable value.
    /// If NULL, then call TNullToValue and use the value return by the latter.
    /// @attention  The default implementation of TNullToValue (g_ThrowOnNull)
    ///             throws an exception! 
    operator TValue(void) const
    {
        return m_IsNull ? TNullToValue()() : m_Value;
    }

    /// Get a const reference to the current value. If NULL, the reference is
    /// a copy of the default value or throw (depending on TNullToValue
    /// implementation). In any case the IsNull state is unchanged.
    const TValue& GetValue(void) const
    {
        if ( m_IsNull ) {
            const_cast<TValue&>(m_Value) = TNullToValue()();
        }
        return m_Value;
    }

    /// Get a non-const reference to the value. If NULL, try to initialize this
    /// instance with the default value or throw.
    TValue& SetValue(void)
    {
        if ( m_IsNull ) {
            *this = TNullToValue()();
        }
        return m_Value;
    }

    /// Assign a value to the nullable.
    CNullable& operator= (TValue value)
    {
        m_IsNull = false; m_Value = value;
        return *this;
    }
    
    /// Reset nullable to unassigned state.
    CNullable& operator= (ENull  null_value)
    {
        m_IsNull = true;
        return *this;
    }

private:
    bool   m_IsNull;
    TValue m_Value;
};


// "min" and "max" templates
//

// Always get rid of the old non-conformant min/max macros
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#if defined(HAVE_NO_MINMAX_TEMPLATE)
#  define NOMINMAX

/// Min function template.
template <class T>
inline
const T& min(const T& a, const T& b) {
    return b < a ? b : a;
}

/// Max function template.
template <class T>
inline
const T& max(const T& a, const T& b) {
    return  a < b ? b : a;
}
#endif /* HAVE_NO_MINMAX_TEMPLATE */



// strdup()
//

#ifndef HAVE_STRDUP
/// Supply string duplicate function, if one is not defined.
extern char* strdup(const char* str);
#endif



//  ITERATE
//  NON_CONST_ITERATE
//  REVERSE_ITERATE
//  NON_CONST_REVERSE_ITERATE
//  ERASE_ITERATE
//
// Useful macros to write 'for' statements with the STL container iterator as
// a variable.
//

// *ITERATE helper to enforce constness of the container reference
template<typename Type>
inline const Type& s_ITERATE_ConstRef(const Type& obj)
{
    return obj;
}
#define ITERATE_CONST(Cont) NCBI_NS_NCBI::s_ITERATE_ConstRef(Cont)

// *ITERATE helper to verify that the container isn't a temporary object
#ifdef _DEBUG
template<typename Type>
inline bool s_ITERATE_SameObject(const Type& obj1, const Type& obj2)
{
    return &obj1 == &obj2;
}
# define ITERATE_BEGIN(Cont, Begin)                                     \
    (NCBI_ASSERT_EXPR(NCBI_NS_NCBI::s_ITERATE_SameObject(Cont, Cont),   \
                      "rvalue container in *ITERATE"), (Cont).Begin())
#else
# define ITERATE_BEGIN(Cont, Begin) ((Cont).Begin())
#endif

// *ITERATE helper macro to declare iterator variable
#if 0 && defined(NCBI_HAVE_CXX11)
# define ITERATE_VAR(Type) auto
#else
# define ITERATE_VAR(Type) Type
#endif

/// ITERATE macro to sequence through container elements.
#define ITERATE(Type, Var, Cont)                                        \
    for ( ITERATE_VAR(Type::const_iterator)                             \
              Var = ITERATE_BEGIN(ITERATE_CONST(Cont), begin),          \
              NCBI_NAME2(Var,_end) = ITERATE_CONST(Cont).end();         \
          Var != NCBI_NAME2(Var,_end);  ++Var )

/// Non constant version of ITERATE macro.
#define NON_CONST_ITERATE(Type, Var, Cont)                              \
    for ( ITERATE_VAR(Type::iterator) Var = ITERATE_BEGIN(Cont, begin); \
          Var != (Cont).end();  ++Var )

/// ITERATE macro to reverse sequence through container elements.
#define REVERSE_ITERATE(Type, Var, Cont)                                \
    for ( ITERATE_VAR(Type::const_reverse_iterator)                     \
              Var = ITERATE_BEGIN(ITERATE_CONST(Cont), rbegin),         \
              NCBI_NAME2(Var,_end) = ITERATE_CONST(Cont).rend();        \
          Var != NCBI_NAME2(Var,_end);  ++Var )

/// Non constant version of REVERSE_ITERATE macro.
#define NON_CONST_REVERSE_ITERATE(Type, Var, Cont)                      \
    for ( ITERATE_VAR(Type::reverse_iterator)                           \
              Var = ITERATE_BEGIN(Cont, rbegin);                        \
          Var != (Cont).rend();  ++Var )

/// Non-constant version with ability to erase current element, if container
/// permits. Use only on containers, for which erase do not ruin other
/// iterators into the container, e.g. map, list, but NOT vector.
/// See also VECTOR_ERASE
#define ERASE_ITERATE(Type, Var, Cont) \
    for ( ITERATE_VAR(Type::iterator) Var = ITERATE_BEGIN(Cont, begin), \
              NCBI_NAME2(Var,_next) = Var;                              \
          (Var = NCBI_NAME2(Var,_next)) != (Cont).end() &&              \
              (++NCBI_NAME2(Var,_next), true); )

/// Use this macro inside body of ERASE_ITERATE cycle to erase from
/// vector-like container. Plain erase() call would invalidate Var_next
/// iterator and would make the cycle controlling code to fail.
#define VECTOR_ERASE(Var, Cont) (NCBI_NAME2(Var,_next) = (Cont).erase(Var))

/// The body of the loop will be run with Var equal to false and then true.
/// The seemlingly excessive complexity of this macro is to get around a couple of limitations:
/// * A bool only has two states, so it's not possible to represent the complete state space
///   of (first iteration, second iteration, done) without another variable.
/// * The variables declared in a for-loop's first part must be of the same type, so
///   the other variable has to be a bool instead of something more convenient such as
///   as a loop-counter.
#define ITERATE_BOTH_BOOL_VALUES(BoolVar) \
    for( bool BoolVar##BOTH_BOOL_VALUES_DONE##__LINE__ = false, BoolVar = false; ! BoolVar##BOTH_BOOL_VALUES_DONE##__LINE__  ; BoolVar##BOTH_BOOL_VALUES_DONE##__LINE__ = BoolVar, BoolVar = true )

/// idx loops from 0 (inclusive) to up_to (exclusive)
#define ITERATE_0_IDX(idx, up_to) \
    for( TSeqPos idx = 0; idx < up_to; ++idx )

/// Just repeat the body of the loop num_iters times
#define ITERATE_SIMPLE(num_iters) \
    ITERATE_0_IDX( _dummy_idx_94768308_##__LINE__, num_iters ) // the number has no significance; it is entirely random.

/// Type for sequence locations and lengths.
///
/// Use this typedef rather than its expansion, which may change.
typedef unsigned int TSeqPos;

/// Define special value for invalid sequence position.
const TSeqPos kInvalidSeqPos = ((TSeqPos) (-1));


/// Type for signed sequence position.
///
/// Use this type when and only when negative values are a possibility
/// for reporting differences between positions, or for error reporting --
/// though exceptions are generally better for error reporting.
/// Use this typedef rather than its expansion, which may change.
typedef int TSignedSeqPos;

/// Type for Taxon1-name.taxid
typedef int TTaxId;

/// Type for sequence GI.
///
/// Use this typedef rather than its expansion, which may change.

//#define NCBI_STRICT_GI
//#define NCBI_INT8_GI
//#define NCBI_STRICT_ENTREZ_ID

#ifdef NCBI_STRICT_GI
# define NCBI_INT8_GI
#endif

#ifdef NCBI_INT8_GI

// Generic id type which needs to be the same size as GI.
typedef Int8 TIntId;
typedef Uint8 TUintId;

#ifdef NCBI_STRICT_GI

// Strict mode can be enabled only for Int8 GIs.

class CStrictId64
{
public:
    CStrictId64(void) : m_Gi(0) {}

    bool operator==(const CStrictId64& gi) const { return m_Gi == gi.m_Gi; }
    bool operator!=(const CStrictId64& gi) const { return m_Gi != gi.m_Gi; }
    bool operator<(const CStrictId64& gi) const { return m_Gi < gi.m_Gi; }
    bool operator<=(const CStrictId64& gi) const { return m_Gi <= gi.m_Gi; }
    bool operator>(const CStrictId64& gi) const { return m_Gi > gi.m_Gi; }
    bool operator>=(const CStrictId64& gi) const { return m_Gi >= gi.m_Gi; }

    CStrictId64& operator++(void) { m_Gi++; return *this; }
    CStrictId64 operator++(int) { CStrictId64 tmp = *this; m_Gi++; return tmp; }
    CStrictId64& operator--(void) { m_Gi--; return *this; }
    CStrictId64 operator--(int) { CStrictId64 tmp = *this; m_Gi--; return tmp; }

    TIntId operator+(const CStrictId64& gi) const { return m_Gi + gi.m_Gi; }
    TIntId operator-(const CStrictId64& gi) const { return m_Gi - gi.m_Gi; }

    CStrictId64 operator+(TIntId offset) const { return m_Gi + offset; }
    CStrictId64 operator-(TIntId offset) const { return m_Gi - offset; }
#if defined(NCBI_INT8_GI)
    CStrictId64 operator+(Int4 offset) const { return m_Gi + offset; }
    CStrictId64 operator-(Int4 offset) const { return m_Gi - offset; }
#endif

    CStrictId64(TIntId value) : m_Gi(value) {}
    CStrictId64& operator=(TIntId value) { m_Gi = value; return *this; }
    operator TIntId(void) const { return m_Gi; }
    bool operator==(TIntId value) const { return m_Gi == value; }

#if defined(NCBI_INT8_GI) && defined(NCBI_TEST_APPLICATION)
    CStrictId64(Int4 value) : m_Gi(value) {}
    CStrictId64& operator=(Int4 value) { m_Gi = value; return *this; }
    bool operator==(Int4 value) const { return m_Gi == value; }
#endif

    operator bool(void) const { return m_Gi != 0; }

private:
#if defined(NCBI_INT8_GI) && !defined(NCBI_TEST_APPLICATION)
    CStrictId64(Int4);
    CStrictId64& operator=(Int4);
    operator Int4(void) const;
#endif
    CStrictId64(Int1);
    CStrictId64(Uint1);
    CStrictId64(Int2);
    CStrictId64(Uint2);
    CStrictId64(Uint4);
    CStrictId64(Uint8);
    CStrictId64(float);
    CStrictId64(double);
    CStrictId64& operator=(Int1);
    CStrictId64& operator=(Uint1);
    CStrictId64& operator=(Int2);
    CStrictId64& operator=(Uint2);
    CStrictId64& operator=(Uint4);
    CStrictId64& operator=(Uint8);
    CStrictId64& operator=(float);
    CStrictId64& operator=(double);
    operator Int1(void) const;
    operator Uint1(void) const;
    operator Int2(void) const;
    operator Uint2(void) const;
    operator Uint4(void) const;
    operator Uint8(void) const;
    operator float(void) const;
    operator double(void) const;

private:
    TIntId m_Gi;
};

/// @deprecated: Use CStrictId64 instead of CStrictGi, or TGi/TEntrezId typedefs.
NCBI_DEPRECATED typedef CStrictId64 CStrictGi;


inline
CNcbiOstream& operator<<(CNcbiOstream& out, const CStrictId64& gi)
{
    return out << TIntId(gi);
}

inline
CNcbiIstream& operator>>(CNcbiIstream& in, CStrictId64& gi)
{
    TIntId id;
    in >> id;
    gi = id;
    return in;
}

typedef CStrictId64 TGi;

#else // NCBI_STRICT_GI

typedef TIntId TGi;

#endif // NCBI_STRICT_GI

#else // NCBI_INT8_GI

typedef int TGi;
typedef Int4 TIntId;
typedef Uint4 TUintId;

#endif

/// TEntrezId type for entrez ids which require the same strictness as TGi.
#ifdef NCBI_STRICT_ENTREZ_ID
# ifndef NCBI_STRICT_GI
#  undef NCBI_STRICT_ENTREZ_ID
# endif
typedef TGi TEntrezId;
#else
typedef TIntId TEntrezId;
#endif

/// a helper template to enforce constness of argument to GI_CONST macro
template<TIntId gi>
class CConstGIChecker {
public:
    static const TIntId value = gi;
};
#define GI_CONST(gi) (TGi(CConstGIChecker<gi>::value))
#define ZERO_GI GI_CONST(0)
#define INVALID_GI GI_CONST(-1)

/// Temporary macros to convert TGi to other types (int, unsigned etc.).
#define GI_TO(T, gi) (static_cast<T>(TIntId(gi)))
#define GI_FROM(T, value) (TGi(static_cast<TIntId>(value)))

/// Convert gi-compatible int to/from other types.
#define INT_ID_TO(T, id) (static_cast<T>(id))
#define INT_ID_FROM(T, value) (static_cast<TIntId>(value))


/// Helper address class
class CRawPointer
{
public:
    /// add offset to object reference (to get object's member)
    static void* Add(void* object, ssize_t offset);
    static const void* Add(const void* object, ssize_t offset);
    /// calculate offset inside object
    static ssize_t Sub(const void* first, const void* second);
};


inline
void* CRawPointer::Add(void* object, ssize_t offset)
{
    return static_cast<char*> (object) + offset;
}

inline
const void* CRawPointer::Add(const void* object, ssize_t offset)
{
    return static_cast<const char*> (object) + offset;
}

inline
ssize_t CRawPointer::Sub(const void* first, const void* second)
{
    return (ssize_t)/*ptrdiff_t*/
        (static_cast<const char*> (first) - static_cast<const char*> (second));
}



/// Buffer with an embedded pre-reserved space.
///
/// It is convenient to use if you want to avoid allocation in heap
/// for smaller requested sizes, and use stack for such cases.
/// @example:
///    CFastBuffer<2048> buf(some_size);

template <size_t KEmbeddedSize, class TType = char>
class CFastBuffer
{
public:
    CFastBuffer(size_t buf_size)
        : m_Size(buf_size),
          m_Buffer(buf_size <= KEmbeddedSize ? m_EmbeddedBuffer : new TType[buf_size])
    {}
    ~CFastBuffer() { if (m_Buffer != m_EmbeddedBuffer) delete[] m_Buffer; }

    TType        operator[] (size_t pos) const { return m_Buffer[pos]; }

    TType&       operator* (void)       { return *m_Buffer; }
    const TType& operator* (void) const { return *m_Buffer; }

    TType*       operator+ (size_t offset)       { return m_Buffer + offset; }
    const TType* operator+ (size_t offset) const { return m_Buffer + offset; }

    TType*       begin(void)       { return m_Buffer; }
    const TType* begin(void) const { return m_Buffer; }

    TType*       end(void)       { return m_Buffer + m_Size; }
    const TType* end(void) const { return m_Buffer + m_Size; }

    size_t       size(void) const { return m_Size; }

private:
     size_t m_Size;
     TType* m_Buffer;
     TType  m_EmbeddedBuffer[KEmbeddedSize];
};



/// Macro used to mark a constructor as deprecated.
///
/// The correct syntax for this varies from compiler to compiler:
/// older versions of GCC (prior to 3.4) require NCBI_DEPRECATED to
/// follow any relevant constructor declarations, but some other
/// compilers (Microsoft Visual Studio 2005, IBM Visual Age / XL)
/// require it to precede any relevant declarations, whether or not
/// they are for constructors.
#if defined(NCBI_COMPILER_MSVC) || defined(NCBI_COMPILER_VISUALAGE)
#  define NCBI_DEPRECATED_CTOR(decl) NCBI_DEPRECATED decl
#else
#  define NCBI_DEPRECATED_CTOR(decl) decl NCBI_DEPRECATED
#endif

/// Macro used to mark a class as deprecated.
///
/// @sa NCBI_DEPRECATED_CTOR
#define NCBI_DEPRECATED_CLASS NCBI_DEPRECATED_CTOR(class)


/////////////////////////////////////////////////////////////////////////////
/// Support for safe enum flags
/////////////////////////////////////////////////////////////////////////////
///
/// The CSafeFlags enum is used to define flags with enum definition
/// so that they preserve type after bit-wise operations,
/// and doesn't allow to be used in context of another enum/flags/int type.
/// With the definition of enum, usually inside class that uses it:
///   class CMyClass {
//    public:
///        enum EFlags {
///           fFlag1 = 1<<0,
///           fFlag2 = 1<<1,
///           fFlag3 = 1<<1,
///           fMask  = 0|fFlag1|fFlag2
///       };
/// you can add at the same level a macro declaration:
///       DECLARE_SAFE_FLAGS_TYPE(EFlags, TFlags);
///   };
/// and then outside the class, or at the same level if it's not a class definition:
///   DECLARE_SAFE_FLAGS(EMyFlags::EFlags);
/// The first macro DECLARE_SAFE_FLAGS_TYPE() declares a typedef for safe-flags.
/// The second macro DECLARE_SAFE_FLAGS() marks the enum as a safe-flags enum, so that
/// bit-wise operations on its values will preserve the safe-flags type.
/// The zero or in fMask definition is necessary to prefent incorrect early instantiation
/// of safe-flags definition on MSVC, so that combined flag value could be assigned.
/// No other modification of sources is necessary for correctly used enum.
/// In case the enum values are used in a wrong context, like as a different enum,
/// compilation error will occur.
/// If the use in a different context is actually necessary you can enforce it by casting
/// enum values to int either explicitly, or implicitly by or-ing with zero like in
/// the example code above.

/// Safe enum flags template, not to be used explicitly.
/// Use macros DECLARE_SAFE_FLAGS_TYPE and DECLARE_SAFE_FLAGS instead.
template<class Enum>
class CSafeFlags {
public:
    typedef Enum enum_type;
    typedef typename underlying_type<enum_type>::type storage_type;

    CSafeFlags()
        : m_Flags(0)
        {
        }
    CSafeFlags(enum_type flags)
        : m_Flags(flags)
        {
        }

    storage_type get() const
        {
            return m_Flags;
        }

    DECLARE_OPERATOR_BOOL(m_Flags != 0);

    bool operator==(const CSafeFlags& b) const
        {
            return get() == b.get();
        }
    bool operator!=(const CSafeFlags& b) const
        {
            return get() != b.get();
        }

    CSafeFlags operator~() const
        {
            return CSafeFlags(~get());
        }
    
    CSafeFlags operator&(const CSafeFlags& b) const
        {
            return CSafeFlags(get() & b.get());
        }
    CSafeFlags& operator&=(const CSafeFlags& b)
        {
            m_Flags &= b.get();
            return *this;
        }
    
    CSafeFlags operator|(const CSafeFlags& b) const
        {
            return CSafeFlags(get() | b.get());
        }
    CSafeFlags& operator|=(const CSafeFlags& b)
        {
            m_Flags |= b.get();
            return *this;
        }
    
    CSafeFlags operator^(const CSafeFlags& b) const
        {
            return CSafeFlags(get() ^ b.get());
        }
    CSafeFlags& operator^=(const CSafeFlags& b)
        {
            m_Flags ^= b.get();
            return *this;
        }
    
    CSafeFlags without(const CSafeFlags& b) const
        {
            return CSafeFlags(get()&~b.get());
        }
    CSafeFlags& reset(const CSafeFlags& b)
        {
            m_Flags &= ~b.get();
            return *this;
        }

private:
    explicit
    CSafeFlags(storage_type flags)
        : m_Flags(flags)
        {
        }

    storage_type m_Flags;
};
/// Macro DECLARE_SAFE_FLAGS_TYPE defines a typedef for safe-flags.
/// First argument is enum name, second argument is the new typedef name.
/// In place of old code:
///   typedef EFlags TFlags;
/// put new macro:
///   DECLARE_SAFE_FLAGS_TYPE(EFlags, TFlags);
#define DECLARE_SAFE_FLAGS_TYPE(E, T)           \
    typedef NCBI_NS_NCBI::CSafeFlags<E> T

/// Macro DECLARE_SAFE_FLAGS marks a enum as safe-flags enum.
/// The argument is the enum name.
/// If the enum is defined inside a class then DECLARE_SAFE_FLAGS()
/// must be placed outside the class definition:
///   DECLARE_SAFE_FLAGS(CMyClass::EFlags);
#define DECLARE_SAFE_FLAGS(E)                                           \
    inline NCBI_NS_NCBI::CSafeFlags<E> AsSafeFlags(const E* ptr) { return *ptr; }

/// Helper operators for safe-flags enums.
/// These operators will be used only for enums marked
/// as safe-flag enums by macro DECLARE_SAFE_FLAGS()
template<class E> inline
decltype(AsSafeFlags(static_cast<const E*>(0))) operator|(E a, E b)
{
    return CSafeFlags<E>(a)|b;
}
template<class E> inline CSafeFlags<E> operator|(E a, CSafeFlags<E> b)
{
    return b | a;
}
template<class E> inline
decltype(AsSafeFlags(static_cast<const E*>(0))) operator&(E a, E b)
{
    return CSafeFlags<E>(a)&b;
}
template<class E> inline CSafeFlags<E> operator&(E a, CSafeFlags<E> b)
{
    return b & a;
}
template<class E> inline
decltype(AsSafeFlags(static_cast<const E*>(0))) operator^(E a, E b)
{
    return CSafeFlags<E>(a)^b;
}
template<class E> inline CSafeFlags<E> operator^(E a, CSafeFlags<E> b)
{
    return b ^ a;
}
template<class E> inline
decltype(AsSafeFlags(static_cast<const E*>(0))) operator~(E a)
{
    return ~CSafeFlags<E>(a);
}
template<class E> inline
ostream& operator<<(ostream& out, CSafeFlags<E> v)
{
    return out << v.get();
}


END_NCBI_NAMESPACE;

BEGIN_STD_NAMESPACE;

template<class T1, class T2>
inline
void swap(NCBI_NS_NCBI::pair_base_member<T1,T2>& pair1,
          NCBI_NS_NCBI::pair_base_member<T1,T2>& pair2)
{
    pair1.Swap(pair2);
}


template<class P, class D>
inline
void swap(NCBI_NS_NCBI::AutoPtr<P,D>& ptr1,
          NCBI_NS_NCBI::AutoPtr<P,D>& ptr2)
{
    ptr1.Swap(ptr2);
}


#if (defined(NCBI_COMPILER_GCC) && NCBI_COMPILER_VERSION < 340)  ||  defined(NCBI_COMPILER_WORKSHOP)  ||  defined(NCBI_COMPILER_MIPSPRO)

#define ArraySize(array)  (sizeof(array)/sizeof((array)[0]))

#else

template<class Element, size_t Size>
inline
size_t ArraySize(const Element (&)[Size])
{
    return Size;
}

#ifdef NCBI_STRICT_GI
template <> struct hash<ncbi::CStrictId64>
{
    size_t operator()(const ncbi::CStrictId64 & x) const
    {
        return hash<ncbi::TIntId>()((ncbi::TIntId)x);
    }
};
#endif /* NCBI_STRICT_GI */

#endif

END_STD_NAMESPACE;

/* @} */

#endif  /* CORELIB___NCBIMISC__HPP */
