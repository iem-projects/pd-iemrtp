/*
 * rtpbuffer: buffer RTP-packets so they can be retrieved again (based on TS)
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

/* description:
 * this buffers RTP-packets based on their TIMESTAMP,
 * buffer size:
 *     we only buffer a fixed number of packets (buffer size can be set at startup)
 *     if a new packet is to be added to a full buffer, older packets will get dropped
 */

#include "iemrtp.h"
#include <string.h>

static t_class *rtpbuffer_class;

typedef struct _rtpbuffer_packet {
  u_int32 timestamp;
  u_int32 stopstamp;
  int argc;
  t_atom*argv;
  struct _rtpbuffer_packet *next;
} t_rtpbuffer_packet;

typedef struct _rtpbuffer
{
  t_object x_obj;
  t_outlet*x_dataout;
  t_outlet*x_infout;

  t_rtpbuffer_packet*x_start;
  u_int32 x_packetcount;
  u_int32 x_maxpackets;

  t_inlet*x_durin;
  t_float x_packetduration; /* in ticks */
} t_rtpbuffer;

static u_int32 rtpbuffer_getpktdur(t_rtpbuffer*x) {
  u_int32 result=0;
  if(x->x_packetduration>0)
    result=x->x_packetduration;
  x->x_packetduration=0;
  return result;
}

static void rtpbuffer_info_added(t_rtpbuffer*x, u_int32 ts) {
  t_atom ap[2];
  SETUINT32(ap, ts);
  outlet_anything(x->x_infout, gensym("added"), 2, ap);
}
static void rtpbuffer_info_deleted(t_rtpbuffer*x, u_int32 ts) {
  t_atom ap[2];
  SETUINT32(ap, ts);
  outlet_anything(x->x_infout, gensym("deleted"), 2, ap);
}
static void rtpbuffer_pktout(t_rtpbuffer*x, t_rtpbuffer_packet*pkt) {
  outlet_list(x->x_dataout, 0, pkt->argc, pkt->argv);
}


static t_rtpbuffer_packet*packet_create(int argc, t_atom*argv) {
  t_rtpheader hdr;
  t_rtpbuffer_packet*pkt=NULL;
  memset(&hdr, 0, sizeof(hdr));
  if(iemrtp_atoms2rtpheader(argc, argv, &hdr)>0) {
    pkt=(t_rtpbuffer_packet*)calloc(1, sizeof(t_rtpbuffer_packet));
    if(pkt) {
      pkt->timestamp=hdr.ts;
      pkt->argc=argc;
      pkt->argv=(t_atom*)malloc(argc * sizeof(t_atom));
      memcpy(pkt->argv, argv, argc * sizeof(t_atom));
    }
  }
  return pkt;
}
static void packet_destroy(t_rtpbuffer_packet*pkt) {
  if(!pkt)return;
  pkt->timestamp=0;
  pkt->argc=0;
  free(pkt->argv);
  pkt->argv=NULL;
  pkt->next=NULL;
  free(pkt);
}

static void rtpbuffer_freepackets(t_rtpbuffer*x) {
  t_rtpbuffer_packet*pkt;
  t_atom ap[1];
  for(pkt=x->x_start; pkt; pkt=pkt->next) {
    u_int32 ts=pkt->timestamp;
    x->x_start=pkt;
    packet_destroy(pkt);
    x->x_packetcount--;
    rtpbuffer_info_deleted(x, ts);
  }
  x->x_start=NULL;
  if(x->x_packetcount) {
    pd_error(x, "after deleting all packets, we still have a packetcount of %d", x->x_packetcount);
    x->x_packetcount=0;
  }
  SETFLOAT(ap, x->x_packetcount);
  outlet_anything(x->x_infout, gensym("size"), 1, ap);
}


static t_rtpbuffer_packet* UNUSED_FUNCTION(packet_matchTS)(t_rtpbuffer_packet*pkt, const u_int32 timestamp) {
  if((timestamp >= pkt->timestamp) && (timestamp <= pkt->stopstamp)) return pkt;
  return NULL;
}
/* returns the given packet if it has data after timestamp */
static t_rtpbuffer_packet* packet_afterTS(t_rtpbuffer_packet*pkt, const u_int32 timestamp) {
  if(pkt->timestamp>=timestamp)return pkt;
  /* currently this is inaccurate
   * if the packet holds data from TS0..TS1 and timestamp is inbetween (TS0<timestamp<TS1)
   * then the packet is skipped, which it should not
   */
  if(pkt->stopstamp>=timestamp)return pkt;
  return NULL;
}


static void rtpbuffer_queryTS(t_rtpbuffer*x, const u_int32 ts0, const u_int32 ts1){
  t_rtpbuffer_packet*pkt=NULL;
  /* find packet with matching timestamp */
  t_rtpbuffer_packet*buf;

  for(buf=x->x_start; buf; buf=buf->next) {
    pkt=packet_afterTS(buf, ts0);
    if(pkt)break;
  }

  /* and output them */
  if(pkt) {
    for(buf=pkt; buf && buf->timestamp<=ts1; buf=buf->next) {
      rtpbuffer_pktout(x, buf);
    }
  }
}

static void rtpbuffer_query(t_rtpbuffer*x, t_symbol* UNUSED(s), int argc, t_atom*argv) {
  u_int32 ts0, ts1;
  ts0=GETUINT32(argc<2?argc:2, argv);
  if(argc>2)
    ts1=GETUINT32(argc-2, argv+2);
  else
    ts1=ts0;
  rtpbuffer_queryTS(x, ts0, ts1);
}

/* add a new packet to the buffer */
static void rtpbuffer_list(t_rtpbuffer*x, t_symbol* UNUSED(s), int argc, t_atom*argv) {
  u_int32 i;
  t_atom ap[1];

  /* create a new packet */
  t_rtpbuffer_packet*pkt=packet_create(argc, argv);
  /* the previous packet for inserting */
  t_rtpbuffer_packet*prev=NULL;
  /* insert it into the buffer list */
  t_rtpbuffer_packet*buf;

  if(!pkt)return;
  pkt->stopstamp = pkt->timestamp+rtpbuffer_getpktdur(x);

  for(buf=x->x_start; buf; buf=buf->next) {
    if(buf->timestamp > pkt->timestamp)
      break;
    prev=buf;
  }
  if(prev) {
    pkt->next = prev->next;
    prev->next= pkt;
  } else {
    pkt->next = x->x_start;
    x->x_start= pkt;
  }
  rtpbuffer_info_added(x, pkt->timestamp);

  x->x_packetcount++;

  /* remove extraneous packets */
  prev=x->x_start;
  for(i=x->x_maxpackets; i<x->x_packetcount && prev;  i++) {
    u_int32 ts=prev->timestamp;
    x->x_start=prev->next;
    packet_destroy(prev);
    prev=x->x_start;
    x->x_packetcount--;
    rtpbuffer_info_deleted(x, ts);
  }

  SETFLOAT(ap, x->x_packetcount);
  outlet_anything(x->x_infout, gensym("size"), 1, ap);
}
static void rtpbuffer_bang(t_rtpbuffer*x) {
  t_rtpbuffer_packet*pkt;
  for(pkt=x->x_start; pkt; pkt=pkt->next) {
    rtpbuffer_pktout(x, pkt);
  }
}

/* create rtpbuffer with args <channels> <skip> */
static void *rtpbuffer_new(t_float f)
{
  int maxpackets = f;
  t_rtpbuffer *x = (t_rtpbuffer *)pd_new(rtpbuffer_class);

  x->x_maxpackets = (maxpackets<1)?32:maxpackets;
  x->x_packetcount= 0;

  x->x_packetduration=0;
  x->x_durin=floatinlet_new(&x->x_obj, &x->x_packetduration);

  x->x_dataout=outlet_new(&x->x_obj, &s_list);

  /* outlet for misc information, e.g. when we drop buffers, or don't have buffers */
  x->x_infout=outlet_new(&x->x_obj, 0);
  return (x);
}



static void rtpbuffer_free(t_rtpbuffer *x) {
  outlet_free(x->x_dataout);
  outlet_free(x->x_infout);
  inlet_free (x->x_durin);
  rtpbuffer_freepackets(x);
}

void rtpbuffer_setup(void)
{
  rtpbuffer_class = class_new(gensym("rtpbuffer"), (t_newmethod)rtpbuffer_new, (t_method)rtpbuffer_free,
                              sizeof(t_rtpbuffer), 0, A_DEFFLOAT, A_NULL);

  /* dump all the buffered packets */
  class_addbang(rtpbuffer_class, (t_method)rtpbuffer_bang);


  /* add a new buffer */
  class_addlist(rtpbuffer_class, (t_method)rtpbuffer_list);

  /* query a buffer */
  /* query <TShi> <TSlo>: output packet for TS
   * query <TS0hi> <TS0lo> <TS1hi> <TS1lo>: output packets between TS0 and TS1 (incl.)
   */
  class_addmethod(rtpbuffer_class, (t_method)rtpbuffer_query, gensym("query"), A_GIMME);
}
