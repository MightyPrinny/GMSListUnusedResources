/*
Copyright (c) 2008-2016, Wojciech Muła
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef STR_STR_SSE4_H
#define STR_STR_SSE4_H

#include <string>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <immintrin.h>

#define FORCE_INLINE inline __attribute__((always_inline))
#define MAYBE_UNUSED inline __attribute__((unused))

namespace {

    MAYBE_UNUSED
    bool always_true(const char*, const char*) {
        return true;
    }

    MAYBE_UNUSED
    bool memcmp1(const char* a, const char* b) {
        return a[0] == b[0];
    }

    MAYBE_UNUSED
    bool memcmp2(const char* a, const char* b) {
        const uint16_t A = *reinterpret_cast<const uint16_t*>(a);
        const uint16_t B = *reinterpret_cast<const uint16_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp3(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp2(a, b) && memcmp1(a + 2, b + 2);
#else
        const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        return (A & 0x00ffffff) == (B & 0x00ffffff);
#endif
    }

    MAYBE_UNUSED
    bool memcmp4(const char* a, const char* b) {

        const uint32_t A = *reinterpret_cast<const uint32_t*>(a);
        const uint32_t B = *reinterpret_cast<const uint32_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp5(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp4(a, b) && memcmp1(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x000000fffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp6(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp4(a, b) && memcmp2(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x0000fffffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp7(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP 
        return memcmp4(a, b) && memcmp3(a + 4, b + 4);
#else
        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return ((A ^ B) & 0x00fffffffffffffflu) == 0;
#endif
    }

    MAYBE_UNUSED
    bool memcmp8(const char* a, const char* b) {

        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return A == B;
    }

    MAYBE_UNUSED
    bool memcmp9(const char* a, const char* b) {

        const uint64_t A = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t B = *reinterpret_cast<const uint64_t*>(b);
        return (A == B) & (a[8] == b[8]);
    }

    MAYBE_UNUSED
    bool memcmp10(const char* a, const char* b) {

        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint16_t Aw = *reinterpret_cast<const uint16_t*>(a + 8);
        const uint16_t Bw = *reinterpret_cast<const uint16_t*>(b + 8);
        return (Aq == Bq) & (Aw == Bw);
    }

    MAYBE_UNUSED
    bool memcmp11(const char* a, const char* b) {

#ifdef USE_SIMPLE_MEMCMP
        return memcmp8(a, b) && memcmp3(a + 8, b + 8);
#else
        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
        const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
        return (Aq == Bq) & ((Ad & 0x00ffffff) == (Bd & 0x00ffffff));
#endif
    }

    MAYBE_UNUSED
    bool memcmp12(const char* a, const char* b) {

        const uint64_t Aq = *reinterpret_cast<const uint64_t*>(a);
        const uint64_t Bq = *reinterpret_cast<const uint64_t*>(b);
        const uint32_t Ad = *reinterpret_cast<const uint32_t*>(a + 8);
        const uint32_t Bd = *reinterpret_cast<const uint32_t*>(b + 8);
        return (Aq == Bq) & (Ad == Bd);
    }

}

namespace bits {

    template <typename T>
    T clear_leftmost_set(const T value) {

        assert(value != 0);

        return value & (value - 1);
    }


    template <typename T>
    unsigned get_first_bit_set(const T value) {

        assert(value != 0);

        return __builtin_ctz(value);
    }


    template <>
    unsigned get_first_bit_set<uint64_t>(const uint64_t value) {

        assert(value != 0);

        return __builtin_ctzl(value);
    }

} // namespace bits

size_t FORCE_INLINE sse42_strstr_anysize(const char* s, size_t n, const char* needle, size_t k) {

    assert(k > 0);
    assert(n > 0);

    const __m128i N = _mm_loadu_si128((__m128i*)needle);

    for (size_t i = 0; i < n; i += 16) {
    
        const int mode = _SIDD_UBYTE_OPS 
                       | _SIDD_CMP_EQUAL_ORDERED
                       | _SIDD_BIT_MASK;

        const __m128i D   = _mm_loadu_si128((__m128i*)(s + i));
        const __m128i res = _mm_cmpestrm(N, k, D, n - i, mode);
        uint64_t mask = _mm_cvtsi128_si64(res);

        while (mask != 0) {

            const auto bitpos = bits::get_first_bit_set(mask);

            // we know that at least the first character of needle matches
            if (memcmp(s + i + bitpos + 1, needle + 1, k - 1) == 0) {
                return i + bitpos;
            }

            mask = bits::clear_leftmost_set(mask);
        }
    }

    return std::string::npos;
}


template <size_t k, typename MEMCMP>
size_t FORCE_INLINE sse42_strstr_memcmp(const char* s, size_t n, const char* needle, MEMCMP memcmp_fun) {

    assert(k > 0);
    assert(n > 0);

    const __m128i N = _mm_loadu_si128((__m128i*)needle);

    for (size_t i = 0; i < n; i += 16) {
    
        const int mode = _SIDD_UBYTE_OPS 
                       | _SIDD_CMP_EQUAL_ORDERED
                       | _SIDD_BIT_MASK;

        const __m128i D   = _mm_loadu_si128((__m128i*)(s + i));
        const __m128i res = _mm_cmpestrm(N, k, D, n - i, mode);
        uint64_t mask = _mm_cvtsi128_si64(res);

        while (mask != 0) {

            const auto bitpos = bits::get_first_bit_set(mask);

            if (memcmp_fun(s + i + bitpos + 1, needle + 1)) {
                return i + bitpos;
            }

            mask = bits::clear_leftmost_set(mask);
        }
    }

    return std::string::npos;
}

// ------------------------------------------------------------------------

size_t sse42_strstr(const char* s, size_t n, const char* needle, size_t k) {

    size_t result = std::string::npos;

    if (n < k) {
        return result;
    }

	switch (k) {
		case 0:
			return 0;

		case 1: {
            const char* res = reinterpret_cast<const char*>(strchr(s, needle[0]));

			return (res != nullptr) ? res - s : std::string::npos;
            }

        case 2:
            result = sse42_strstr_memcmp<2>(s, n, needle, memcmp1);
            break;

        case 3:
            result = sse42_strstr_memcmp<3>(s, n, needle, memcmp2);
            break;

        case 4:
            result = sse42_strstr_memcmp<4>(s, n, needle, memcmp3);
            break;

        case 5:
            result = sse42_strstr_memcmp<5>(s, n, needle, memcmp4);
            break;

        case 6:
            result = sse42_strstr_memcmp<6>(s, n, needle, memcmp5);
            break;

        case 7:
            result = sse42_strstr_memcmp<7>(s, n, needle, memcmp6);
            break;

        case 8:
            result = sse42_strstr_memcmp<8>(s, n, needle, memcmp7);
            break;

        case 9:
            result = sse42_strstr_memcmp<9>(s, n, needle, memcmp8);
            break;

        case 10:
            result = sse42_strstr_memcmp<10>(s, n, needle, memcmp9);
            break;

        case 11:
            result = sse42_strstr_memcmp<11>(s, n, needle, memcmp10);
            break;

        case 12:
            result = sse42_strstr_memcmp<12>(s, n, needle, memcmp11);
            break;

		default:
			result = sse42_strstr_anysize(s, n, needle, k);
            break;
    }

    if (result <= n - k) {
        return result;
    } else {
        return std::string::npos;
    }
}

#endif