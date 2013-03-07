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
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)
#error No byte order defined
#endif

#define RTP_HEADERSIZE 13

static t_class *L16pay_class;

typedef struct _L16pay
{
	t_object x_obj;
  t_sample**x_in;
  unsigned int x_channels;
  unsigned int x_vecsize;
  unsigned int x_mtu;



  unsigned int x_packetsize;

  t_atom*x_buffer;
  unsigned int x_buffersize;
} t_L16pay;

static void L16pay_MTU(t_L16pay *x, t_floatarg f)
{
	int t = f;
  if(f<RTP_HEADERSIZE) {
    pd_error(x, "MTU-size (%d) must not be smaller than %d", t, RTP_HEADERSIZE);
  } else {
    x->x_mtu = t;
  }
}
static void L16pay_preparePacket(t_L16pay*x) {
  unsigned int channels = x->x_channels;
  unsigned int mtu      = x->x_mtu;
  unsigned int vecsize  = x->x_vecsize;

  unsigned int payload=channels * vecsize * 2; // number of bytes in a single block
  unsigned int packetcount = (payload / (mtu - (RTP_HEADERSIZE + 100 ))) + 1;

  unsigned int packetsize = RTP_HEADERSIZE+ (payload/packetcount);

  post("L16pay: channels=%d/%d @ %d", channels, vecsize, mtu);
  post("\tpayload: %d", payload);
  post("\tpackets: %d*%d", packetcount, packetsize);

  x->x_packetsize = packetsize;

  if(x->x_buffersize < (packetsize*packetcount)) {
    if(x->x_buffer)
      freebytes(x->x_buffer, x->x_buffersize * sizeof(t_atom));
    x->x_buffersize = packetsize * packetcount;
    x->x_buffer = getbytes(x->x_buffersize * sizeof(t_atom));
  }
  post("\tbufsize: %d", x->x_buffersize);

}


/* ******************************************************************************** */
/*                          the work                krow eht                        */
/* ******************************************************************************** */


static t_int *L16pay_perform(t_int *w)
{
	t_L16pay* x = (t_L16pay*)(w[1]);
  int vecsize = x->x_vecsize;
  const t_sample scale = 32767.;
  unsigned int i;
  unsigned int c=x->x_channels;
	for (i=0;i<c;i++) {
		t_sample*in = x->x_in[i];
    int n;
    for(n=0; n<vecsize; n++) {
      short out;
      int s = in[n] * scale;
      if      (s > 32767) s = 32767;
      else if (s <-32767) s =-32767;
      /* convert to big-endian */
#ifdef BIG_ENDIAN
      out=s;
#else
      tmp=(s << 8) | (s >> 8);
#endif
      

    }
  }
	return(w+2);
}


/* ---------------------- Setup junk -------------------------- */

static void L16pay_dsp(t_L16pay *x, t_signal **sp)
{
  int n=sp[0]->s_n, i;

  x->x_vecsize=n;

  for(i=0; i<n; i++) {
    x->x_in[i]=sp[i];
  }
  

  dsp_add(L16pay_perform, 1, x);
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
  x->x_vecsize  = 64;
  x->x_mtu      = 1500;
  x->x_buffer   = NULL;
  x->x_buffersize=0;

  L16pay_preparePacket(x);


	while (--c) {
		inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal")); /* channels inlet */
	}
	outlet_new(&x->x_obj, gensym("list"));



	return (x);
}



static void L16pay_free(t_L16pay *x)
{

}

void L16pay_setup(void)
{
	L16pay_class = class_new(gensym("L16pay"), (t_newmethod)L16pay_new, (t_method)L16pay_free,
		sizeof(t_L16pay), 0, A_DEFFLOAT,0);
	class_addmethod(L16pay_class, nullfn, gensym("signal"), 0);
	class_addmethod(L16pay_class, (t_method)L16pay_dsp, gensym("dsp"), 0);

	class_addmethod(L16pay_class, (t_method)L16pay_MTU, gensym("mtu"), A_FLOAT, 0);
}
