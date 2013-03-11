/*
 * rtcp.h: RTP support for Pd (RTCP components)
 *
 * (c) 2013 IOhannes m zm�lnig, institute of electronic music and acoustics (iem)
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
#define RTP_MAX_SDES 255      /* maximum text length for SDES */

typedef enum {
  RTCP_SR   = 200,
  RTCP_RR   = 201,
  RTCP_SDES = 202,
  RTCP_BYE  = 203,
  RTCP_APP  = 204
} rtcp_type_t;

typedef enum {
  RTCP_SDES_END   = 0,
  RTCP_SDES_CNAME = 1,
  RTCP_SDES_NAME  = 2,
  RTCP_SDES_EMAIL = 3,
  RTCP_SDES_PHONE = 4,
  RTCP_SDES_LOC   = 5,
  RTCP_SDES_TOOL  = 6,
  RTCP_SDES_NOTE  = 7,
  RTCP_SDES_PRIV  = 8
} rtcp_sdes_type_t;

/*
 * RTCP common header word
 */
typedef struct {
  unsigned int version:2;   /* protocol version */
  unsigned int p:1;         /* padding flag */
  unsigned int count:5;     /* varies by packet type */
  unsigned int pt:8;        /* RTCP packet type */
  u_int16 length;           /* pkt len in words, w/o this word */
} rtcp_common_t;
/*
 * Reception report block
 */
typedef struct {
  u_int32 ssrc;             /* data source being reported */
  unsigned int fraction:8;  /* fraction lost since last SR/RR */
  int lost:24;              /* cumul. no. pkts lost (signed!) */
  u_int32 last_seq;         /* extended last seq. no. received */
  u_int32 jitter;           /* interarrival jitter */
  u_int32 lsr;              /* last SR packet from this source */
  u_int32 dlsr;             /* delay since last SR packet */
} rtcp_rr_t;

/*
 * SDES item
 */
typedef struct {
  u_int8 type;              /* type of item (rtcp_sdes_type_t) */
  u_int8 length;            /* length of item (in octets) */
  char data[1];             /* text, not null-terminated */
} rtcp_sdes_item_t;

/*
 * One RTCP packet
 */
typedef struct {
  rtcp_common_t common;     /* common header */
  union {
    /* sender report (SR) */
    struct {
      u_int32 ssrc;     /* sender generating this report */
      u_int32 ntp_sec;  /* NTP timestamp */
      u_int32 ntp_frac;
      u_int32 rtp_ts;   /* RTP timestamp */
      u_int32 psent;    /* packets sent */
      u_int32 osent;    /* octets sent */
      rtcp_rr_t rr[1];  /* variable-length list */
    } sr;
    
    /* reception report (RR) */
    struct {
      u_int32 ssrc;     /* receiver generating this report */
      rtcp_rr_t rr[1];  /* variable-length list */
    } rr;
    
    /* source description (SDES) */
    struct rtcp_sdes {
      u_int32 src;      /* first SSRC/CSRC */
      rtcp_sdes_item_t item[1]; /* list of SDES items */
    } sdes;
    
    /* BYE */
    struct {
      u_int32 src[1];   /* list of sources */
      
      /* can't express trailing text for reason */
    } bye;
  } r;
} rtcp_t;

typedef struct rtcp_sdes rtcp_sdes_t;

/*
 * Per-source state information
 */
typedef struct {
  u_int16 max_seq;        /* highest seq. number seen */
  u_int32 cycles;         /* shifted count of seq. number cycles */
  u_int32 base_seq;       /* base seq number */
  u_int32 bad_seq;        /* last 'bad' seq number + 1 */
  u_int32 probation;      /* sequ. packets till source is valid */
  u_int32 received;       /* packets received */
  u_int32 expected_prior; /* packet expected at last interval */
  u_int32 received_prior; /* packet received at last interval */
  u_int32 transit;        /* relative trans time for prev pkt */
  u_int32 jitter;         /* estimated jitter */
  /* ... */
} source;

