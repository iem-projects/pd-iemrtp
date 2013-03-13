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
#include "iemrtp.h"
#include <stdlib.h>
static t_class *rtpparse_class;

typedef struct _rtpparse
{
	t_object x_obj;
  t_outlet*x_dataout;
  t_outlet*x_infoout;
  t_outlet*x_rejectout;

  t_rtpheader x_rtpheader;
} t_rtpparse;

static void rtpparse_bang(t_rtpparse*x){
  t_atom ap[4];
  t_rtpheader*rtp=&x->x_rtpheader;
  unsigned int version = rtp->version;
  unsigned int cc = rtp->cc, c;
  t_outlet*out=x->x_infoout;
  SETFLOAT(ap+0, version);
  outlet_anything(out, SELECTOR_RTPHEADER_VERSION, 1, ap);
  if(version!=2) {
    static int printerror=1;
    if(printerror)
      pd_error(x, "cannot parse RTP-header v%d", version);
    printerror=0;
  }

  SETFLOAT(ap+0, rtp->p);
  outlet_anything(out, SELECTOR_RTPHEADER_P, 1, ap);
  SETFLOAT(ap+0, rtp->x);
  outlet_anything(out, SELECTOR_RTPHEADER_X, 1, ap);
  SETFLOAT(ap+0, rtp->cc);
  outlet_anything(out, SELECTOR_RTPHEADER_CC, 1, ap);
  SETFLOAT(ap+0, rtp->m);
  outlet_anything(out, SELECTOR_RTPHEADER_M, 1, ap);
  SETFLOAT(ap+0, rtp->pt);
  outlet_anything(out, SELECTOR_RTPHEADER_PT, 1, ap);
  SETFLOAT(ap+0, rtp->seq);
  outlet_anything(out, SELECTOR_RTPHEADER_SEQ, 1, ap);
  SETUINT32(ap, rtp->ts);
  outlet_anything(out, SELECTOR_RTPHEADER_TS, 2, ap);
  SETUINT32(ap, rtp->ssrc);
  outlet_anything(out, SELECTOR_RTPHEADER_SSRC, 2, ap);
  for(c=0; c<cc; c++) {
    SETFLOAT(ap+0, c);
    SETUINT32(ap+1, rtp->csrc[c]);
    outlet_anything(out, SELECTOR_RTPHEADER_CSRC, 3, ap);
  }
}

static void rtpparse_list(t_rtpparse*x, t_symbol*s, int argc, t_atom*argv){
  int result=atoms2header(argc, argv, &x->x_rtpheader);
  if(result>0) {
    rtpparse_bang(x);
    outlet_list(x->x_dataout, s, argc-result, argv+result);
  } else {
    outlet_list(x->x_rejectout, s, argc, argv);
  }
    }

/* create rtpparse with args <channels> <skip> */
static void *rtpparse_new(void)
{
	t_rtpparse *x = (t_rtpparse *)pd_new(rtpparse_class);

	x->x_dataout=outlet_new(&x->x_obj, &s_list);
	x->x_infoout=outlet_new(&x->x_obj, 0);
	x->x_rejectout=outlet_new(&x->x_obj, 0);
	return (x);
}



static void rtpparse_free(t_rtpparse *x) {
  rtpheader_freemembers(&x->x_rtpheader);

  outlet_free(x->x_dataout);
  outlet_free(x->x_infoout);
  outlet_free(x->x_rejectout);
}

void rtpparse_setup(void)
{
	rtpparse_class = class_new(gensym("rtpparse"), (t_newmethod)rtpparse_new, (t_method)rtpparse_free,
		sizeof(t_rtpparse), 0,0);

	class_addlist(rtpparse_class, (t_method)rtpparse_list);
}
