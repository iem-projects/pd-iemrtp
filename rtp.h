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
static inline u_int32 SETUINT32(t_atom ap[2], u_int32 i) {
  SETFLOAT(ap+0, ((i>>16)&0xFFFF));
  SETFLOAT(ap+1, ((i>> 0)&0xFFFF));
  return 2;
}
/**
 * @brief read 16bit unsigned integer from byte array (BIG-ENDIAN)
 * @param ap pointer to atom-array to read u_int16 from
 * @return the u_int16
 */
static inline u_int16 atombytes_getU16(t_atom ap[2]) {
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
static inline u_int32 atombytes_getU32(t_atom ap[4]) {
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
static inline int atombytes_setU32(u_int32 i, t_atom ap[4]) {
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
static inline int atombytes_setU16(u_int16 i, t_atom ap[2]) {
  ap++->a_w.w_float=(i>> 8) & 0xFF;
  ap++->a_w.w_float=(i>> 0) & 0xFF;
  return 2;
}
static inline u_int32 uint32bytes2atoms(u_int32 ival, t_atom*ap) {
  u_int32 i=0;
  t_uint32bytes v;
  v.i=ival;
#if BYTE_ORDER == LITTLE_ENDIAN
   ap[i++].a_w.w_float=v.b[3];
   ap[i++].a_w.w_float=v.b[2];
   ap[i++].a_w.w_float=v.b[1];
   ap[i++].a_w.w_float=v.b[0];
#else
   ap[i++].a_w.w_float=v.b[0];
   ap[i++].a_w.w_float=v.b[1];
   ap[i++].a_w.w_float=v.b[2];
   ap[i++].a_w.w_float=v.b[3];
#endif
  return sizeof(v);
}


#define RTP_HEADERSIZE 13
#define RTP_BYTESPERSAMPLE 2
#define EMPTYPACKETBYTES 100

typedef struct _rtpheader {
#if BYTE_ORDER == LITTLE_ENDIAN
  /* byte#2 */
  unsigned int pt:7;        /* payload type */
  unsigned int m:1;         /* marker bit */
  /* byte#1 */
  unsigned int cc:4;        /* CSRC count */
  unsigned int x:1;         /* header extension flag */
  unsigned int p:1;         /* padding flag */
  unsigned int version:2;   /* protocol version */
#else
  /* byte#1 */
  unsigned int version:2;   /* protocol version */
  unsigned int p:1;         /* padding flag */
  unsigned int x:1;         /* header extension flag */
  unsigned int cc:4;        /* CSRC count */
  /* byte#2 */
  unsigned int m:1;         /* marker bit */
  unsigned int pt:7;        /* payload type */
#endif
  /* byte#3-4 */
  unsigned int seq:16;      /* sequence number */
  /* byte#5-8 */
  u_int32 ts;               /* timestamp */
  /* byte#9-12 */
  u_int32 ssrc;             /* synchronization source */
  /* optional byte#13...(12+pt*4) */
  u_int32*csrc;             /* optional CSRC list */
} t_rtpheader;

static inline u_int32 header2atoms(t_rtpheader*rtpheader, t_atom*ap0) {
  t_atom*ap=ap0;
  u_int8 b;
  u_int32 c, cc=rtpheader->cc;
  b=(rtpheader->version << 6) | (rtpheader->p << 5) | (rtpheader->x << 4) | (rtpheader->cc << 0);
  ap++->a_w.w_float=b;

  b=(rtpheader->m << 7) | (rtpheader->pt << 0);
  ap++->a_w.w_float=b;

  ap+=atombytes_setU16(rtpheader->seq, ap);
  ap+=atombytes_setU32(rtpheader->ts, ap);
  ap+=atombytes_setU32(rtpheader->ssrc, ap);
  for(c=0; c < cc; c++) {
    ap+=atombytes_setU32(rtpheader->csrc[c], ap);
  }
  return (12+cc*4);
}
/**
 * @brief parse a byte-package (atom list) to an rtpheader.
 * @param argc total length of the list
 * @param argv array of bytes (as atoms)
 * @param rtpheader pointer to initialized RTP-header, that is used as output.
 * @note any CSRC array in the rtpheader will be deleted and replaced by CSRC fields found in the byte-package
 * @return the number of bytes consumed by the header
 *         on error, 0 or a negative number (minimum expected packet size ) is returned
 */
static inline int atoms2header(int argc, t_atom*argv, t_rtpheader*rtpheader) {
  u_int8  b;
  u_int8 cc;
  int retval=12;

  if(!argc) {
    return -retval;
  }

  b=atom_getint(argv+0);
  cc=(b >> 0) & 0x0F;
  retval=12+cc*4;
  if(argc<retval) {
    return -retval;
  }
  if(rtpheader->csrc)free(rtpheader->csrc);
  rtpheader->csrc=malloc(cc * sizeof(u_int32));

  rtpheader->version = (b >> 6) & 0x03;
  rtpheader->p       = (b >> 5) & 0x01;
  rtpheader->x       = (b >> 4) & 0x01;
  rtpheader->cc      = cc;

  b=atom_getint(argv+1);
  rtpheader->m       = (b >> 7) & 0x01;
  rtpheader->pt      = (b >> 0) & 0x7F;

  rtpheader->seq  =atombytes_getU16(argv+2);
  rtpheader->ts   =atombytes_getU32(argv+4);
  rtpheader->ssrc =atombytes_getU32(argv+8);

  for(b=0; b<cc; b++) {
    rtpheader->csrc[b]=atombytes_getU32(argv+12+4*b);
  }
  return retval;
}
