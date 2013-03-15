/*
 * L16decode: decode L16 packet into de-interleaved sample data
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

static t_class *L16decode_class;

typedef struct _L16decode
{
  t_object x_obj;
  u_int32 x_channels;  // number of channels

  /* buffer for outputting samples */
  t_atom*x_atombuffer;
  u_int32 x_atombuffersize;
} t_L16decode;

static void L16decode_list(t_L16decode*x, t_symbol*s, int argc, t_atom*argv) {
  u_int32 c, channels=x->x_channels;
  u_int32 framesize=(RTP_BYTESPERSAMPLE*channels);
  u_int32 f, frames=(argc / framesize) ; // 1 extra frame for channel ID
  u_int32 i;

  /* setup output buffer */
  if(x->x_atombuffersize<frames*framesize+1) {
    freebytes(x->x_atombuffer, x->x_atombuffersize*sizeof(t_atom));
    x->x_atombuffersize = ((frames+1)*framesize / 64 + 1) * 64;
    x->x_atombuffer=getbytes(x->x_atombuffersize*sizeof(t_atom));
    for(i=0; i<x->x_atombuffersize; i++)
      SETFLOAT(x->x_atombuffer+i, 0.);
  }

  c=0;
  for(f=0; f<frames; f++) {
    for(c=0; c<channels; c++) {
      unsigned char hibyte=atom_getint(argv++);
      unsigned char lobyte=atom_getint(argv++);
      signed short s = (hibyte<<8) + lobyte;
      t_sample fsample=s/32767.;
      u_int32 index=c*frames+f+1;
      //      post("index=%d=%d*(%d-1)+%d+1",index, c, frames, f);
      //      post("sample[%d/%d]= %d [%d, %d]\t --> [%d] %f", f, c, s, hibyte, lobyte, index, fsample);
      x->x_atombuffer[index].a_w.w_float = fsample;
    }
  }

  /* output sample lists */
  for(i=0; i<channels; i++) {
    t_atom*ap=x->x_atombuffer+frames*i;
    SETFLOAT(ap, i);
    outlet_list(x->x_obj.ob_outlet, s, frames+1, ap);
  }
}

static void L16decode_channels(t_L16decode *x, t_float c) {
  int i=c;
  if(i>0)
  x->x_channels = i;
}

/* create L16decode with args <channels> <skip> */
static void *L16decode_new(t_floatarg fchan)
{
  t_L16decode *x = (t_L16decode *)pd_new(L16decode_class);
  int ichan = fchan;
  if(ichan < 1)
    ichan = 2;

  x->x_channels = ichan;
  outlet_new(&x->x_obj, gensym("list"));

  return (x);
}



static void L16decode_free(t_L16decode *x) {
  if(x->x_atombuffer) freebytes(x->x_atombuffer, x->x_atombuffersize * sizeof(*(x->x_atombuffer)));
}

void L16decode_setup(void)
{
  L16decode_class = class_new(gensym("L16decode"), (t_newmethod)L16decode_new, (t_method)L16decode_free,
    sizeof(t_L16decode), 0, A_DEFFLOAT,0);

  class_addmethod(L16decode_class, (t_method)L16decode_channels , gensym("channels" ), A_FLOAT, 0);
  class_addlist  (L16decode_class, (t_method)L16decode_list);
}
