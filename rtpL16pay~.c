/*
 * rtpL16pay: RTP-playloader for L16
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

static t_class *rtpL16pay_class;
typedef struct _rtpL16pay
{
  t_rtppay x_obj;
} t_rtpL16pay;


/* ******************************************************************************** */
/*                          the work                krow eht                        */
/* ******************************************************************************** */

static void L16_perform(u_int32 vecsize, u_int32 channels, t_sample**ins, u_int8*buffer)
{
  const t_sample scale = 0x7FFF / 1.f;
  u_int32 n, c;

  for(n=0; n<vecsize; n++) {
    for (c=0;c<channels;c++) {
      // LATER: if(in>1.f) this returns "-1"
      signed short s = ins[c][n] * scale;
      *buffer++=(s>>8) & 0xFF;
      *buffer++=(s>>0) & 0xFF;
    }
  }
}

/* create rtpL16pay with args <channels> */
static void *rtpL16pay_new(t_symbol*s, int argc, t_atom*argv)
{
  t_rtpL16pay *x = (t_rtpL16pay *)pd_new(rtpL16pay_class);
  return iemrtp_rtppay_new(&x->x_obj, s,
                           2, L16_perform,
                           argc, argv);
}



static void rtpL16pay_free(t_rtpL16pay *x) {
  iemrtp_rtppay_free(&x->x_obj);
}

void rtpL16pay_tilde_setup(void)
{
  rtpL16pay_class = class_new(gensym("rtpL16pay~"),
                              (t_newmethod)rtpL16pay_new, (t_method)rtpL16pay_free,
                              sizeof(t_rtpL16pay), 0,
                              A_GIMME,0);
  iemrtp_rtppay_classnew(rtpL16pay_class);
}
