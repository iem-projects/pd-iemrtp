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
#include "iemrtp.h"

static t_class *L16pay_class;
typedef struct _L16pay
{
	t_rtppay x_obj;
} t_L16pay;


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

/* create L16pay with args <channels> <skip> */
static void *L16pay_new(t_floatarg fchan)
{
  int ichan = fchan;
	t_L16pay *x = (t_L16pay *)pd_new(L16pay_class);
  return iemrtp_rtppay_new(&x->x_obj, ichan, 2, L16_perform);
}



static void L16pay_free(t_L16pay *x) {
  iemrtp_rtppay_free(&x->x_obj);
}

void L16pay_tilde_setup(void)
{
	L16pay_class = class_new(gensym("L16pay~"), (t_newmethod)L16pay_new, (t_method)L16pay_free,
		sizeof(t_L16pay), 0, A_DEFFLOAT,0);
  iemrtp_rtppay_classnew(L16pay_class);
}
