#ifndef MY_RANDOM_INCLUDED
#define MY_RANDOM_INCLUDED

/*
   Copyright (c) 2012, 2025, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is designed to work with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have either included with
   the program or referenced in the documentation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file include/my_rnd.h
  A wrapper to use OpenSSL PRNGs.
*/

#include <stddef.h>
#include <cstdint>
#if defined(__GNUC__) && defined(__x86_64__) || defined(_WIN32)
#include <nmmintrin.h>
#include <wmmintrin.h>
#elif defined(__GNUC__) && defined(__aarch64__) && !defined(APPLE_ARM)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

#include "my_compiler.h"
#include "my_rdtsc.h"

double my_rnd_ssl(bool *failed);
int my_rand_buffer(unsigned char *buffer, size_t buffer_size);

#if defined(__GNUC__) && defined(__x86_64__) || defined(_WIN32)
MY_ATTRIBUTE((target("sse4.2")))
static inline uint64_t my_crc32_uint64(uint64_t crc, uint64_t data) {
  return _mm_crc32_u64(crc, data);
}
#elif defined(__GNUC__) && defined(__aarch64__)
#if !defined(APPLE_ARM)
MY_ATTRIBUTE((target("+crc")))
#endif
static inline uint64_t my_crc32_uint64(uint64_t crc, uint64_t data) {
  return (uint64_t)__crc32cd((uint32_t)crc, data);
}
#endif

/**
    Returns random value in double type between 0.0 to 1.0.
*/
static inline double my_rnd_double() {
  uint64_t value = my_timer_cycles();
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__aarch64__)) || \
    defined(_WIN32)
  /* Same with crc32_hash_uint64() of InnoDB */
  value *= 0xb5eb6fbadd39bf9b;
  value = (my_crc32_uint64(0, value) ^ value) << 32 |
          my_crc32_uint64(0, (value >> 32 | value << 32));
  return (double)value / ((double)0x8000000000000000u * 2.0);
#else
  /* Same with hash_uint32_pair_ib() of InnoDB (worse hash and granularity) */
  const uint32_t n1 = (uint32_t)(value & 0xFFFFFFFF);
  const uint32_t n2 = (uint32_t)(value >> 32);
  constexpr uint32_t HASH_RANDOM_MASK = 1463735687;
  constexpr uint32_t HASH_RANDOM_MASK2 = 1653893711;
  value = ((((n1 ^ n2 ^ HASH_RANDOM_MASK2) << 8) + n1) ^ HASH_RANDOM_MASK) + n2;
  value &= 0xFFFFFFFF;
  return (double)value / (double)0x0000000100000000u;
#endif
}

#endif /* MY_RANDOM_INCLUDED */
