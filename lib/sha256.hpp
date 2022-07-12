/****************************************************************************************************************************
 * Copyleft (c) 2022 by Marco Gerodetti                                                                                     *
 *                                                                                                                          *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public        *
 * License as published by the Free Software Foundation, version 2. This program is distributed WITHOUT ANY WARRANTY;       *
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public     *
 * License for more details <http://www.gnu.org/licenses/gpl-2.0.html>.                                                     *
 *                                                                                                                          *
 *   written in simple C++, POSIX compatibility for Environmental requirements, alternativ for windows to                   *
 *   using c++ ISO/IEC 14882:2011, Reference: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf                    *
 *                                                                                                                          *
 ****************************************************************************************************************************///

#ifndef sha256_h
#define sha256_h

#include "base.hpp"

VERSION(sha256_hpp, 0, 2, 0, 3);

class Sha256 {
private:
    bool finished = false;
    u8  buffer_filled = 0;
    u64 contendlen = 0;
    u32 hs32[8] = { 0 };
    static constexpr u8 SHA256_BLOCK_SIZE = 32;             // SHA256 outputs a 32 byte digest
    static constexpr u8 bufferSizeAs32b = 64;
    static constexpr u8 size_payload_buffer_as32bit = 16;
public:
    static constexpr u8 size_payload_buffer_as08bit = 4 * size_payload_buffer_as32bit;
private:
    BytesOfScalar< u32, Endianes::Big> payload_buffer_as32bit[ bufferSizeAs32b] = { 0 };
    DArrayContainer< u8, SHA256_BLOCK_SIZE> m_hash;

    typedef const u32 u32c;

    constexpr static u32c hs32init[]{ 0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u };

    constexpr static u32c k[] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u, 0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u, 0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu, 0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u, 0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u, 0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u, 0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u, 0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u, 0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };

    static constexpr u32c rot_r(const u32c a, const u32c b)               noexcept { return (a >> b) | (a << (32 - b)); }
    static constexpr u32c ch(   const u32c x, const u32c y, const u32c z) noexcept { return (x &  y) ^ (~x &  z); }
    static constexpr u32c maj(  const u32c x, const u32c y, const u32c z) noexcept { return (x &  y) ^ (x &  z) ^ (y & z); }
    static constexpr u32c ep0(  const u32c x)                             noexcept { return (rot_r(x, 2) ^ rot_r(x, 13) ^ rot_r(x, 22)); }
    static constexpr u32c ep1(  const u32c x)                             noexcept { return (rot_r(x, 6) ^ rot_r(x, 11) ^ rot_r(x, 25)); }
    static constexpr u32c sig0( const u32c x)                             noexcept { return (rot_r(x, 7) ^ rot_r(x, 18) ^ ((x) >> 3)); }
    static constexpr u32c sig1( const u32c x)                             noexcept { return (rot_r(x, 17) ^ rot_r(x, 19) ^ ((x) >> 10)); }

    constexpr void
        process_buffer() noexcept {
        // 6.2.2 SHA-256 Hash Computation
        // buffer32b[ 0..15] is already filled by payload 8bit[ 0..63]
        // prepare [16..63]
        for (u8 i = size_payload_buffer_as32bit; i < bufferSizeAs32b; i++)
            payload_buffer_as32bit[i].m_v = sig1(payload_buffer_as32bit[i - 2].m_v)
            + payload_buffer_as32bit[i - 7].m_v
            + sig0(payload_buffer_as32bit[i - 15].m_v)
            + payload_buffer_as32bit[i - 16].m_v;

        u32 a = hs32[0], b = hs32[1], c = hs32[2], d = hs32[3], e = hs32[4], f = hs32[5], g = hs32[6], h = hs32[7];

        for (u8 i = 0; i < bufferSizeAs32b; ++i) {
            const u32 t1 = h + ep1(e) + ch(e, f, g) + k[i] + payload_buffer_as32bit[i].m_v;
            const u32 t2 = ep0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        hs32[0] += a; hs32[1] += b; hs32[2] += c; hs32[3] += d; hs32[4] += e; hs32[5] += f; hs32[6] += g; hs32[7] += h;
        buffer_filled = 0;
    }

    //5.1.2 Padding the Message
    constexpr inline void padding_the_message_except_space(const u8 up_to_space) {
        i8 to_fill = size_payload_buffer_as08bit - buffer_filled - up_to_space;
        if (to_fill >= 0) //ready to pad?
            while (to_fill--)
                add_byte(0); //pad
        else {  // not enough space to pad? => add block
            padding_the_message_except_space(0); // will fill buffer up, triggers processing block and reset space
            padding_the_message_except_space(up_to_space); //Now, the padding will succeed
        }
    }
    
    constexpr void finalize() {
        if (!finished) {
            //6.2.1 SHA-256 Preprocessing 2.
            const u64 conten_bit_len = payloadLen() * 8;
            add_byte(0x80);
            //5.1.2 Padding the Message
            padding_the_message_except_space(sizeof(conten_bit_len));

            for (auto b : ByteArrayOfScalar< u64, Endianes::Big>(conten_bit_len))
                add_byte(b); // will reaching full buffer and get processed

            m_hash.reset();
            for (const u32 st : hs32)
                m_hash << ByteArrayOfScalar< u32, Endianes::Big>(st);
            finished = true;
        }
    }

public:
    constexpr Sha256()  noexcept { reset(); }
 
    constexpr const u64 payloadLen() const { return contendlen + buffer_filled; }
    
    constexpr Sha256&
    reset() noexcept {
        finished = false;
        contendlen = 0;
        buffer_filled = 0;
        m_hash.reset();
        //6.2.1, 5.3.3 SHA-256 Preprocessing 1.
        cpyArray( hs32init, hs32);
        return *this;
    }

    constexpr inline void
    add_byte(const u8 b) {
        if (finished)
            reset();
        //6.2.2 SHA-256 Hash Computation => 1. Prepare the message schedule
        payload_buffer_as32bit[buffer_filled / 4].setByte(b, buffer_filled % 4);
        if (++buffer_filled < size_payload_buffer_as08bit)
            return;
        //6.2.2 SHA-256 Hash Computation, if block is full
        process_buffer();
        contendlen += size_payload_buffer_as08bit;
    }

    constexpr inline void
    add_block(const ArraySpan<u8> &p_a_08b) {
        if ( finished)
            reset();
        //6.2.2 SHA-256 Hash Computation => 1. Prepare the message schedule
        // check fast block condition
        if ( buffer_filled == 0 && p_a_08b.count() == size_payload_buffer_as08bit)
        {
            for (BytesOfScalar< u32, Endianes::Big> &a_32b : payload_buffer_as32bit) {
                a_32b.setByte( p_a_08b[ buffer_filled++], 0);
                a_32b.setByte( p_a_08b[ buffer_filled++], 1);
                a_32b.setByte( p_a_08b[ buffer_filled++], 2);
                a_32b.setByte( p_a_08b[ buffer_filled++], 3);
            }
            //6.2.2 SHA-256 Hash Computation, if block is full
            process_buffer();
            contendlen += size_payload_buffer_as08bit;
        }
        else { // slow procedure, if it is not in block condition
            for (const u8 b08b : p_a_08b)
                add_byte( b08b);
        }
    }

    constexpr ArraySpan<u8> hash() {
        finalize();
        return m_hash.reader();
    }

    constexpr inline Sha256& operator << (const u8 c)       noexcept {
        add_byte(c);
        return *this;
    }
};

constexpr DArray<char>& operator << (DArray<char>& p_s, Sha256 &m_sha) noexcept {
    return serialize<u8, 16, '\0'>( p_s,  m_sha.hash());
}

template <typename T> constexpr
Sha256& operator << ( Sha256& m_sha, ArraySpan<T> contend) noexcept {
    for (const auto c : contend)
        m_sha << c;
    return m_sha;
}


#endif /* sha256_h */
