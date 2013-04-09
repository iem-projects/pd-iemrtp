/*
 * iemrtp: RTP support for Pd (common functionality)
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

#include "iemrtp.h"
#include <string.h>

u_int32 iemrtp_rtpheader2atoms(t_rtpheader*rtpheader, t_atom*ap0) {
  t_atom*ap=ap0;
  u_int8 b;
  u_int32 c, cc=rtpheader->cc;
  b=(rtpheader->version << 6) | (rtpheader->p << 5) | (rtpheader->x << 4) | (rtpheader->cc << 0);
  ap++->a_w.w_float=b;

  b=(rtpheader->m << 7) | (rtpheader->pt << 0);
  ap++->a_w.w_float=b;

  ap+=atombytes_setU16(rtpheader->seq, ap);
  ap+=atombytes_setU32(rtpheader->ts , ap);
  ap+=atombytes_setU32(rtpheader->ssrc,ap);
  for(c=0; c < cc; c++) {
    ap+=atombytes_setU32(rtpheader->csrc[c], ap);
  }
  return (12+cc*4);
}


void iemrtp_rtpheader_freemembers(t_rtpheader*rtpheader) {
 if(rtpheader->csrc)free(rtpheader->csrc);
 rtpheader->csrc=NULL;
 rtpheader->cc=0;
}
int iemrtp_rtpheader_ensureCSRC(t_rtpheader*rtpheader, int size) {
  u_int32*csrc = NULL;
  int i;
  if(size>0x0F || size < 0)return 0; /* invalid size */
  if(size<=rtpheader->cc  )return size; /* already large enough */
  csrc = calloc(size, sizeof(u_int32));
  if(!csrc)return 0;

  for(i=0; i<rtpheader->cc; i++) {
    csrc[i]=rtpheader->csrc[i];
  }
  iemrtp_rtpheader_freemembers(rtpheader);
  rtpheader->csrc = csrc;
  rtpheader->cc   = size;
  return rtpheader->cc;
}


int iemrtp_atoms2rtpheader(int argc, t_atom*argv, t_rtpheader*rtpheader) {
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
  iemrtp_rtpheader_freemembers(rtpheader);
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

/* ======================================================== */


void iemrtp_rtcp_freemembers(rtcp_t*x) {
  switch(x->common.pt) {
  case(RTCP_SR  ):
    if(x->r.sr.rr)freebytes(x->r.sr.rr, x->r.sr.rr_count*sizeof(rtcp_rr_t));
    x->r.sr.rr=NULL;  x->r.sr.rr_count=0;
    break;
  case(RTCP_RR  ):
    if(x->r.rr.rr)freebytes(x->r.rr.rr, x->r.rr.rr_count*sizeof(rtcp_rr_t));
    x->r.rr.rr=NULL;  x->r.rr.rr_count=0;
    break;
  case(RTCP_SDES):
    if(x->r.sdes.item) {
      u_int32 i;
      for(i=0; i<x->r.sdes.item_count; i++) {
        rtcp_sdes_item_t*item=x->r.sdes.item+i;
        free(item->data);
        item->data=NULL; item->length=0; item->type=0;
      }
      freebytes(x->r.sdes.item, x->r.sdes.item_count*sizeof(rtcp_sdes_item_t));
    }
    x->r.sdes.item=NULL; x->r.sdes.item_count=0;
    break;
  case(RTCP_BYE ):
    if(x->r.bye.src)freebytes(x->r.bye.src, x->r.bye.src_count*sizeof(u_int32));
    x->r.bye.src=NULL;  x->r.bye.src_count=0;
    break;
  case(RTCP_APP ):
    if(x->r.sr.rr)freebytes(x->r.sr.rr, x->r.sr.rr_count*sizeof(rtcp_rr_t));
    x->r.sr.rr=NULL;  x->r.sr.rr_count=0;
    break;
  }
}


static void atoms2rtcp_rrlist(u_int32 frames, t_atom*argv, rtcp_rr_t*x) {
  u_int32 f;
  for(f=0; f<frames; f++) {
    t_atom*ap=argv+f*24;
    rtcp_rr_t*rr=x+f;
    rr->ssrc    =atombytes_getU32(ap+ 0);
    rr->fraction=atom_getint     (ap+ 4);
    rr->lost    =atombytes_getU32(ap+ 5) >> 8;
    rr->last_seq=atombytes_getU32(ap+ 8);

    rr->jitter  =atombytes_getU32(ap+ 12);
    rr->lsr     =atombytes_getU32(ap+ 16);
    rr->dlsr    =atombytes_getU32(ap+ 20);
  }
}

static u_int32 atoms2rtcp_sr(rtcp_common_sr_t*x, u_int32 argc, t_atom*argv) {
  const u_int32 framesize=24;
  u_int32 frames;
  frames=(argc/framesize);
  if(frames*framesize > argc) {
    return -(frames*framesize);
  }

  x->ssrc     = atombytes_getU32(argv+ 0);
  x->ntp_sec  = atombytes_getU32(argv+ 4);
  x->ntp_frac = atombytes_getU32(argv+ 8);
  x->rtp_ts   = atombytes_getU32(argv+12);
  x->psent    = atombytes_getU32(argv+16);
  x->osent    = atombytes_getU32(argv+20);

  /* now comes a variable length list of rtcp_rr_t's, each 24 bytes */
  x->rr_count=frames-1;
  x->rr = getbytes(x->rr_count * sizeof(rtcp_rr_t));

  atoms2rtcp_rrlist(x->rr_count, argv + 24, x->rr);

  return frames*framesize;
}
static u_int32 atoms2rtcp_rr(rtcp_common_rr_t*x, u_int32 argc, t_atom*argv) {
  const u_int32 framesize=24;
  u_int32 frames=(argc-4)/framesize;
  if(frames*framesize+4 > argc) {
    return -(frames*framesize+4);
  }

  x->ssrc     = atombytes_getU32(argv+ 0);

  /* now comes a variable length list of rtcp_rr_t's, each framesize bytes */
  x->rr_count=frames;
  x->rr = getbytes(x->rr_count * sizeof(rtcp_rr_t));

  atoms2rtcp_rrlist(x->rr_count, argv + 4, x->rr);

  return frames*framesize+4;
}

static u_int32 atoms2rtcp_sdes(rtcp_sdes_t*x, u_int32 argc, t_atom*argv) {
#warning FIXME: SDES parser broken for multiple chunks
  t_atom*ap;
  u_int32 f, frames = 0;
  u_int8 len;
  u_int32 datalengths=0;
  if(argc<4+1+1) { // SRC+item0.type+item0.length
    return -(4+1+1);
  }
  x->src     = atombytes_getU32(argv+ 0);

  /* count the number of items */
  for(ap=argv+4;
      datalengths<argc-4;
      frames++) {
    int type=atom_getint(ap++);
    if(type==0)break; // stop type
    datalengths++ ; // skip type
    len=atom_getint(ap++); datalengths++; //length of following string
    ap+=len; datalengths+=len;
  }

  x->item_count=frames;
  x->item = getbytes(x->item_count * sizeof(rtcp_sdes_item_t));

  ap=argv+4;
  for(f=0; f<frames; f++) {
    u_int8 l;
    rtcp_sdes_item_t*item=x->item+f;
    item->type=atom_getint(ap++);
    len=atom_getint(ap++);
    item->length=len;
    item->data=malloc(len+1);
    for(l=0; l<len; l++) {
      item->data[l]=atom_getint(ap++);
    }
    item->data[len]=0;
  }
  return argc; // datalengths+4
}



static u_int32 atoms2rtcp_bye(rtcp_common_bye_t*x, u_int32 argc, t_atom*argv) {
  u_int32 f, frames = (argc/4);
  if(frames*4 != argc) {
    return -(frames*4);
  }
  x->src_count=frames;
  x->src = getbytes(x->src_count * sizeof(u_int32));

  for(f=0; f<frames; f++) {
    x->src[f] = atombytes_getU32(argv+4*f);
  }
  return argc;
}
int iemrtp_atoms2rtcp(int argc, t_atom*argv, rtcp_t*x) {
  u_int8 b;
  int retval=4;
  u_int16 length;
  iemrtp_rtcp_freemembers(x);
  if(!argc) return -retval;

  b=atom_getint(argv+0);
  x->common.version=(b >> 6) & 0x03;
  x->common.p      =(b >> 5) & 0x01;
  x->common.count  =(b >> 0) & 0x1F;

  b=atom_getint(argv+1);
  x->common.pt     =b;

  length =atombytes_getU16(argv+2);
  x->common.length =length;

  if((length+1)*4 > argc) {
    return (-(length+1)*4);
  }

  switch(x->common.pt) {
  case(RTCP_SR):
    retval+=atoms2rtcp_sr(&(x->r.sr), length*4, argv+4);
    break;
  case(RTCP_RR):
    retval+=atoms2rtcp_rr(&(x->r.rr), length*4, argv+4);
    break;
  case(RTCP_SDES):
    retval+=atoms2rtcp_sdes(&(x->r.sdes), length*4, argv+4);
    break;
  case(RTCP_BYE):
    retval+=atoms2rtcp_bye(&(x->r.bye), length*4, argv+4);
    break;
  case(RTCP_APP):
    //atoms2rtcp_app(&(x->r.app), length*4, argv+4);
    break;
  }
  return retval;
}
STATIC_INLINE int RTCPatombytes_fromRR(rtcp_rr_t*rr, t_atom*ap) {
  ap+=atombytes_setU32(rr->ssrc    , ap);
  ap++->a_w.w_float   =rr->fraction;
  ap+=atombytes_setU24(rr->lost    , ap);
  ap+=atombytes_setU32(rr->last_seq, ap);
  ap+=atombytes_setU32(rr->jitter  , ap);
  ap+=atombytes_setU32(rr->lsr     , ap);
  ap+=atombytes_setU32(rr->dlsr    , ap);
  return 24;
}
STATIC_INLINE int RTCPatombytes_fromSDES(rtcp_sdes_item_t*item, t_atom*ap) {
  u_int8 i;
  ap++->a_w.w_float   =item->type;
  ap++->a_w.w_float   =item->length;
  for(i=0; i<item->length; i++)
    ap++->a_w.w_float   =item->data[i];
  return 2+item->length;
}

int iemrtp_rtcp2atoms(const rtcp_t*x, int argc, t_atom*ap) {
  int padding=0, reqbytes=0;
  u_int8 b;
  u_int32 i;
  if(!x)return 0;

  /*
    header: 4 bytes
    SR: 6*4 + rr_count*(6*4) = 24*(rr_count+1)
    RR: 4 + rr_count*(6*4)   = 4*(1+6*rr_count)
    SDES: 4 + item_count*(2+length[i])
    BYE: 4*src_count
    APP: ?
  */
  reqbytes=4;
  switch(x->common.pt) {
  case(RTCP_SR  ):
    reqbytes+=4*6*(x->r.sr.rr_count +1);
    break;
  case(RTCP_RR  ):
    reqbytes+=4*6*(x->r.rr.rr_count) + 4;
    break;
  case(RTCP_SDES):
    reqbytes+=4;
    for(i=0; i<x->r.sdes.item_count; i++)
      reqbytes+=2+x->r.sdes.item[i].length;
    reqbytes+=1; // there's an extra 0 byte at the end of SDES items
    break;
  case(RTCP_BYE ):
    reqbytes+=4*x->r.bye.src_count;
    break;
  default:
  case(RTCP_APP ):
    return 0;
  }
  padding = (reqbytes%4)?(4 - (reqbytes & 0x3)):0; // packetsize must be 4byte aligned
  reqbytes += padding;

  if(argc<reqbytes)return -reqbytes; // header takes at least 4 bytes

  /* write header */
  b=(x->common.version << 6) | (x->common.p << 5) | (x->common.count);
  ap++->a_w.w_float=b;
  ap++->a_w.w_float=x->common.pt;
  if(x->common.length)
    ap+=atombytes_setU16(x->common.length, ap);
  else
    ap+=atombytes_setU16((reqbytes-4)>>2, ap);

  switch(x->common.pt) {
  case(RTCP_SR  ):
    ap+=atombytes_setU32(x->r.sr.ssrc    , ap);
    ap+=atombytes_setU32(x->r.sr.ntp_sec , ap);
    ap+=atombytes_setU32(x->r.sr.ntp_frac, ap);
    ap+=atombytes_setU32(x->r.sr.rtp_ts  , ap);
    ap+=atombytes_setU32(x->r.sr.psent   , ap);
    ap+=atombytes_setU32(x->r.sr.osent   , ap);
    for(i=0; i<x->r.sr.rr_count; i++)
      ap+=RTCPatombytes_fromRR(x->r.sr.rr+i      , ap);
    break;
  case(RTCP_RR  ):
    ap+=atombytes_setU32(x->r.rr.ssrc            , ap);
    for(i=0; i<x->r.rr.rr_count; i++)
      ap+=RTCPatombytes_fromRR(x->r.rr.rr+i      , ap);
    break;
  case(RTCP_SDES):
    ap+=atombytes_setU32(x->r.sdes.src           , ap);
    for(i=0; i<x->r.sdes.item_count; i++)
      ap+=RTCPatombytes_fromSDES(x->r.sdes.item+i, ap);
    ap++->a_w.w_float=0; // terminating 0-type item
    break;
  case(RTCP_BYE ):
    for(i=0; i<x->r.bye.src_count; i++)
      ap+=atombytes_setU32(x->r.bye.src[i]       , ap);
    break;
  default:
    return 0;
  }
  while(padding-->0)
    ap++->a_w.w_float=0; // padding with zeros

  return reqbytes;
}

void iemrtp_rtcp_changetype(rtcp_t*rtcp, const rtcp_type_t pt) {
  if(pt==rtcp->common.pt)return;
  else {
    unsigned int version=rtcp->common.version;
    unsigned int p=rtcp->common.p;
    iemrtp_rtcp_freemembers(rtcp);
    memset(rtcp, 0, sizeof(rtcp_t));

    rtcp->common.version=version;
    rtcp->common.p=p;
    rtcp->common.pt     =pt;
  }
}
/* make sure that at least <size> elements can fit into the rtcp.r.rr struct
 * returns the size of the buffer after a possible alloc
 */
#define RTCP_ENSURE(fun, field, typ, minsize, maxsize)                \
  int iemrtp_rtcp_ensure##fun(rtcp_t*rtcp, int size) {                \
    typ*tmp;                                                          \
    u_int32 i;                                                        \
    if(size<minsize) return 0;                                        \
    if(maxsize>minsize && size>maxsize) return 0;                     \
    if(size<=(int)rtcp->r.field##_count)return rtcp->r.field##_count; \
    tmp=calloc(size, sizeof(typ));                                    \
    if(!tmp)return 0;                                                 \
    for(i=0; i<rtcp->r.field##_count; i++)                            \
      tmp[i]=rtcp->r.field[i];                                        \
    iemrtp_rtcp_freemembers(rtcp);                                    \
    rtcp->r.field=tmp;                                                \
    rtcp->r.field##_count=size;                                       \
    return rtcp->r.field##_count;                                     \
  }
RTCP_ENSURE(RR,   rr.rr,     rtcp_rr_t,        0, 0);
RTCP_ENSURE(SR,   sr.rr,     rtcp_rr_t,        0, 0);
RTCP_ENSURE(SDES, sdes.item, rtcp_sdes_item_t, 0, 0);
RTCP_ENSURE(BYE,  bye.src,   u_int32,          0, 0);



/* ======================================================== */

static void rtppay_preparePacket(t_rtppay*x) {
  u_int32 payload; // number of bytes in a single block

  payload=x->x_usedchannels * x->x_vecsize * x->x_bytespersample; // number of bytes in a single block

  /* get buffer for payload */
  if(x->x_buffersize<payload) {
    if(x->x_buffer)freebytes(x->x_buffer, x->x_buffersize);
    x->x_buffersize=payload;
    x->x_buffer = getbytes(x->x_buffersize);
  }
  x->x_payload=payload;

  if(x->x_atombuffersize < x->x_mtu) {
    u_int32 i;
    if(x->x_atombuffer)
      freebytes(x->x_atombuffer, x->x_atombuffersize * sizeof(t_atom));
    x->x_atombuffersize = x->x_mtu;
    x->x_atombuffer = getbytes(x->x_atombuffersize * sizeof(t_atom));

    // initialize the buffers with floats,
    // so we don't have to repeatedly set the atomtype later
    for(i=0; i<x->x_atombuffersize; i++) {
      SETFLOAT(x->x_atombuffer+i, 0);
    }
  }
}


/* ******************************************************************************** */
/*                          the work                krow eht                        */
/* ******************************************************************************** */


static t_int *rtppay_perform(t_int *w)
{
  t_rtppay* x = (t_rtppay*)(w[1]);

  if(!x->x_running && !x->x_banged)return(w+2);
  x->x_banged=0;

  if(x->x_perform) {
    (x->x_perform)(x->x_vecsize, x->x_usedchannels, x->x_in, x->x_buffer);
    clock_delay(x->x_clock, 0);
  }
  return(w+2);
}

static void rtppay_tick(t_rtppay *x) {      /* callback function for the clock */
  u_int8*buffer=x->x_buffer;
  int payload=x->x_payload;
  u_int32 mtu = x->x_mtu;
  u_int32 channels = x->x_usedchannels;

  while(payload>0) {
    u_int32 headersize;
    u_int32 packetsize;
    u_int32 frames;
    t_atom*ap;
    u_int32 j;

    headersize=iemrtp_rtpheader2atoms(&x->x_rtpheader, x->x_atombuffer);
    x->x_rtpheadersize=headersize;

    /* 1st guess at headersize */
    packetsize  = mtu - headersize;
    if(packetsize > (u_int32)payload) {
      /* last packet */
      packetsize = payload;
    } else {
      /* don't fill the packet to the rim (so give some bytes extra) */
      if(packetsize>EMPTYPACKETBYTES*2) packetsize-=EMPTYPACKETBYTES;
    }
    /* number of frames fitting into a package */
    frames=packetsize / (channels * x->x_bytespersample);
    /* actual packetsize, with full frames */
    packetsize = frames * (channels * x->x_bytespersample);

    if(packetsize<1) {
      pd_error(x, "oops, empty packet...");
      return;
    }

    ap=x->x_atombuffer + headersize;
    for(j=0; j<packetsize; j++) {
      ap[j].a_w.w_float=*buffer++;
    }
    outlet_list(x->x_outlet, &s_list, headersize+packetsize, x->x_atombuffer);

    x->x_rtpheader.seq += 1;
    x->x_rtpheader.ts  += frames;
    x->x_rtpheader.m    = 0;

    payload-=packetsize;
  }
}


/* ---------------------- Setup junk -------------------------- */

static void rtppay_dsp(t_rtppay *x, t_signal **sp)
{
  u_int32 c;

  x->x_vecsize=sp[0]->s_n;

  rtppay_preparePacket(x);

  for(c=0; c<x->x_channels; c++) {
    x->x_in[c]=sp[c]->s_vec;
  }

  dsp_add(rtppay_perform, 1, x);
}

static u_int8 rtppay_state(t_rtppay *x, t_float f);
static void rtppay_restart(t_rtppay*x) {
  u_int8 oldstate=rtppay_state(x, 0);
  rtppay_preparePacket(x);
  rtppay_state(x, oldstate);
}

static void rtppay_MTU(t_rtppay *x, t_floatarg f)
{
  int t = f;
  int minsize=x->x_rtpheadersize + x->x_channels * x->x_bytespersample;
  if(f<minsize) {
    pd_error(x, "MTU-size (%d) must not be smaller than %d", t, minsize);
    return;
  }
  x->x_mtu = t;
  rtppay_restart(x);
}

static void rtppay_chans(t_rtppay *x, t_floatarg f)
{
  int i = f;
  unsigned u;
  if(i<0) {
    pd_error(x, "negative number of active channels (%d) not permitted", i);
    return;
  }
  u=i;
  if(u>x->x_channels) {
    pd_error(x, "cannot have more active channels (%u) than physical channels %d", u, x->x_channels);
    return;
  }
  x->x_usedchannels = u;
  rtppay_restart(x);
}

static u_int8 rtppay_state(t_rtppay *x, t_float f) {
  u_int8 oldstate=x->x_running;
  if((int)f)
    x->x_rtpheader.m    = 1;

  x->x_running = (int)f;
  return oldstate;
}

static void rtppay_start(t_rtppay *x) {
  rtppay_state(x, 1);
}
static void rtppay_bang(t_rtppay *x) {
  x->x_banged=1;
}
static void rtppay_stop(t_rtppay *x) {
  rtppay_state(x, 0);
}

static void rtppay_version(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int version = atom_getint(argv);
    if(version != 2) pd_error(x, "currently only version '2' is supported!");
    x->x_rtpheader.version = 2;
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.version);
}
static void rtppay_p(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int p = atom_getint(argv);
    if(p != 0) pd_error(x, "currently only padding '0' is supported!");
    x->x_rtpheader.p = 0;
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.p);
}
static void rtppay_x(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int ext = atom_getint(argv);
    if(ext != 0) pd_error(x, "currently only extension '0' is supported!");
    x->x_rtpheader.x = 0;
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.x);
}
static void rtppay_cc(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int cc = atom_getint(argv);
    if(!cc) {
      iemrtp_rtpheader_freemembers(&x->x_rtpheader);
      return;
    }
    if(!iemrtp_rtpheader_ensureCSRC(&x->x_rtpheader, cc)) {
      pd_error(x, "unable to resize CSRC to %d (must be <%d)!", cc, 0x0F);
    }
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.cc);
}
static void rtppay_m(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int m = atom_getint(argv);
    x->x_rtpheader.m = (m!=0);
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.m);
}
static void rtppay_pt(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int pt = atom_getint(argv);
    x->x_rtpheader.pt = pt;
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.pt);
}
static void rtppay_seq(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int seq = atom_getint(argv);
    x->x_rtpheader.seq = seq;
  }  else     post("%s: %d", s->s_name, x->x_rtpheader.seq);
}
static void rtppay_ts(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc)
    x->x_rtpheader.ts = GETUINT32(argc, argv);
  else
    post("%s: %08x", s->s_name, x->x_rtpheader.ts);
}
static void rtppay_SSRC(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc)
    x->x_rtpheader.ssrc = GETUINT32(argc, argv);
  else
    post("%s: %08x", s->s_name, x->x_rtpheader.ssrc);
}
static void rtppay_CSRC(t_rtppay *x, t_symbol*s, int argc, t_atom*argv) {
  unsigned int i;
  if(argc) {
    switch(argc) {
    case 2: case 3: do {
        int index=atom_getint(argv);
        u_int32 id = GETUINT32(argc-1, argv+1);
        if(!iemrtp_rtpheader_ensureCSRC(&x->x_rtpheader, index+1)) {
          pd_error(x, "couldn't set CSRC-id to %d (must be <%d)", index, 0x0F);
          return;
        }
        x->x_rtpheader.csrc[index]=id;
      } while(0);
      break;
    default:
      pd_error(x, "usage: CSRC <index> <ID:hi> <ID:lo>");
      return;
    }
  } else {
    for(i=0; i<x->x_rtpheader.cc; i++) {
      post("%s[%d]: %08x", s->s_name, i, x->x_rtpheader.csrc[i]);
    }
  }
}


/* create rtppay with args <channels> <skip> */
void *iemrtp_rtppay_new(t_rtppay*x, int ichan, int bytespersample, t_rtppay_perform perform)
{
  int c;
  if(ichan < 1)
    ichan = 2;
  c=ichan;

  x->x_channels = ichan;
  x->x_usedchannels = x->x_channels;
  x->x_vecsize  = 1024;
  x->x_mtu      = 1500;
  x->x_atombuffer    = NULL;
  x->x_atombuffersize= 0;
  x->x_buffer        = NULL;
  x->x_buffersize    = 0;

  if(bytespersample<1) {
    pd_error(x, "bytespersample = %d, must be at least 1", bytespersample);
    bytespersample=1;

  }
  x->x_bytespersample=bytespersample;
  x->x_perform = perform;

  x->x_clock=clock_new(x, (t_method)rtppay_tick);

  x->x_rtpheader.version  = 2;
  x->x_rtpheader.p  = 0;
  x->x_rtpheader.x  = 0;
  x->x_rtpheader.cc = 0;
  x->x_rtpheader.m  = 1;
  x->x_rtpheader.pt = 96;
  x->x_rtpheader.seq = 0;
  x->x_rtpheader.ts = 0;
  x->x_rtpheader.ssrc = 0;
  x->x_rtpheader.csrc = 0;

  x->x_rtpheadersize=sizeof(x->x_rtpheader);

  rtppay_preparePacket(x);

  x->x_in = getbytes(x->x_channels * sizeof(t_sample*));

  while (--c) {
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal")); /* channels inlet */
  }
  x->x_outlet=outlet_new(&x->x_obj, gensym("list"));

  return (x);
}

void iemrtp_rtppay_free(t_rtppay *x) {
  if(x->x_buffer)     freebytes(x->x_buffer    , x->x_buffersize);
  if(x->x_atombuffer) freebytes(x->x_atombuffer, x->x_atombuffersize * sizeof(*(x->x_atombuffer)));
  if(x->x_in)         freebytes(x->x_in        , x->x_channels       * sizeof(t_sample*));
  if(x->x_clock)      clock_free(x->x_clock);
}

void iemrtp_rtppay_classnew(t_class*rtppay_class)
{
  class_addmethod(rtppay_class, nullfn, gensym("signal"), 0);
  class_addmethod(rtppay_class, (t_method)rtppay_dsp, gensym("dsp"), 0);

  class_addbang  (rtppay_class, (t_method)rtppay_bang);
  class_addmethod(rtppay_class, (t_method)rtppay_state, gensym("auto"), A_FLOAT, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_start, gensym("start"), 0);
  class_addmethod(rtppay_class, (t_method)rtppay_stop , gensym("stop" ), 0);
  class_addmethod(rtppay_class, (t_method)rtppay_MTU  , gensym("mtu"), A_FLOAT, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_chans, gensym("channels"), A_FLOAT, 0);


  class_addmethod(rtppay_class, (t_method)rtppay_version, SELECTOR_RTPHEADER_VERSION, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_p,       SELECTOR_RTPHEADER_P, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_x,       SELECTOR_RTPHEADER_X, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_cc,      SELECTOR_RTPHEADER_CC, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_m,       SELECTOR_RTPHEADER_M, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_pt,      SELECTOR_RTPHEADER_PT, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_seq,     SELECTOR_RTPHEADER_SEQ, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_ts,      SELECTOR_RTPHEADER_TS, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_SSRC,    SELECTOR_RTPHEADER_SSRC, A_GIMME, 0);
  class_addmethod(rtppay_class, (t_method)rtppay_CSRC,    SELECTOR_RTPHEADER_CSRC, A_GIMME, 0);
}
