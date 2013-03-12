/*
 * rtp: RTP support for Pd
 *
 * (c) 2013 IOhannes m zmölnig, institute of electronic music and acoustics (iem)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "m_pd.h"

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || \
    defined(ANDROID)
#include <endian.h>
#endif
#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BIG_ENDIAN    0x0002
#define BYTE_ORDER    LITTLE_ENDIAN
#endif
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)
#error No byte order defined
#endif

#include <stdlib.h>

#if 1
# define STATIC_INLINE static inline
#else
# define STATIC_INLINE
#endif

/* integer type handling */
typedef unsigned char  u_int8;
typedef unsigned short u_int16;
typedef unsigned int   u_int32;

typedef union {
  u_int32 i;
  u_int16 s[2];
  u_int8  b[4];
} t_uint32bytes;


/**
 * @brief convert 32bit unsigned integer into a list of Pd
 *        (using two 16bit unsigned integers)
 * @param ap pointer to atom-array to store the u_int32 into
 * @param i  the u_int32 value
 * @return number of atoms consumed (2)
 */
STATIC_INLINE u_int32 SETUINT32(t_atom ap[2], u_int32 i) {
  SETFLOAT(ap+0, ((i>>16)&0xFFFF));
  SETFLOAT(ap+1, ((i>> 0)&0xFFFF));
  return 2;
}
/**
 * @brief read 16bit unsigned integer from byte array (BIG-ENDIAN)
 * @param ap pointer to atom-array to read u_int16 from
 * @return the u_int16
 */
STATIC_INLINE u_int16 atombytes_getU16(t_atom ap[2]) {
  u_int16 result=0;
  result<<= 8;result+=atom_getint(ap++);
  result<<= 8;result+=atom_getint(ap++);
  return result;
}
/**
 * @brief read 32bit unsigned integer from byte array (BIG-ENDIAN)
 * @param ap pointer to atom-array to read u_int32 from
 * @return the u_int32
 */
STATIC_INLINE u_int32 atombytes_getU32(t_atom ap[4]) {
  u_int32 result=0;
  result<<= 8;result+=atom_getint(ap++);
  result<<= 8;result+=atom_getint(ap++);
  result<<= 8;result+=atom_getint(ap++);
  result<<= 8;result+=atom_getint(ap++);
  return result;
}

/**
 * @brief convert a 32bit unsigned integer into a bytearray (in atoms, BIG_ENDIAN)
 * @param i the 32bit unsigned integer
 * @param ap pointer to atom-array to write bytes to; the atoms have to be initialized to float-atoms beforehand (e.g. using SETFLOAT)
 * @return the number of atoms written (4)
 */
STATIC_INLINE int atombytes_setU32(u_int32 i, t_atom ap[4]) {
  ap++->a_w.w_float=(i>>24) & 0xFF;
  ap++->a_w.w_float=(i>>16) & 0xFF;
  ap++->a_w.w_float=(i>> 8) & 0xFF;
  ap++->a_w.w_float=(i>> 0) & 0xFF;
  return 4;
}
/**
 * @brief convert a 16bit unsigned integer into a bytearray (in atoms, BIG_ENDIAN)
 * @param i the 16bit unsigned integer
 * @param ap pointer to atom-array to write bytes to; the atoms have to be initialized to float-atoms beforehand (e.g. using SETFLOAT)
 * @return the number of atoms written (2)
 */
STATIC_INLINE int atombytes_setU16(u_int16 i, t_atom ap[2]) {
  ap++->a_w.w_float=(i>> 8) & 0xFF;
  ap++->a_w.w_float=(i>> 0) & 0xFF;
  return 2;
}


#include "rtp.h"
#include "rtcp.h"
