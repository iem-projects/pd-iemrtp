/*
 * rtcp.h: RTP support for Pd (RTCP components)
 *
 * (c) 2013 IOhannes m zmölnig, institute of electronic music and acoustics (iem)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#define RTP_MAX_SDES 255      /* maximum text length for SDES */

typedef enum {
  RTCP_SR   = 200, /* Sender Report */
  RTCP_RR   = 201, /* Receiver Report */
  RTCP_SDES = 202, /* Source DEScription */
  RTCP_BYE  = 203, /* GoodBYE */
  RTCP_APP  = 204, /* APPlication specific */
  RTCP_RTPFB= 205, /* Transport layer FB message */
  RTCP_PSFB = 206, /* Payload-specific FB message */
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
#define SELECTOR_RTCP_HEADER_SUBTYPE  gensym("subtype")
#define SELECTOR_RTCP_HEADER_TYPE     gensym("type")
#define SELECTOR_RTCP_HEADER_FORMAT   gensym("format")

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
#define SELECTOR_RTCP_SR_RR       SELECTOR_RTCP_RR

#define SELECTOR_RTCP_SDES       gensym("SDES")
#define SELECTOR_RTCP_SDES_SRC   gensym("SRC")

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


#define SELECTOR_RTCP_RTPFB      gensym("RTPFB")
#define SELECTOR_RTCP_RTPFB_NACK gensym("NACK")
#define SELECTOR_RTCP_RTPFB_SENDER_SSRC gensym("senderSSRC")
#define SELECTOR_RTCP_RTPFB_MEDIA_SSRC gensym("mediaSSRC")

typedef enum {
  RTCP_RTPFB_NACK = 1,  /* Not ACKnowledged */
  RTCP_RTPFB_X    = 31, /* for future expansion of the ID number space */
  /* the values 0, 2..30 are unassigned FMTs */
} rtcp_rtpfb_type_t;


#define SELECTOR_RTCP_PSFB       gensym("PSFB")
#define SELECTOR_RTCP_PSFB_PLI   gensym("PLI")
#define SELECTOR_RTCP_PSFB_SLI   gensym("SLI")
#define SELECTOR_RTCP_PSFB_RPSI  gensym("RPSI")
#define SELECTOR_RTCP_PSFB_RPSI_PB  gensym("padding")
#define SELECTOR_RTCP_PSFB_RPSI_PT  gensym("pt")

#define SELECTOR_RTCP_PSFB_AFB   gensym("AFB")
typedef enum {
 RTCP_PSFB_PLI  =  1, /* Picture Loss Indication (PLI) */
 RTCP_PSFB_SLI  =  2, /* Slice Loss Indication (SLI) */
 RTCP_PSFB_RPSI =  3, /* Reference Picture Selection Indication (RPSI) */
 RTCP_PSFB_AFB  = 15, /* Application layer FB (AFB) message */
 RTCP_PSFB_X    = 31, /* reserved for future expansion of the sequence number space */
 /* other values between 0..30 are unassigned FMTs */
} rtcp_psfb_type_t;

/*
 * RTCP common header word
 */
typedef struct {
  unsigned int version:2;   /* protocol version */
  unsigned int p:1;         /* padding flag */
  unsigned int subtype:5;   /* varies by packet type */

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


typedef struct rtcp_rtpfb_nack_ {
  unsigned int pid:16;
  unsigned int blp:16;
}  rtcp_rtpfb_nack_t;
typedef struct rtcp_rtpfb_common_nack {
#define MAX_RTPFB_NACK_COUNT (65536-2)
  rtcp_rtpfb_nack_t*nack;
  u_int32           nack_count;
} rtcp_rtpfb_common_nack_t;

typedef struct  rtcp_psfb_sli_ {
  unsigned int first:13;
  unsigned int number:13;
  unsigned int pictureid:6;
}  rtcp_psfb_sli_t;
typedef struct rtcp_psfb_common_sli_ {
  rtcp_psfb_sli_t*sli;
  u_int32         sli_count;
} rtcp_psfb_common_sli_t;

typedef struct  rtcp_psfb_rpsi_ {
  unsigned int pb:8;
  unsigned int zero:1;
  unsigned char pt:7;
  unsigned char*data;
  u_int32       data_count;
}  rtcp_psfb_rpsi_t;

typedef struct {
  u_int32 sender; /* receiver generating this report */
  u_int32  media; /* SSRC of the media we send info about (remote) */
} rtcp_common_fbsrc_t;

typedef struct {
  rtcp_common_fbsrc_t ssrc; /* sender/media ssrc */
  rtcp_rtpfb_common_nack_t nack;
} rtcp_common_rtpfb_t;
typedef union {
  rtcp_psfb_common_sli_t sli;
  rtcp_psfb_rpsi_t rpsi;
} rtcp_psfb_common_t;
typedef struct {
  rtcp_common_fbsrc_t ssrc; /* sender/media ssrc */
  rtcp_psfb_common_t psfb;
} rtcp_common_psfb_t;


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

    /* RTP feedback (RTPFB) */
    rtcp_common_rtpfb_t rtpfb;
    /* payload specific feedback (PSFB) */
    rtcp_common_psfb_t psfb;
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

/**
 * @brief synthesize a byte-package (atom list) from an RTCP struct.
 * @param RTCP struct to an initialized RTCP struct
 * @param argc total length of the list
 * @param argv array of bytes (as atoms)
 * @return the number of elements consumed within the atom-list
 *         on error, 0 or a negative number (minimum expected list size ) is returned
 */
int iemrtp_rtcp2atoms(const rtcp_t*x, int argc, t_atom*argv);

/**
 * @brief make sure that the subtype field for the RTCP-packet is valid
 * @param RTCP struct to be checked (and fixed if needed/possible)
 * @return TRUE is the subtype field is now valid (FALSE if we don't know)
 */
int iemrtp_rtcp_fixsubtype(rtcp_t*x);


/* change the type of the RTCP packet,
 * freeing ressources no longer needed
 * and initializing to a minimal set
 */
void iemrtp_rtcp_changetype(rtcp_t*rtcp, const rtcp_type_t pt);
/* change type of RTPFB packet */
void iemrtp_rtcp_rtpfb_changetype(rtcp_t*rtcp, const rtcp_rtpfb_type_t typ);
/* change type of PSFB packet */
void iemrtp_rtcp_psfb_changetype(rtcp_t*rtcp, const rtcp_psfb_type_t typ);
/* parse atom into type */
int iemrtp_rtcp_atom2rtpfbtype(t_atom[1]);
int iemrtp_rtcp_atom2psfbtype (t_atom[1]);

/* make sure that at least <size> elements can fit into the rtcp.r.rr struct
 */
int iemrtp_rtcp_ensureRR(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.sr struct
 */
int iemrtp_rtcp_ensureSR(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.sdes struct
 */
int iemrtp_rtcp_ensureSDES(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.bye struct
 */
int iemrtp_rtcp_ensureBYE(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.rtpfb.nack struct
 */
int iemrtp_rtcp_ensureNACK(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.rtpfb.sli struct
 */
int iemrtp_rtcp_ensureSLI(rtcp_t*rtcp, int size);
/* make sure that at least <size> elements can fit into the rtcp.r.psfb.rpsi struct
 */
int iemrtp_rtcp_ensureRPSI(rtcp_t*rtcp, u_int32 size);
