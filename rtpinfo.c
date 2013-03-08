/*
 * rtpinfo: parse RTP packages to get some info
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
#include "rtp.h"

static t_class *rtpinfo_class;

#define STATIC_INLINE static inline

typedef struct _rtpinfo
{
	t_object x_obj;
  t_rtpheader x_rtpheader;
} t_rtpinfo;

static void rtpinfo_bang(t_rtpinfo*x){
  t_atom ap[4];
  t_rtpheader*rtp=&x->x_rtpheader;
  unsigned int version = rtp->version;
  unsigned int cc = rtp->cc, c;
  SETFLOAT(ap+0, version);
  outlet_anything(x->x_obj.ob_outlet, gensym("version"), 1, ap);
  if(version!=2) {
    static int printerror=1;
    if(printerror)
      pd_error(x, "cannot parse RTP-header v%d", version);
    printerror=0;
  }

  SETFLOAT(ap+0, rtp->p);
  outlet_anything(x->x_obj.ob_outlet, gensym("padding"), 1, ap);
  SETFLOAT(ap+0, rtp->x);
  outlet_anything(x->x_obj.ob_outlet, gensym("extension"), 1, ap);
  SETFLOAT(ap+0, rtp->cc);
  outlet_anything(x->x_obj.ob_outlet, gensym("cc"), 1, ap);
  SETFLOAT(ap+0, rtp->m);
  outlet_anything(x->x_obj.ob_outlet, gensym("marker"), 1, ap);
  SETFLOAT(ap+0, rtp->pt);
  outlet_anything(x->x_obj.ob_outlet, gensym("payload_type"), 1, ap);
  SETFLOAT(ap+0, rtp->seq);
  outlet_anything(x->x_obj.ob_outlet, gensym("sequence_number"), 1, ap);
  SETUINT32(ap, rtp->ts);
  outlet_anything(x->x_obj.ob_outlet, gensym("timestamp"), 2, ap);
  SETUINT32(ap, rtp->ssrc);
  outlet_anything(x->x_obj.ob_outlet, gensym("SSRC"), 2, ap);
  for(c=0; c<cc; c++) {
    SETFLOAT(ap+0, c);
    SETUINT32(ap+1, rtp->csrc[c]);
    outlet_anything(x->x_obj.ob_outlet, gensym("CSRC"), 3, ap);
  }
}
STATIC_INLINE int atoms2header(int argc, t_atom*argv, t_rtpheader*rtpheader) {

  return -1;
}

static void rtpinfo_list(t_rtpinfo*x, t_symbol*s, int argc, t_atom*argv){
  int result=atoms2header(argc, argv, &x->x_rtpheader);
  if(!result) {
    rtpinfo_bang(x);
  } else {
    pd_error(x, "failed to parse data: is it an RTP-packet?");
  }
}

/* create rtpinfo with args <channels> <skip> */
static void *rtpinfo_new(void)
{
	t_rtpinfo *x = (t_rtpinfo *)pd_new(rtpinfo_class);

	outlet_new(&x->x_obj, 0);
	return (x);
}



static void rtpinfo_free(t_rtpinfo *x) {
  free(x->x_rtpheader.csrc);
  x->x_rtpheader.csrc=NULL;
}

void rtpinfo_setup(void)
{
	rtpinfo_class = class_new(gensym("rtpinfo"), (t_newmethod)rtpinfo_new, (t_method)rtpinfo_free,
		sizeof(t_rtpinfo), 0,0);

	class_addlist(rtpinfo_class, (t_method)rtpinfo_list);
}
