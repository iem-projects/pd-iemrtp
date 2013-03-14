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


#define SELECTOR_RTCP_HEADER_VERSION  gensym("version")
#define SELECTOR_RTCP_HEADER_P        gensym("padding")
#define SELECTOR_RTCP_HEADER_COUNT    gensym("count")
#define SELECTOR_RTCP_HEADER_PT       gensym("payload_type")
#define SELECTOR_RTCP_HEADER_TYPE     gensym("type")

#define SELECTOR_RTCP_RR          gensym("RR")
#define SELECTOR_RTCP_RR_SSRC     gensym("SSRC")
#define SELECTOR_RTCP_RR_FRACTION gensym("fraction")
#define SELECTOR_RTCP_RR_LOST     gensym("lost")
#define SELECTOR_RTCP_RR_LAST_SEQ gensym("last_seq")
#define SELECTOR_RTCP_RR_JITTER   gensym("jitter")
#define SELECTOR_RTCP_RR_LSR      gensym("lsr")
#define SELECTOR_RTCP_RR_DLSR     gensym("dlsr")

#define SELECTOR_RTCP_SR          gensym("SR")
#define SELECTOR_RTCP_SR_SSRC     SELECTOR_RTCP_RR_SSRC
#define SELECTOR_RTCP_SR_NTP      gensym("NTP")
#define SELECTOR_RTCP_SR_TS       gensym("timestamp")
#define SELECTOR_RTCP_SR_PSENT    gensym("packets_sent")
#define SELECTOR_RTCP_SR_OSENT    gensym("octets_sent")

#define SELECTOR_RTCP_SDES       gensym("SDES")
#define SELECTOR_RTCP_SDES_SRC   gensym("SRC")
#define SELECTOR_RTCP_SDES_SSRC  SELECTOR_RTCP_RR_SSRC
#define SELECTOR_RTCP_SDES_END   gensym("END")
#define SELECTOR_RTCP_SDES_CNAME gensym("CNAME")
#define SELECTOR_RTCP_SDES_NAME  gensym("NAME")
#define SELECTOR_RTCP_SDES_EMAIL gensym("EMAIL")
#define SELECTOR_RTCP_SDES_PHONE gensym("PHONE")
#define SELECTOR_RTCP_SDES_LOC   gensym("LOC")
#define SELECTOR_RTCP_SDES_TOOL  gensym("TOOL")
#define SELECTOR_RTCP_SDES_NOTE  gensym("NOTE")
#define SELECTOR_RTCP_SDES_PRIV  gensym("PRIV")

#define SELECTOR_RTCP_BYE        gensym("BYE")
#define SELECTOR_RTCP_BYE_SRC    gensym("SRC")

#define SELECTOR_RTCP_APP        gensym("APP")

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
  char*data;                /* text, (not) null-terminated */
} rtcp_sdes_item_t;

typedef struct rtcp_common_sr_ {
  u_int32 ssrc;     /* sender generating this report */
  u_int32 ntp_sec;  /* NTP timestamp */
  u_int32 ntp_frac;
  u_int32 rtp_ts;   /* RTP timestamp */
  u_int32 psent;    /* packets sent */
  u_int32 osent;    /* octets sent */

  rtcp_rr_t*rr;        /* variable-length list */
  u_int32   rr_count;  /* length of list */
} rtcp_common_sr_t;
typedef struct rtcp_common_rr_ {
  u_int32 ssrc;        /* receiver generating this report */
  rtcp_rr_t*rr;        /* variable-length list */
  u_int32   rr_count;  /* length of list */
} rtcp_common_rr_t;
typedef struct  rtcp_sdes {
  u_int32 src;      /* first SSRC/CSRC */
  rtcp_sdes_item_t*item; /* list of SDES items */
  u_int32   item_count;  /* length of list */
} rtcp_sdes_t;
typedef struct  rtcp_common_bye_ {
  u_int32*src;       /* list of sources */
  u_int32 src_count; /* length of list */

  /* can't express trailing text for reason */
} rtcp_common_bye_t;
/*
 * One RTCP packet
 */
typedef struct {
  rtcp_common_t common;     /* common header */
  union {
    /* sender report (SR) */
    rtcp_common_sr_t sr;

    /* reception report (RR) */
    rtcp_common_rr_t rr;

    /* source description (SDES) */
    rtcp_sdes_t sdes;

    /* BYE */
    rtcp_common_bye_t bye;
  } r;
} rtcp_t;


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

/**
 * @brief free dynamically allocated members in RTCP struct.
 *        the rtcp struct itself is not freed;
 *        the freed members are set to valid (zero) values.
 * @param x pointer to initialized RTCP struct
 */
void iemrtp_rtcp_freemembers(rtcp_t*x);
/**
 * @brief parse a byte-package (atom list) to an rtcp struct.
 * @param argc total length of the list
 * @param argv array of bytes (as atoms)
 * @param RTCP struct to initialized RTP-header, that is used as output.
 * @note any dynamically allocated data in the RTCP struct will be deleted and replaced.
 * @return the number of bytes consumed by the struct
 *         on error, 0 or a negative number (minimum expected packet size ) is returned
 */
int iemrtp_atoms2rtcp(int argc, t_atom*argv, rtcp_t*x);
