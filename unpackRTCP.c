/*
 * unpackRTCP: parse RTCP packages to get some info
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
#include "iemrtp.h"
#include <stdlib.h>
static t_class *unpackRTCP_class;

typedef struct _unpackRTCP
{
	t_object x_obj;
  t_outlet*x_countout;
  t_outlet*x_infoout;
  t_outlet*x_rejectout;
  rtcp_t x_rtcpheader;
} t_unpackRTCP;


static void unpackRTCP_rrlist(t_outlet*out, t_symbol*s, u_int32 argc, rtcp_rr_t*argv){
  t_atom ap[4];
  u_int32 i;
  for(i=0; i<argc; i++) {
    rtcp_rr_t*rr=argv+i;
    SETFLOAT(ap+0, i);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_SSRC);
    SETUINT32(ap+2, rr->ssrc);
    outlet_anything(out, s, 4, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_FRACTION);
    SETFLOAT(ap+2, rr->fraction);
    outlet_anything(out, s, 3, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_LOST);
    SETFLOAT(ap+2, rr->lost);
    outlet_anything(out, s, 3, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_LAST_SEQ);
    SETUINT32(ap+2, rr->last_seq);
    outlet_anything(out, s, 4, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_JITTER);
    SETUINT32(ap+2, rr->jitter);
    outlet_anything(out, s, 4, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_LSR);
    SETUINT32(ap+2, rr->lsr);
    outlet_anything(out, s, 4, ap);

    SETSYMBOL(ap+1, SELECTOR_RTCP_RR_DLSR);
    SETUINT32(ap+2, rr->dlsr);
    outlet_anything(out, s, 4, ap);
  }
}
static void unpackRTCP_sdesitems(t_outlet*out, u_int32 argc, rtcp_sdes_item_t*argv){
  t_atom ap[2];
  t_symbol*s_sdes=SELECTOR_RTCP_SDES;
  u_int32 i;
  for(i=0; i<argc; i++) {
    rtcp_sdes_item_t*sdes=argv+i;
    int count=1;
    switch(sdes->type) {
    case(RTCP_SDES_END  ):
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_END);
      outlet_anything(out, s_sdes, count, ap);
      return;
      break;
    case(RTCP_SDES_CNAME): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_CNAME);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_NAME ): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_NAME);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_EMAIL): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_EMAIL);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_PHONE): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_PHONE);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_LOC  ): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_LOC);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_TOOL ): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_TOOL);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_NOTE ): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_NOTE);
      outlet_anything(out, s_sdes, count, ap);
      break;
    case(RTCP_SDES_PRIV ): 
      if(sdes->data && sdes->data[0]) {SETSYMBOL(ap+count, gensym(sdes->data)); count++; }
      SETSYMBOL(ap, SELECTOR_RTCP_SDES_PRIV);
      outlet_anything(out, s_sdes, count, ap);
      break;
    }
  }
}

/* sender report */
static void unpackRTCP_sr(t_unpackRTCP*x){
  t_atom ap[4];
  t_outlet*out=x->x_infoout;
  rtcp_t*rtcp=&x->x_rtcpheader;

  SETUINT32(ap, rtcp->r.sr.ssrc);
  outlet_anything(out, SELECTOR_RTCP_SR_SSRC, 2, ap);

  SETUINT32(ap+0, rtcp->r.sr.ntp_sec);
  SETUINT32(ap+2, rtcp->r.sr.ntp_frac);
  outlet_anything(out, SELECTOR_RTCP_SR_NTP, 4, ap);

  SETUINT32(ap, rtcp->r.sr.rtp_ts);
  outlet_anything(out, SELECTOR_RTCP_SR_TS, 2, ap);

  SETUINT32(ap, rtcp->r.sr.psent);
  outlet_anything(out, SELECTOR_RTCP_SR_PSENT, 2, ap);
  SETUINT32(ap, rtcp->r.sr.osent);
  outlet_anything(out, SELECTOR_RTCP_SR_OSENT, 2, ap);

  unpackRTCP_rrlist(out, SELECTOR_RTCP_SR, rtcp->r.sr.rr_count, rtcp->r.sr.rr);
}
static void unpackRTCP_rr(t_unpackRTCP*x){
  t_atom ap[2];
  t_outlet*out=x->x_infoout;
  rtcp_t*rtcp=&x->x_rtcpheader;

  SETUINT32(ap, rtcp->r.rr.ssrc);
  outlet_anything(out, SELECTOR_RTCP_RR_SSRC, 2, ap);
  unpackRTCP_rrlist(out, SELECTOR_RTCP_RR, rtcp->r.rr.rr_count, rtcp->r.rr.rr);
}

static void unpackRTCP_sdes(t_unpackRTCP*x){
  t_atom ap[2];
  t_outlet*out=x->x_infoout;
  rtcp_t*rtcp=&x->x_rtcpheader;

  SETUINT32(ap, rtcp->r.sdes.src);
  outlet_anything(out, SELECTOR_RTCP_SDES_SRC, 2, ap);
  unpackRTCP_sdesitems(out, rtcp->r.sdes.item_count, rtcp->r.sdes.item);
}
static void unpackRTCP_bye(t_unpackRTCP*x){
  t_atom ap[3];
  t_outlet*out=x->x_infoout;
  rtcp_t*rtcp=&x->x_rtcpheader;
  u_int32*srcs=rtcp->r.bye.src;
  u_int32 src;
  u_int32 count=0;
#warning FIXME handle empty SRC list
  while((src=(*srcs++))) {
    SETFLOAT (ap+0, count++);
    SETUINT32(ap+1, src);
    outlet_anything(out, SELECTOR_RTCP_BYE, 3, ap);
  }
}

static void unpackRTCP_rtcp(t_unpackRTCP*x){
  t_atom ap[4];
  rtcp_common_t*rtcp=&x->x_rtcpheader.common;
  unsigned int version = rtcp->version;
  t_outlet*out=x->x_infoout;

  unsigned int type =  rtcp->pt;
  SETFLOAT(ap+0, version);
  outlet_anything(out, SELECTOR_RTCP_HEADER_VERSION, 1, ap);
  if(version!=2) {
    static int printerror=1;
    if(printerror)
      pd_error(x, "cannot parse RTCP-header v%d", version);
    printerror=0;
  }

  SETFLOAT(ap+0, rtcp->p);
  outlet_anything(out, SELECTOR_RTCP_HEADER_P, 1, ap);
  switch(type) {
  case RTCP_SR  : SETSYMBOL(ap+0, SELECTOR_RTCP_SR); break;
  case RTCP_RR  : SETSYMBOL(ap+0, SELECTOR_RTCP_RR); break;
  case RTCP_SDES: SETSYMBOL(ap+0, SELECTOR_RTCP_SDES); break;
  case RTCP_BYE : SETSYMBOL(ap+0, SELECTOR_RTCP_BYE); break;
  case RTCP_APP : SETSYMBOL(ap+0, SELECTOR_RTCP_APP); break;
  default:
    SETFLOAT(ap+0, type);
  }
  outlet_anything(out, SELECTOR_RTCP_HEADER_TYPE, 1, ap);

  SETFLOAT(ap+0, rtcp->count);
  outlet_anything(out, SELECTOR_RTCP_HEADER_COUNT, 1, ap);

  switch(type) {
  case(RTCP_SR):
    unpackRTCP_sr(x);
    break;
  case(RTCP_RR):
    unpackRTCP_rr(x);
    break;
  case(RTCP_SDES):
    unpackRTCP_sdes(x);
    break;
  case(RTCP_BYE):
    unpackRTCP_bye(x);
    break;
  case(RTCP_APP):
    //unpackRTCP_app(x);
    break;
  }
}

static void unpackRTCP_list(t_unpackRTCP*x, t_symbol*s, int argc, t_atom*argv){
  int pkt=0;
  int result=0;
  int offset=0;
  result=iemrtp_atoms2rtcp(argc, argv, &x->x_rtcpheader);
  while(result>0) {
    outlet_float(x->x_countout, pkt);
    unpackRTCP_rtcp(x);
    offset+=result;
    if(argc-offset<=0)
      return;
    result=iemrtp_atoms2rtcp(argc-offset, argv+offset, &x->x_rtcpheader);
    pkt++;
  }
  if(result<=0)
    outlet_list(x->x_rejectout, s, argc+offset, argv-offset);
}

/* create unpackRTCP with args <channels> <skip> */
static void *unpackRTCP_new(void)
{
	t_unpackRTCP *x = (t_unpackRTCP *)pd_new(unpackRTCP_class);

	x->x_infoout=outlet_new(&x->x_obj, &s_float);
	x->x_countout=outlet_new(&x->x_obj, &s_list);
	x->x_rejectout=outlet_new(&x->x_obj, &s_list);
	return (x);
}



static void unpackRTCP_free(t_unpackRTCP *x) {
  iemrtp_rtcp_freemembers(&x->x_rtcpheader);
  outlet_free(x->x_infoout);
  outlet_free(x->x_countout);
  outlet_free(x->x_rejectout);
}

void unpackRTCP_setup(void)
{
	unpackRTCP_class = class_new(gensym("unpackRTCP"), (t_newmethod)unpackRTCP_new, (t_method)unpackRTCP_free,
		sizeof(t_unpackRTCP), 0,0);

	class_addlist(unpackRTCP_class, (t_method)unpackRTCP_list);
}
