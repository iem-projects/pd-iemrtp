/*
 * L16pay: RTP-playloader for L16
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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

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

typedef unsigned char u_int8;
typedef unsigned int u_int32;

static t_class *L16pay_class;

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

typedef struct _L16pay
{
	t_object x_obj;
  t_sample**x_in;
  u_int32 x_channels;  // number of channels
  u_int32 x_vecsize;   // Pd's blocksize
  u_int32 x_mtu;       // MTU of the socket

  t_rtpheader  x_rtpheader; // the RTP-header
  u_int32 x_rtpheadersize; // the size of the RTP-header

  /* buffer to store the bytes into */
  u_int8*x_buffer;
  u_int32 x_buffersize;

  /* buffer for outputting bytes as list */
  t_atom*x_atombuffer;
  u_int32 x_atombuffersize;
  t_clock*x_clock;

  /* packetizing */
  u_int32 x_payload;
} t_L16pay;

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

#if 0
  for(j=0; j<i; j++) {
    int x=atom_getint(ap0+j);
    unsigned char c=x;
    if(0==j%4)startpost(" "); startpost("%02x", c);
  } endpost();
#endif

  return i;
}

static void L16pay_preparePacket(t_L16pay*x) {
  u_int32 payload; // number of bytes in a single block

  payload=x->x_channels * x->x_vecsize * RTP_BYTESPERSAMPLE; // number of bytes in a single block

  post("L16pay: channels=%d/%d @ %d", x->x_channels, x->x_vecsize, x->x_mtu);
  post("\tpayload: %d"              , payload);

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
  post("\tbufsize: %d", x->x_atombuffersize);
}


/* ******************************************************************************** */
/*                          the work                krow eht                        */
/* ******************************************************************************** */


static t_int *L16pay_perform(t_int *w)
{
  const t_sample scale = 32767.;
	t_L16pay* x = (t_L16pay*)(w[1]);
  u_int32 n, vecsize = x->x_vecsize;
  u_int32 c, channels=x->x_channels;
  t_sample**ins=x->x_in;
  short*buffer=(short*)x->x_buffer;

  for(n=0; n<vecsize; n++) {
    for (c=0;c<channels;c++) {
      short s = ins[c][n] * scale;
      /* convert to big-endian */
#ifdef LITTLE_ENDIAN
      s=(s << 8) | (s >> 8);
#endif
      *buffer++=s;
    }
  }
  clock_delay(x->x_clock, 0);
	return(w+2);
}

#define EMPTYPACKETBYTES 100
static void L16pay_tick(t_L16pay *x) {      /* callback function for the clock */
  u_int8*buffer=x->x_buffer;
  int payload=x->x_payload;
  u_int32 mtu = x->x_mtu;
  u_int32 channels = x->x_channels;

  while(payload>0) {
    u_int32 headersize;
    u_int32 packetsize;
    u_int32 frames;
    t_atom*ap;
    u_int32 j;

    headersize=header2atoms(&x->x_rtpheader, x->x_atombuffer);
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
    frames=packetsize / (channels * RTP_BYTESPERSAMPLE);
    /* actual packetsize, with full frames */
    packetsize = frames * (channels * RTP_BYTESPERSAMPLE);

    if(packetsize<1) {
      pd_error(x, "oops, empty packet...");
      return;
    }

    ap=x->x_atombuffer + headersize;
    for(j=0; j<packetsize; j++) {
      ap[j].a_w.w_float=*buffer++;
    }
    outlet_list(x->x_obj.ob_outlet, &s_list, headersize+packetsize, x->x_atombuffer);

    x->x_rtpheader.seq +=1;
    x->x_rtpheader.ts  +=frames;

    payload-=packetsize;
  }
}


/* ---------------------- Setup junk -------------------------- */

static void L16pay_dsp(t_L16pay *x, t_signal **sp)
{
  u_int32 c;

  x->x_vecsize=sp[0]->s_n;
  L16pay_preparePacket(x);

  for(c=0; c<x->x_channels; c++) {
    x->x_in[c]=sp[c]->s_vec;
  }

  dsp_add(L16pay_perform, 1, x);
}

static void L16pay_MTU(t_L16pay *x, t_floatarg f)
{
	int t = f;
  if(f<x->x_rtpheadersize) {
    pd_error(x, "MTU-size (%d) must not be smaller than %d", t, x->x_rtpheadersize);
  } else {
    x->x_mtu = t;
  }
}



/* create L16pay with args <channels> <skip> */
static void *L16pay_new(t_floatarg fchan)
{
	t_L16pay *x = (t_L16pay *)pd_new(L16pay_class);
  int ichan = fchan;
	int c;
  if(ichan < 1)
    ichan = 2;
  c=ichan;

	x->x_channels = ichan;
  x->x_vecsize  = 1024;
  x->x_mtu      = 1500;
  x->x_atombuffer    = NULL;
  x->x_atombuffersize= 0;
  x->x_buffer        = NULL;
  x->x_buffersize    = 0;

  x->x_clock=clock_new(x, (t_method)L16pay_tick);

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

  L16pay_preparePacket(x);

  x->x_in = getbytes(x->x_channels * sizeof(t_sample*));

	while (--c) {
		inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal")); /* channels inlet */
	}
	outlet_new(&x->x_obj, gensym("list"));

	return (x);
}



static void L16pay_free(t_L16pay *x) {
  if(x->x_buffer)     freebytes(x->x_buffer    , x->x_buffersize);
  if(x->x_atombuffer) freebytes(x->x_atombuffer, x->x_atombuffersize * sizeof(*(x->x_atombuffer)));
  if(x->x_in)         freebytes(x->x_in        , x->x_channels       * sizeof(t_sample*));
  if(x->x_clock)      clock_free(x->x_clock);
}

void L16pay_setup(void)
{
	L16pay_class = class_new(gensym("L16pay"), (t_newmethod)L16pay_new, (t_method)L16pay_free,
		sizeof(t_L16pay), 0, A_DEFFLOAT,0);
	class_addmethod(L16pay_class, nullfn, gensym("signal"), 0);
	class_addmethod(L16pay_class, (t_method)L16pay_dsp, gensym("dsp"), 0);

	class_addmethod(L16pay_class, (t_method)L16pay_MTU, gensym("mtu"), A_FLOAT, 0);
}
