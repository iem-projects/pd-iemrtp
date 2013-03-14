/*
 * iemrtp: RTP support for Pd (common functionality)
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

u_int32 iemrtp_rtpheader2atoms(t_rtpheader*rtpheader, t_atom*ap0) {
  t_atom*ap=ap0;
  u_int8 b;
  u_int32 c, cc=rtpheader->cc;
  b=(rtpheader->version << 6) | (rtpheader->p << 5) | (rtpheader->x << 4) | (rtpheader->cc << 0);
  ap++->a_w.w_float=b;

  b=(rtpheader->m << 7) | (rtpheader->pt << 0);
  ap++->a_w.w_float=b;

  ap+=atombytes_setU16(rtpheader->seq, ap);
  ap+=atombytes_setU32(rtpheader->ts , ap);
  ap+=atombytes_setU32(rtpheader->ssrc,ap);
  for(c=0; c < cc; c++) {
    ap+=atombytes_setU32(rtpheader->csrc[c], ap);
  }
  return (12+cc*4);
}


void iemrtp_rtpheader_freemembers(t_rtpheader*rtpheader) {
 if(rtpheader->csrc)free(rtpheader->csrc);
 rtpheader->csrc=NULL;
 rtpheader->cc=0;
}
int iemrtp_rtpheader_ensureCSRC(t_rtpheader*rtpheader, int size) {
  u_int32*csrc = NULL;
  int i;
  if(size>0x0F || size < 0)return 0; /* invalid size */
  if(size<=rtpheader->cc  )return size; /* already large enough */
  csrc = calloc(size, sizeof(u_int32));
  if(!csrc)return 0;

  for(i=0; i<rtpheader->cc; i++) {
    csrc[i]=rtpheader->csrc[i];
  }
  iemrtp_rtpheader_freemembers(rtpheader);
  rtpheader->csrc = csrc;
  rtpheader->cc   = size;
  return rtpheader->cc;
}


int iemrtp_atoms2rtpheader(int argc, t_atom*argv, t_rtpheader*rtpheader) {
  u_int8  b;
  u_int8 cc;
  int retval=12;

  if(!argc) {
    return -retval;
  }

  b=atom_getint(argv+0);
  cc=(b >> 0) & 0x0F;
  retval=12+cc*4;
  if(argc<retval) {
    return -retval;
  }
  iemrtp_rtpheader_freemembers(rtpheader);
  rtpheader->csrc=malloc(cc * sizeof(u_int32));

  rtpheader->version = (b >> 6) & 0x03;
  rtpheader->p       = (b >> 5) & 0x01;
  rtpheader->x       = (b >> 4) & 0x01;
  rtpheader->cc      = cc;

  b=atom_getint(argv+1);
  rtpheader->m       = (b >> 7) & 0x01;
  rtpheader->pt      = (b >> 0) & 0x7F;

  rtpheader->seq  =atombytes_getU16(argv+2);
  rtpheader->ts   =atombytes_getU32(argv+4);
  rtpheader->ssrc =atombytes_getU32(argv+8);

  for(b=0; b<cc; b++) {
    rtpheader->csrc[b]=atombytes_getU32(argv+12+4*b);
  }
  return retval;
}
