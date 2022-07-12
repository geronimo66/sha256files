/****************************************************************************************************************************
 * Copyleft (c) 2022 by Marco Gerodetti                                                                                     *
 *                                                                                                                          *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public        *
 * License as published by the Free Software Foundation, version 2. This program is distributed WITHOUT ANY WARRANTY;       *
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public     *
 * License for more details <http://www.gnu.org/licenses/gpl-2.0.html>.                                                     *
 *                                                                                                                          *
 *   written in simple C++, POSIX compatibility for Environmental requirements, alternativ for windows to                   *
 *   using c++ ISO/IEC 14882:2011, POSIX c-libs IEEE Std 1003.1, https://pubs.opengroup.org/onlinepubs/9699919799/).        *
 *                                                                                                                          *
 * design paradigms:                                                                                                        *
 *   POSIX libs & environment using              => high compatibility, no 3rd party libs                                   *
 *   Few Sourcefile & libs                       => easy to handle, few complexity                                          *
 ****************************************************************************************************************************/

//  Unpublished Version, NOT To USE

#ifndef base_hpp
#define base_hpp

// C- Standard-Libs complaind with POSIX IEEE Std 1003.1-2017
#include <math.h>
#include <ctype.h>
#include <limits.h>

#if defined _WIN32          // Supports MS-Windows or POSIX
#define POSIX false
#else // UNIX/LINUX
#define POSIX true
#endif

#if POSIX
    #include <errno.h>
    #include <stdint.h>
    typedef int8_t         i8;
    typedef int16_t        i16;
    typedef int32_t        i32;
    typedef int64_t        i64;
    typedef uint8_t        u8;
    typedef uint16_t       u16;
    typedef uint32_t       u32;
    typedef uint64_t       u64;
    #define CONSTEXPR_IF constexpr
    #define path_separator '/'
    #define path_separator_string "/"
    /* Adjusted the POSIX similarities of Windows                                          */
#else // _WIN32
    typedef char           i8;
    typedef short          i16;
    typedef int            i32;
    typedef long long      i64;
    typedef unsigned char  u8;
    typedef unsigned short u16;
    typedef unsigned int   u32;
    typedef unsigned long long  u64;
    #define CONSTEXPR_IF const
    #define path_separator '\\'
    #define path_separator_string "\\"
    #define LITLE_ENDIAN 1
    #define HOST_ORDER LITLE_ENDIAN
#endif

typedef char tchar;

class Static {
protected:
    constexpr Static() noexcept { };
    constexpr void operator=(Static const&) = delete;
};

class Independent : public Static {
protected:
    constexpr Independent()  noexcept { };
    ~Independent() noexcept { };
    constexpr Independent(Independent const&) = delete;
};

template <typename T> 
constexpr  T min(const T  a, const T  b) noexcept { return a < b ? a : b; }
template <typename T> 
constexpr  T max(const T  a, const T  b) noexcept { return a > b ? a : b; }

struct narrow{
    constexpr static i64 to_i64( const u64 a) { return min<u64>( a, 0x7fffffffffffffff); }
    constexpr static u64 to_u64( const i64 a) { return max<u64>( a, 0);  }
};


// Span or an Range from begin till one before end; reaching end means overrun. begin == end means nothing. end == 1+begin means point.
template <typename T> 
class Span {
private:
    const T m_begin;
    const T m_end;
public:
    constexpr  Span(const Span<T>& o)                    noexcept : m_begin(o.begin()), m_end(o.end()) {}
    constexpr  Span(const T p_b, const T p_e)            noexcept : m_begin(min(p_b, p_e)), m_end(max(p_b, p_e)) {}
    constexpr  T        limit(const T i)           const noexcept { return max(begin(), min(i, end())); }
    constexpr  bool     conatins(const T c)        const noexcept { return c >= m_begin && c <= m_end; }
    constexpr  T        begin()                    const noexcept { return m_begin; };
    constexpr  T        end()                      const noexcept { return m_end; };
    constexpr  u64      count()                    const noexcept { return narrow::to_u64( this->end() - this->begin()); }
};


// Index between Span. Indexing from begin till one before end; reaching end means overrun. begin == end means nothing. end == begin+1 means one point.
template <typename T> 
class Index : public Span<const T> {
private:
    T m_current;
    constexpr  T       get_index()                 const noexcept { return this->limit(m_current); }
    constexpr  void    set_index(T p_idx)                noexcept { m_current = this->limit(p_idx); }
protected:
    constexpr  void    move_index(u64 add)               noexcept { if (!eof()) m_current = this->limit(m_current + add); }
public:
    constexpr  Index(const Span<const T>  p)             noexcept : Span<const T>(p.begin(), p.end()), m_current(p.begin()) {}
    constexpr  Index(const T p_b, const T p_e)           noexcept : Span<const T>(p_b, p_e), m_current(p_b) {}
    constexpr  const bool eof()                    const noexcept { return m_current >= this->end(); }
    constexpr  T    current()                      const noexcept { return get_index(); }
    constexpr  T    get_next()                           noexcept { T r = current(); if (!eof()) m_current++; return r; }
    constexpr  void resetIndex(u64 p_index = 0)          noexcept { set_index(this->begin() + p_index); }
    constexpr  u64  past_count()                   const noexcept { return narrow::to_u64( this->current() - this->begin()); }
    constexpr  u64  future_count()                 const noexcept { return narrow::to_u64( this->end() - this->current()); }
};

template <typename T> 
class ArraySpan : public Span< T* const> {
public:
    constexpr ArraySpan(const Span<T* const> &p_a)       noexcept  : Span< T* const>( p_a.begin(),  p_a.end()) { }
    constexpr ArraySpan(const ArraySpan<T> &p_a,
                        const u64 start,
                        const u64 end = ULLONG_MAX)      noexcept  : Span< T* const>( &p_a[ start], &p_a[end]) { }

    constexpr inline T&  operator []( u64 idx)  const noexcept {
        return  this->begin()[ min<u64>( idx, this->count()) ];
    }
    constexpr bool isIn(const T c)                             const noexcept {
        for (auto cx : *this) if (cx == c) 
            return true; 
        return false; 
    }
    constexpr T    isIn(const T c, const T def)                const noexcept { return isIn( c) ? c : def; }
    constexpr bool equals( const ArraySpan< const T> &b)       const noexcept {
        const ArraySpan< T> &a = *this;
        if ( a.count() != b.count())
            return false;
        for ( auto i = 0; i < a.count(); i++)
            if ( a[ i] != b[ i])
                return false;
        return true;
    }
};

template <typename T> constexpr 
void memClean(const ArraySpan<T> &m_ar)  noexcept {
    constexpr T empty = 0; // ensure T is atomic integer
    for (T &e : m_ar)
        e = empty;
}
template <typename T> constexpr 
void memClean(const T &o)  noexcept {
    memClean<u8>(ArraySpan<u8>({ (u8* const)&o, ((u8* const)&o) + sizeof(T) - 1 }));
}

template <typename T, u32 SIZE> constexpr 
void cpyArray ( const T(&p_src)[SIZE], T(&p_dest)[SIZE])             noexcept {
    u64 i = 0;
    for ( T &dest : p_dest)
        dest = p_src[ i++];
}

template <typename T> using ArrayIndex = Index< T*>;

// Containers to Instance on globlal memory (or on stack, but careful pelase)
template <class TBASE, typename T, u32 SIZE> 
class DContainer : public TBASE {
protected:
    T m_array[SIZE + 1] = { 0 };  // add last element as spare, with 0 value and must be out of range..
    const T CEOF = 0; // ensure T is primitive
public:
    constexpr        DContainer()                                    noexcept : TBASE(m_array) { m_array[SIZE] = CEOF; }
};

template <typename T> 
class Array : public ArraySpan< T> {
public:
    template<size_t SIZE>
    constexpr  Array(T(&p_a)[SIZE])            noexcept
    : ArraySpan< T>( { &p_a[0], &p_a[SIZE] - 1 }) { }  // without End- Marker
};

template <typename T> using CArray = Array< const T>;

template< class T1, class T2> struct Pair{
    T1 key;
    T2 value;
};
 
template< class T1, class T2>
struct KeyPairs : CArray< Pair< const T1 , const T2 >> {
    const Pair< const T1 , const T2 > m_default;
    
    template<size_t SIZE>
    constexpr KeyPairs<const T1, const T2>( const Pair< const T1 , const T2 >( &p_a)[SIZE], const Pair< const T1 , const T2 > p_default )
    : m_default(p_default)
    , CArray< Pair< const T1 , const T2 > > ( p_a) { }
    
    constexpr T2 valueOf( const T1 &key) const noexcept {
        for ( Pair< const T1 , const T2 > e : *this )
            if ( e.key == key )
                return e.value;
        return m_default.value;
    };
};

template <typename T, u32 SIZE> using ArrayContainer = DContainer< Array< T>, T, SIZE >;

class CString : public ArraySpan< const tchar> {
public:
    template<size_t SIZE> constexpr CString( const tchar(&p_a)[SIZE]) noexcept
        : ArraySpan<const tchar>( { &p_a[0], &p_a[SIZE - 1] }) { } // last element is spare, 0 value and out of range..
};


template<size_t SIZE>
class CStringInstance : public CString {
    struct ConstString_ {
        char m_value[ SIZE];
        constexpr ConstString_( char const (&str)[SIZE]) {  for ( int i=0; i < SIZE ; i++ )  m_value[i] = str[i]; }
    };
    const ConstString_ m_value;
public:
    constexpr CStringInstance( const tchar(&p_a)[SIZE]) noexcept
    : m_value( p_a)
    , CString( m_value.m_value) { } // last element is spare, 0 value and out of range..
};

template <typename T> 
class DArray : public ArrayIndex<T>, Static {
protected:
    template<size_t SIZE> constexpr DArray(T(&p_a)[SIZE])            noexcept
        : ArrayIndex<T>(&p_a[0], &p_a[SIZE - 1]) { } // -1 => last element is spare, 0 value and out of range.

    constexpr DArray<T>& putIn(T c)                                  noexcept {
        *this->get_next() = c;
        *this->current() = this->CEOF;
        return *this;
    }
    template <typename TT> 
    constexpr DArray<T>& streamIn(const TT &v)                       noexcept {
        for (const T c : v)
            putIn(c);
        return *this;
    }
    constexpr DArray<T>& streamReset()                               noexcept {
        this->resetIndex();
        memClean<T>(ArraySpan<T>({ this->begin(), this->end() }));
        return *this;
    }
public:
    static constexpr T CEOF = 0;  //BytesOf<T>::pattern(0);

    constexpr  ArraySpan<T> reader()                           const noexcept { return ArraySpan<T>({ ArrayIndex<T>::begin(), ArrayIndex<T>::current() }); }

    constexpr  u64 update_contend_end(u64 offset)                    noexcept {
        this->move_index(offset);
        *this->current() = this->CEOF;
        return offset;
    }
    template<u16 SIZE>
    constexpr  DArray<T>& operator << (const T(&p_v)[SIZE])          noexcept { return this->streamIn(p_v); }
    constexpr  DArray<T>& operator << (const T p_v)                  noexcept { return this->putIn(p_v); }
    constexpr  DArray<T>& operator << (const ArraySpan<T> p_v)       noexcept { return this->streamIn(p_v); }
    constexpr  DArray<T>& operator << (const ArraySpan<const T> p_v) noexcept { return this->streamIn(p_v); }
    constexpr  DArray<T>& operator << (const char *p_v)              noexcept {
        while (p_v != nullptr && *p_v != 0 && this->future_count() > 0)
            *this << *p_v++;
        return *this;
    }
    constexpr  DArray<T>& reset()                                    noexcept { return this->streamReset(); }
};

using DString = DArray< tchar>;

template <typename T, u32 SIZE> using DArrayContainer  = DContainer< DArray< T>, T, SIZE >;
template <u32 SIZE>             using DStringContainer = DContainer< DString, tchar, SIZE  >;

constexpr char inline toDigit(i64 b) noexcept { return "0123456789abcdef"[b & 0x0f]; };

template < u8 DIGITS = 1, u8 BASIS = 10, char SPACE = '0'> struct Scalar {
    const i64 m_v;
    constexpr Scalar(const i64 p_v)                                  noexcept : m_v(p_v) {};
    constexpr  void serialize(DString& p_stream) const noexcept {
        i64 v = m_v < 0 ? m_v * -1 : m_v; // substid abs() to be constexpr
        if (m_v < 0)
            p_stream << '-';
        i64 divisor = 1;
        i8 digits = DIGITS;
        for (i64 c = v; c >= BASIS; digits--, c /= BASIS)
            divisor *= BASIS;
        while (--digits > 0)
            p_stream << SPACE;
        for (; divisor > 0; divisor /= BASIS) {
            const tchar c = ( toDigit(v / divisor));
            p_stream << c;
            v %= divisor;
        }
    }
};

template <u8 DIGITS = 0, char SPACE = '0'>
using Num = Scalar<DIGITS, 10, SPACE>;
template <u8 DIGITS = 2, char SPACE = '0'>
using Hex = Scalar<DIGITS, 16, SPACE>;

template < u8 DIGITS = 1, u8 BASIS = 10, char SPACE = '0'>
constexpr DString& operator << (DString& p_s, const Scalar<DIGITS, BASIS, SPACE> &p_v) noexcept {
    p_v.serialize(p_s); return p_s;
}

template <typename T, u8 BASIS = 10, char DELEMITER = '\0', typename T1> 
constexpr DString& serialize( DString& p_stream, T1 p_src)         noexcept {
    char sendingDelemiter = '\0';
    for (T hb : p_src) {
        if (sendingDelemiter != '\0')
            p_stream << DELEMITER;
        else
            sendingDelemiter = DELEMITER;
        p_stream << Hex<sizeof(T) * 2>( hb);
    }
    return p_stream;
}

enum Endianes { Big, Little };

template <typename T, Endianes TEndianes> class BytesOfScalar {
protected:
    template<int N>
    struct byteConsts {
        u8 shift[N];
        T   mask[N];
        constexpr byteConsts() noexcept : shift(), mask() {
            for (u8 i = 0; i < N; i++) {
                shift[i] = 8 * (TEndianes == Endianes::Little ? i : N - 1 - i);
                mask[i] = ~((T)0xff << shift[i]);
            }
        }
    };
    constexpr static auto btc = byteConsts< sizeof(T)>();
    
    constexpr static T fromBytes(const u8 (&p_a)[sizeof(T)] ){
        BytesOfScalar< T, TEndianes> ret;
        for ( u8 i = 0; i < sizeof(T); i++ )
            ret.setByte( p_a[ i], i);
        return ret.m_v;
    }
public:
    T m_v = 0;
    constexpr BytesOfScalar( const T p_value = 0)             noexcept : m_v( p_value) { }
    constexpr BytesOfScalar( const u8 (&p_a)[sizeof(T)])      noexcept : m_v( fromBytes( p_a)) { }
    constexpr u8 getByte( const u8 index)               const noexcept { return 0xff & (m_v >> btc.shift[index % sizeof(T)]); }
    constexpr void setByte( const u8 byte, u8 index)          noexcept {
        index %= sizeof(T);
        m_v &= btc.mask[index];
        m_v |= ( byte << btc.shift[index]);
    }
};

template <typename T, Endianes TEndianes> 
struct ByteArrayOfScalar :  public  ArrayContainer< u8, sizeof(T) > {
    constexpr inline ByteArrayOfScalar( const BytesOfScalar< T, TEndianes> p_v) noexcept :  ArrayContainer< u8, sizeof(T) >() {
        u64 i = 0;
        for ( u8 &byt : this->m_array )
            byt = p_v.getByte(i++);
    }
};

struct IP_Def {
    const CStringInstance<13> m_name;
    const BytesOfScalar<u32, Endianes::Big> m_ip;
    const u16 m_port;
    constexpr IP_Def(const char(&p_name)[13], const u8 p_ip4, const u8 p_ip3, const u8 p_ip2, const u8 p_ip1, u16 _port) noexcept
    : m_name(p_name), m_ip( { p_ip1, p_ip2, p_ip3, p_ip4} ), m_port(_port) { }
};

constexpr DArray<char>& operator << ( DArray<char>& p_s, const IP_Def &p) noexcept {
    return p_s << p.m_name << ": "
    << Num<3>( p.m_ip.getByte(3)) << '.' << Num<3>( p.m_ip.getByte(2)) << '.' << Num<3>( p.m_ip.getByte(1)) <<'.' << Num<3>( p.m_ip.getByte(0))
    << ':' << Num<>( p.m_port);
}
typedef const IP_Def * const IP;

constexpr CStringInstance cAllowdSpecCharOfFileName("-+.");
constexpr CStringInstance cPathDelemiters("\\:/");

template <typename T> 
constexpr ArraySpan<T> ExtractFileName(const ArraySpan<T> &path) {
    u64 start_pos = 0, i = 0;
    for (const char c : path)
        if ( cPathDelemiters.isIn(c)) start_pos = ++i;
        else  ++i;
    return ArraySpan<T>(path, start_pos);
}

constexpr 
inline bool CheckFileNameChar(const char c, bool p_allows_special_chars) {
    return (Span<const char>('A', 'Z').conatins(c) || Span<const char>('a', 'z').conatins(c) || Span<const char>('0', '9').conatins(c)
        || (p_allows_special_chars && cAllowdSpecCharOfFileName.isIn(c)));
}

template <typename T> 
constexpr DString& CleanFileName(const Span< T* const> &path, DString &dest, bool p_allows_special_chars = true) {
    for (const char c : path)
        dest << (CheckFileNameChar(c, p_allows_special_chars) ? c : '_');
    return dest;
}

template <typename T> 
constexpr bool CheckFileName(const Span< T* const> &path, bool p_allows_special_chars = true) {
    for (const char c : path)
        if (!CheckFileNameChar(c, p_allows_special_chars))
            return false;
    return true;
}

#define _Exception(errNo, text) Exception { errNo, text, __FILE__, __LINE__, __FUNCTION__ }
struct Exception {
public:
    const long m_err_nr;
    const char* const m_text;
    const char* const m_file;
    const char* const m_function;
    const int m_line;
    constexpr Exception(long p_err_nr, const char* const p_text, const char* const p_file, const int p_line, const char* const p_function) noexcept
        : m_err_nr(p_err_nr), m_text(p_text), m_file(p_file), m_line(p_line), m_function(p_function) {}
};

constexpr DString& operator << ( DString& p_stream, const Exception &ex) noexcept {
    return p_stream << "error #" << Num<>( ex.m_err_nr) << ": '" << ex.m_text << "' in " << ex.m_function << "(), line " << Num<1>( ex.m_line) << '@' << ex.m_file << '\n';
}

#define VERSION( NAME, _v1, _v2, _v3, _v4) CONSTEXPR_IF Version version_ ## NAME = { __FILE__, _v1, _v2, _v3, _v4 }

struct Version : protected DStringContainer<200> {
    using DString::reader;
    template< u8 SIZE> constexpr
    Version(const tchar(&_sourceFilePath)[SIZE], const char _major, const char _minor, const char _patch, const char _build) {
        CleanFileName(ExtractFileName(CStringInstance(_sourceFilePath)), *this);
        *this << " v" << Num<1>(_major) << '.' << Num<1>(_minor) << '.' << Num<1>(_patch) << '.' << Num<1>(_build);
    }
};

VERSION( base_hpp, 0, 3, 0, 3);

#endif /* base_hpp */
