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

#define RTP_HEADERSIZE 13
#define RTP_BYTESPERSAMPLE 2
#define EMPTYPACKETBYTES 100

typedef unsigned char u_int8;
typedef unsigned int u_int32;

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

typedef union {
  u_int32 i;
  u_int8  b[4];
} t_uint32bytes;

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
static inline u_int32 header2atoms(t_rtpheader*rtpheader, t_atom*ap0) {
  u_int8*bytes=(u_int8*)rtpheader;
  t_atom*ap=ap0;
  u_int32 i=0, j;
#if BYTE_ORDER == LITTLE_ENDIAN
  ap++->a_w.w_float=bytes[1];
  ap++->a_w.w_float=bytes[0];
  ap++->a_w.w_float=bytes[3];
  ap++->a_w.w_float=bytes[2];
#else
  ap++->a_w.w_float=bytes[0];
  ap++->a_w.w_float=bytes[1];
  ap++->a_w.w_float=bytes[2];
  ap++->a_w.w_float=bytes[3];
#endif
  i=4;
  ap+=uint32bytes2atoms(rtpheader->ts, ap); i+=4;
  ap+=uint32bytes2atoms(rtpheader->ssrc, ap); i+=4;

  for(j=0; j<rtpheader->cc; j++) {
    ap+=uint32bytes2atoms(rtpheader->csrc[j], ap); i+=4;

  }

  return i;
}



