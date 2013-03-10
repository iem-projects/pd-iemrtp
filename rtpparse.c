/*
 * rtpparse: parse RTP packages to get some info
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
#include <stdlib.h>
static t_class *rtpparse_class;

typedef struct _rtpparse
{
	t_object x_obj;
  t_rtpheader x_rtpheader;
} t_rtpparse;

static void rtpparse_bang(t_rtpparse*x){
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

static void rtpparse_list(t_rtpparse*x, t_symbol*s, int argc, t_atom*argv){
  int result=atoms2header(argc, argv, &x->x_rtpheader);
  if(result>0) {
    rtpparse_bang(x);
  } else {
    pd_error(x, "list too short to form a valid RTP-packet (expected %d, got %d)",-result, argc);
  }
    }

/* create rtpparse with args <channels> <skip> */
static void *rtpparse_new(void)
{
	t_rtpparse *x = (t_rtpparse *)pd_new(rtpparse_class);

	outlet_new(&x->x_obj, 0);
	return (x);
}



static void rtpparse_free(t_rtpparse *x) {
  free(x->x_rtpheader.csrc);
  x->x_rtpheader.csrc=NULL;
}

void rtpparse_setup(void)
{
	rtpparse_class = class_new(gensym("rtpparse"), (t_newmethod)rtpparse_new, (t_method)rtpparse_free,
		sizeof(t_rtpparse), 0,0);

	class_addlist(rtpparse_class, (t_method)rtpparse_list);
}
