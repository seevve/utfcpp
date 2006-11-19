// Copyright 2006 Nemanja Trifunovic

/*
Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


#ifndef UTF8_FOR_CPP_2675DCD0_9480_4c0c_B92A_CC14C027B731
#define UTF8_FOR_CPP_2675DCD0_9480_4c0c_B92A_CC14C027B731

#include <iterator>
#include <exception>

namespace utf8
{
    // The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
    // You may need to change them to match your system. 
    // These typedefs have the same names as ones from cstdint, or boost/cstdint
    typedef unsigned char   uint8_t;
    typedef unsigned short  uint16_t;
    typedef unsigned int    uint32_t;
    
    // Exceptions that may be thrown from the library functions.
    class invalid_code_point : public std::exception {
        uint32_t cp;
    public:
        invalid_code_point(uint32_t cp) : cp(cp) {}
        virtual const char* what() const throw() { return "Invalid code point"; }
        uint32_t code_point() const {return cp;}
    };

    class invalid_utf8 : public std::exception {
        uint8_t u8;
    public:
        invalid_utf8 (uint8_t u) : u8(u) {}
        virtual const char* what() const throw() { return "Invalid UTF-8"; }
        uint8_t utf8_octet() const {return u8;}
    };

    class invalid_utf16 : public std::exception {
        uint16_t u16;
    public:
        invalid_utf16 (uint16_t u) : u16(u) {}
        virtual const char* what() const throw() { return "Invalid UTF-16"; }
        uint16_t utf16_word() const {return u16;}
    };

    class not_enough_room : public std::exception {
    public:
        virtual const char* what() const throw() { return "Not enough space"; }
    };



// Helper code - not intended to be directly called by the library users. May be changed at any time
namespace internal
{    
    // Unicode constants
    // Leading (high) surrogates: 0xd800 - 0xdbff
    // Trailing (low) surrogates: 0xdc00 - 0xdfff
    const uint16_t LEAD_SURROGATE_MIN  = 0xd800u;
    const uint16_t LEAD_SURROGATE_MAX  = 0xdbffu;
    const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
    const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
    const uint16_t LEAD_OFFSET         = LEAD_SURROGATE_MIN - (0x10000 >> 10);
    const uint32_t SURROGATE_OFFSET    = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

    // Maximum valid value for a Unicode code point
    const uint32_t CODE_POINT_MAX      = 0x0010ffffu;

    template<typename octet_type>
    inline uint8_t mask8(octet_type oc)
    {
        return static_cast<uint8_t>(0xff & oc);
    }
    template<typename u16_type>
    inline uint16_t mask16(u16_type oc)
    {
        return static_cast<uint16_t>(0xffff & oc);
    }
    template<typename octet_type>
    inline bool is_trail(octet_type oc)
    {
        return ((mask8(oc) >> 6) == 0x2);
    }

    template <typename u16>
    inline bool is_surrogate(u16 cp)
    {
        return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
    }

    template <typename u32>
    inline bool is_code_point_valid(u32 cp)
    {
        return (cp <= CODE_POINT_MAX && !is_surrogate(cp) && cp != 0xfffe && cp != 0xffff);
    }  

    template <typename octet_iterator>
    inline typename std::iterator_traits<octet_iterator>::difference_type
    sequence_length(octet_iterator lead_it)
    {
        uint8_t lead = mask8(*lead_it);
        if (lead < 0x80) 
            return 1;
        else if ((lead >> 5) == 0x6)
            return 2;
        else if ((lead >> 4) == 0xe)
            return 3;
        else if ((lead >> 3) == 0x1e)
            return 4;
        else 
            return 0;
    }

    enum utf_error {OK, NOT_ENOUGH_ROOM, INVALID_LEAD, INCOMPLETE_SEQUENCE, OVERLONG_SEQUENCE, INVALID_CODE_POINT};

    template <typename octet_iterator>
    utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t* code_point = 0)
    {
        uint32_t cp = mask8(*it);
        // Check the lead octet
        typedef typename std::iterator_traits<octet_iterator>::difference_type octet_differece_type;
        octet_differece_type length = sequence_length(it);

        // "Shortcut" for ASCII characters
        if (length == 1) {
            if (end - it > 0) {
                if (code_point)
                    *code_point = cp;
                ++it;
                return OK;
            }
            else
                return NOT_ENOUGH_ROOM;
        }

        // Do we have enough memory?     
        if (end - it < length)
            return NOT_ENOUGH_ROOM;
        
        // Check trail octets and calculate the code point
        switch (length) {
            case 0:
                return INVALID_LEAD;
                break;
            case 2:
                if (is_trail(*(++it))) { 
                    cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
                }
                else {
                    --it;
                    return INCOMPLETE_SEQUENCE;
                }
            break;
            case 3:
                if (is_trail(*(++it))) {
                    cp = ((cp << 12) & 0xffff) + ((mask8(*it) << 6) & 0xfff);
                    if (is_trail(*(++it))) {
                        cp += (*it) & 0x3f;
                    }
                    else {
                        --it; --it; 
                        return INCOMPLETE_SEQUENCE;
                    }
                }
                else {
                    --it;
                    return INCOMPLETE_SEQUENCE;
                }
            break;
            case 4:
                if (is_trail(*(++it))) {
                    cp = ((cp << 18) & 0x1fffff) + ((mask8(*it) << 12) & 0x3ffff);                
                    if (is_trail(*(++it))) {
                        cp += (mask8(*it) << 6) & 0xfff;
                        if (is_trail(*(++it))) {
                            cp += (*it) & 0x3f; 
                        }
                        else {
                            --it; --it; --it;
                            return INCOMPLETE_SEQUENCE;
                        }
                    }
                    else {
                        --it; --it;
                        return INCOMPLETE_SEQUENCE;
                    }
                }
                else {
                    --it;
                    return INCOMPLETE_SEQUENCE;
                }
            break;
        }
        // Is the code point valid?
        if (!is_code_point_valid(cp)) {
            for (octet_differece_type i = 0; i < length - 1; ++i) 
                --it;
            return INVALID_CODE_POINT;
        }
            
        if (code_point)
            *code_point = cp;
            
        if (cp < 0x80) {
            if (length != 1) {
                for (octet_differece_type i = 0; i < length - 1; ++i)
                    --it;
                return OVERLONG_SEQUENCE;
            }
        }
        else if (cp < 0x800) {
            if (length != 2) {
                for (octet_differece_type i = 0; i < length - 1; ++i)
                    --it;
                return OVERLONG_SEQUENCE;
            }
        }
        else if (cp < 0x10000) {
            if (length != 3) {
                for (octet_differece_type i = 0; i < length - 1; ++i)
                    --it;
                return OVERLONG_SEQUENCE;
            }
        }
           
        ++it;
        return OK;    
    }

} // namespace internal 
    
    /// The library API - functions intended to be called by the users
 
    // Byte order mark
    const uint8_t bom[] = {0xef, 0xbb, 0xbf}; 

    template <typename octet_iterator>
    octet_iterator find_invalid(octet_iterator start, octet_iterator end)
    {
        octet_iterator result = start;
        while (result != end) {
            internal::utf_error err_code = internal::validate_next(result, end);
            if (err_code != internal::OK)
                return result;
        }
        return result;
    }

    template <typename octet_iterator>
    bool is_valid(octet_iterator start, octet_iterator end)
    {
        return (find_invalid(start, end) == end);
    }

    template <typename octet_iterator>
    bool is_bom (octet_iterator it)
    {
        return (
            (internal::mask8(*it++)) == bom[0] &&
            (internal::mask8(*it++)) == bom[1] &&
            (internal::mask8(*it))   == bom[2]
           );
    }
    template <typename octet_iterator>
    octet_iterator append(uint32_t cp, octet_iterator result)
    {
        if (!internal::is_code_point_valid(cp)) 
            throw invalid_code_point(cp);

        if (cp < 0x80)                        // one octet
            *(result++) = static_cast<uint8_t>(cp);  
        else if (cp < 0x800) {                // two octets
            *(result++) = static_cast<uint8_t>((cp >> 6)            | 0xc0);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        else if (cp < 0x10000) {              // three octets
            *(result++) = static_cast<uint8_t>((cp >> 12)           | 0xe0);
            *(result++) = static_cast<uint8_t>((cp >> 6) & 0x3f     | 0x80);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        else if (cp <= internal::CODE_POINT_MAX) {      // four octets
            *(result++) = static_cast<uint8_t>((cp >> 18)           | 0xf0);
            *(result++) = static_cast<uint8_t>((cp >> 12)& 0x3f     | 0x80);
            *(result++) = static_cast<uint8_t>((cp >> 6) & 0x3f     | 0x80);
            *(result++) = static_cast<uint8_t>((cp & 0x3f)          | 0x80);
        }
        else
            throw invalid_code_point(cp);

        return result;
    }

    template <typename octet_iterator>
    uint32_t next(octet_iterator& it, octet_iterator end)
    {
        uint32_t cp = 0;
        internal::utf_error err_code = internal::validate_next(it, end, &cp);
        switch (err_code) {
            case internal::OK :
                break;
            case internal::NOT_ENOUGH_ROOM :
                throw not_enough_room();
            case internal::INVALID_LEAD :
            case internal::INCOMPLETE_SEQUENCE :
            case internal::OVERLONG_SEQUENCE :
                throw invalid_utf8(*it);
            case internal::INVALID_CODE_POINT :
                throw invalid_code_point(cp);
        }
        return cp;        
    }

    template <typename octet_iterator>
    uint32_t prior(octet_iterator& it, octet_iterator start)
    {
        octet_iterator end = it;
        while (internal::is_trail(*(--it))) 
            if (it < start)
                throw invalid_utf8(*it); // error - no lead byte in the sequence
        octet_iterator temp = it;
        return next(temp, end);
    }

    /// Deprecated in versions that include "prior"
    template <typename octet_iterator>
    uint32_t previous(octet_iterator& it, octet_iterator pass_start)
    {
        octet_iterator end = it;
        while (internal::is_trail(*(--it))) 
            if (it == pass_start)
                throw invalid_utf8(*it); // error - no lead byte in the sequence
        octet_iterator temp = it;
        return next(temp, end);
    }

    template <typename octet_iterator, typename distance_type>
    void advance (octet_iterator& it, distance_type n, octet_iterator end)
    {
        for (distance_type i = 0; i < n; ++i)
            next(it, end);
    }

    template <typename octet_iterator>
    typename std::iterator_traits<octet_iterator>::difference_type
    distance (octet_iterator first, octet_iterator last)
    {
        typename std::iterator_traits<octet_iterator>::difference_type dist;
        for (dist = 0; first < last; ++dist) 
            next(first, last);
        return dist;
    }

    template <typename u16bit_iterator, typename octet_iterator>
    octet_iterator utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
    {       
        while (start != end) {
            uint32_t cp = internal::mask16(*start++);
            // Take care of surrogate pairs first
            if (internal::is_surrogate(cp)) {
                if (start != end) {
                    uint32_t trail_surrogate = internal::mask16(*start++);
                    if (trail_surrogate >= internal::TRAIL_SURROGATE_MIN && trail_surrogate <= internal::TRAIL_SURROGATE_MAX)
                        cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;                    
                    else 
                        throw invalid_utf16(static_cast<uint16_t>(trail_surrogate));
                }
                else 
                    throw invalid_utf16(static_cast<uint16_t>(*start));
            
            }
            result = append(cp, result);
        }
        return result;        
    }

    template <typename u16bit_iterator, typename octet_iterator>
    u16bit_iterator utf8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result)
    {
        while (start != end) {
            uint32_t cp = next(start, end);
            if (cp > 0xffff) { //make a surrogate pair
                *result++ = static_cast<uint16_t>((cp >> 10)   + internal::LEAD_OFFSET);
                *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
            }
            else
                *result++ = static_cast<uint16_t>(cp);
        }
        return result;
    }

    template <typename octet_iterator, typename u32bit_iterator>
    octet_iterator utf32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
    {
        while (start != end)
            result = append(*(start++), result);

        return result;
    }

    template <typename octet_iterator, typename u32bit_iterator>
    u32bit_iterator utf8to32 (octet_iterator start, octet_iterator end, u32bit_iterator result)
    {
        while (start < end)
            (*result++) = next(start, end);

        return result;
    }

    namespace unchecked 
    {
        template <typename octet_iterator>
        octet_iterator append(uint32_t cp, octet_iterator result)
        {
            if (cp < 0x80)                        // one octet
                *(result++) = static_cast<uint8_t>(cp);  
            else if (cp < 0x800) {                // two octets
                *(result++) = static_cast<uint8_t>((cp >> 6)          | 0xc0);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            else if (cp < 0x10000) {              // three octets
                *(result++) = static_cast<uint8_t>((cp >> 12)         | 0xe0);
                *(result++) = static_cast<uint8_t>((cp >> 6) & 0x3f   | 0x80);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            else {                                // four octets
                *(result++) = static_cast<uint8_t>((cp >> 18)         | 0xf0);
                *(result++) = static_cast<uint8_t>((cp >> 12)& 0x3f   | 0x80);
                *(result++) = static_cast<uint8_t>((cp >> 6) & 0x3f   | 0x80);
                *(result++) = static_cast<uint8_t>((cp & 0x3f)        | 0x80);
            }
            return result;
        }
        template <typename octet_iterator>
        uint32_t next(octet_iterator& it)
        {
            uint32_t cp = internal::mask8(*it);
            typename std::iterator_traits<octet_iterator>::difference_type length = utf8::internal::sequence_length(it);
            switch (length) {
                case 1:
                    break;
                case 2:
                    it++;
                    cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
                    break;
                case 3:
                    ++it; 
                    cp = ((cp << 12) & 0xffff) + ((internal::mask8(*it) << 6) & 0xfff);
                    ++it;
                    cp += (*it) & 0x3f;
                    break;
                case 4:
                    ++it;
                    cp = ((cp << 18) & 0x1fffff) + ((internal::mask8(*it) << 12) & 0x3ffff);                
                    ++it;
                    cp += (internal::mask8(*it) << 6) & 0xfff;
                    ++it;
                    cp += (*it) & 0x3f; 
                    break;
            }
            ++it;
            return cp;        
        }

        template <typename octet_iterator>
        uint32_t previous(octet_iterator& it)
        {
            while (internal::is_trail(*(--it))) ;
            octet_iterator temp = it;
            return next(temp);
        }

        template <typename octet_iterator, typename distance_type>
        void advance (octet_iterator& it, distance_type n)
        {
            for (distance_type i = 0; i < n; ++i)
                next(it);
        }

        template <typename octet_iterator>
        typename std::iterator_traits<octet_iterator>::difference_type
        distance (octet_iterator first, octet_iterator last)
        {
            typename std::iterator_traits<octet_iterator>::difference_type dist;
            for (dist = 0; first < last; ++dist) 
                next(first);
            return dist;
        }

        template <typename u16bit_iterator, typename octet_iterator>
        octet_iterator utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
        {       
            while (start != end) {
                uint32_t cp = internal::mask16(*start++);
            // Take care of surrogate pairs first
                if (internal::is_surrogate(cp)) {
                    uint32_t trail_surrogate = internal::mask16(*start++);
                    cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
                }
                result = append(cp, result);
            }
            return result;         
        }

        template <typename u16bit_iterator, typename octet_iterator>
        u16bit_iterator utf8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result)
        {
            while (start != end) {
                uint32_t cp = next(start);
                if (cp > 0xffff) { //make a surrogate pair
                    *result++ = static_cast<uint16_t>((cp >> 10)   + internal::LEAD_OFFSET);
                    *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
                }
                else
                    *result++ = static_cast<uint16_t>(cp);
            }
            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        octet_iterator utf32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
        {
            while (start != end)
                result = append(*(start++), result);

            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        u32bit_iterator utf8to32 (octet_iterator start, octet_iterator end, u32bit_iterator result)
        {
            while (start < end)
                (*result++) = next(start);

            return result;
        }

    } // namespace utf8::unchecked
} // namespace utf8 

#endif // header guard
