/*
 * packRTCP: parse RTCP packages to get some info
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
#include <stdlib.h>
#include <string.h>

static t_class *packRTCP_class;

typedef struct _packRTCP
{
  t_object x_obj;
  t_outlet*x_outlet;
  rtcp_t x_rtcp;

  t_atom *x_buffer;
  u_int32 x_buffersize;
} t_packRTCP;



static void packRTCP_bang(t_packRTCP *x) {
  int result = 0;
  result = iemrtp_rtcp2atoms(&x->x_rtcp, x->x_buffersize, x->x_buffer);
  if(result<0) {
    u_int32 i;
    freebytes(x->x_buffer, x->x_buffersize*sizeof(t_atom));
    x->x_buffersize=-2*result;
    x->x_buffer = getbytes(x->x_buffersize*sizeof(t_atom));
    for(i=0; i< x->x_buffersize ; i++) SETFLOAT(x->x_buffer+i, 0.);

    result = iemrtp_rtcp2atoms(&x->x_rtcp, x->x_buffersize, x->x_buffer);
  }
  if(result>0) {
    outlet_list(x->x_outlet, 0, result, x->x_buffer);
  } else {
    pd_error(x, "unable to convert RTCP struct to %d atoms", -result);
  }
}


static void packRTCP_version(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int version = atom_getint(argv);
    if(version != 2) pd_error(x, "currently only version '2' is supported!");
    x->x_rtcp.common.version = 2;
  } else post("%s: %d", s->s_name, x->x_rtcp.common.version);
}
static void packRTCP_p(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int p = atom_getint(argv);
    if(p != 0) pd_error(x, "currently only padding '0' is supported!");
    x->x_rtcp.common.p = 0;
  }  else     post("'%s': %d", s->s_name, x->x_rtcp.common.p);
}
static void packRTCP_pt(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  if(argc) {
    int pt = 0;
    switch(argv->a_type) {
    case A_FLOAT:
      pt=atom_getint(argv);
      break;
    case A_SYMBOL:
      s=atom_getsymbol(argv);
      if     ( SELECTOR_RTCP_SR == s)   pt = RTCP_SR;
      else if( SELECTOR_RTCP_RR == s)   pt = RTCP_RR;
      else if( SELECTOR_RTCP_SDES == s) pt = RTCP_SDES;
      else if( SELECTOR_RTCP_BYE == s)  pt = RTCP_BYE;
      else if( SELECTOR_RTCP_APP == s)  pt = RTCP_APP;
      else {
        pd_error(x, "invalid type specifier '%s'", s->s_name);
        return;
      }
      break;
    default:
      pd_error(x, "invalid type specifier");
      return;
    }
    switch(pt) {
    case(RTCP_SR):
    case(RTCP_RR):
    case(RTCP_SDES):
    case(RTCP_BYE):
      break;
    case(RTCP_APP):
      pd_error(x, "'app' type currently unsupported");
      return;
    default:
      pd_error(x, "invalid type specifier %d", pt);
      return;
    }
    x->x_rtcp.common.pt = pt;
  }  else     post("pt: %d", x->x_rtcp.common.pt);
}
static void packRTCP_app(t_packRTCP *x, t_symbol*s, int UNUSED(argc), t_atom* UNUSED(argv)) {
  pd_error(x, "'%s' type currently unsupported", s->s_name);
}
static void packRTCP_count(t_packRTCP *x, t_symbol*s, int argc, t_atom* argv) {
  rtcp_t*rtcp=&x->x_rtcp;
  switch(rtcp->common.pt) {
  case(RTCP_RTPFB): case(RTCP_PSFB): if(argc) {
    int pt=atom_getint(argv);
    if(pt>30) {
      pd_error(x, "'%s': invalid format %d", s->s_name, pt);
      return;
    }
    rtcp->common.subtype=pt;
#warning FIXME - check whether the argument is valid
  } else {
    post("'%s': %d", rtcp->common.subtype);
  }
    break;
  default:
    pd_error(x, "'%s' only supported for RTPFB/PSFB packets", s->s_name);
  }
}

int setRR(rtcp_rr_t*rr, int argc, t_atom*argv) {
  t_symbol*s;
  if(argc<2)return 0;
  s=atom_getsymbol(argv);
  if(0) { }
  else if(SELECTOR_RTCP_RR_SSRC     == s) rr->ssrc    =GETUINT32(argc-1, argv+1);
  else if(SELECTOR_RTCP_RR_FRACTION == s) rr->fraction=atom_getint(argv+1);
  else if(SELECTOR_RTCP_RR_LOST     == s) rr->lost    =GETUINT32(argc-1, argv+1);
  else if(SELECTOR_RTCP_RR_LAST_SEQ == s) rr->last_seq=GETUINT32(argc-1, argv+1);
  else if(SELECTOR_RTCP_RR_JITTER   == s) rr->jitter  =GETUINT32(argc-1, argv+1);
  else if(SELECTOR_RTCP_RR_LSR      == s) rr->lsr     =GETUINT32(argc-1, argv+1);
  else if(SELECTOR_RTCP_RR_DLSR     == s) rr->dlsr    =GETUINT32(argc-1, argv+1);
  else return 0;
  return 1;
}
int setSDES(rtcp_sdes_item_t*item, int argc, t_atom*argv) {
  t_symbol*s;
  rtcp_type_t typ=0;
  free(item->data);
  item->data=NULL;
  item->length=0;
  item->type  =0;
  if(argc<1)return 1; // deleted entry

  s=atom_getsymbol(argv);
  if(0) {}
  else if(SELECTOR_RTCP_SDES_END   == s) typ=RTCP_SDES_END;
  else if(SELECTOR_RTCP_SDES_CNAME == s) typ=RTCP_SDES_CNAME;
  else if(SELECTOR_RTCP_SDES_NAME  == s) typ=RTCP_SDES_NAME;
  else if(SELECTOR_RTCP_SDES_EMAIL == s) typ=RTCP_SDES_EMAIL;
  else if(SELECTOR_RTCP_SDES_PHONE == s) typ=RTCP_SDES_PHONE;
  else if(SELECTOR_RTCP_SDES_LOC   == s) typ=RTCP_SDES_LOC;
  else if(SELECTOR_RTCP_SDES_TOOL  == s) typ=RTCP_SDES_TOOL;
  else if(SELECTOR_RTCP_SDES_NOTE  == s) typ=RTCP_SDES_NOTE;
  else if(SELECTOR_RTCP_SDES_PRIV  == s) typ=RTCP_SDES_PRIV;
  if(0==typ)return 0;

  item->type = typ;
  if(argc>1) {
    s=atom_getsymbol(argv+1);
    item->data=strndup(s->s_name, 255);
    item->length=strlen(item->data);
  }
  return 1;
}

int setNACK(rtcp_rtpfb_nack_t*nack, int argc, t_atom*argv) {
  if(argc<2)return 0;
  nack->pid = atom_getintarg(0, argc, argv);
  nack->blp = atom_getintarg(1, argc, argv);
  return 1;
}


static void packRTCP_sr(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_SR);

  if(argc>2) {
    t_symbol*s1=atom_getsymbol(argv);
    if(SELECTOR_RTCP_SR_SSRC==s1) {
      x->x_rtcp.r.sr.ssrc=GETUINT32(argc-1, argv+1);
    } else if(SELECTOR_RTCP_SR_NTP  == s1) {
      if(argc<5) {
        pd_error(x, "usage: %s %s <SEChi> <SEClo> <FRAChi> <FRAClo>", s->s_name, s1->s_name);
        return;
      }
      x->x_rtcp.r.sr.ntp_sec =GETUINT32(2, argv+1);
      x->x_rtcp.r.sr.ntp_frac=GETUINT32(2, argv+3);
    } else if(SELECTOR_RTCP_SR_TS   == s1) {
      x->x_rtcp.r.sr.rtp_ts=GETUINT32(argc-1, argv+1);
    } else if(SELECTOR_RTCP_SR_PSENT== s1) {
      x->x_rtcp.r.sr.psent=GETUINT32(argc-1, argv+1);
    } else if(SELECTOR_RTCP_SR_OSENT== s1) {
      x->x_rtcp.r.sr.osent=GETUINT32(argc-1, argv+1);
    } else if(A_FLOAT==argv->a_type) { // 'SR <id> <VALhi> <VALlo>'
      int index=atom_getint(argv);
      if(argc<3) {
        pd_error(x, "usage: %s <#> <type> <VALhi> <VALlo>...", s->s_name);
        return;
      }
      if(iemrtp_rtcp_ensureSR(&x->x_rtcp, index+1) && setRR(x->x_rtcp.r.sr.rr+index, argc-1, argv+1)) {
      } else {
        pd_error(x, "unable to set %s/RR @ %d", s->s_name, index);
        return;
      }
    } else {
      pd_error(x, "invalid field-id '%s'/'%s'", s->s_name, s1->s_name);
      return;
    }
    return;
  }
  pd_error(x, "syntax: %s <field> <VALhi> <VALlo>", s->s_name);
}
static void packRTCP_rr(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_RR);

  if(argc>2) {
    t_symbol*s1=atom_getsymbol(argv);
    if(SELECTOR_RTCP_RR_SSRC==s1) {
      x->x_rtcp.r.rr.ssrc=GETUINT32(argc-1, argv+1);
    } else if(A_FLOAT==argv->a_type) { // 'RR <id> <VALhi> <VALlo>'
      int index;
      if(argc<3) {
        pd_error(x, "usage: %s <#> <type> <VALhi> <VALlo>...", s->s_name);
        return;
      }
      index=atom_getint(argv);
      if(iemrtp_rtcp_ensureRR(&x->x_rtcp, index+1) && setRR(x->x_rtcp.r.rr.rr+index, argc-1, argv+1)) {
      } else {
        pd_error(x, "unable to set %s/RR @ %d", s->s_name, index);
        return;
      }
    } else {
      pd_error(x, "invalid field-id '%s'", s->s_name);
      return;
    }
    return;
  }
  pd_error(x, "syntax: %s <field> <VALhi> <VALlo>", s->s_name);
}

static void packRTCP_sdes(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_SDES);
  if(argc>2) {
    t_symbol*s1=atom_getsymbol(argv);
    if(SELECTOR_RTCP_SDES_SRC==s1) {
      x->x_rtcp.r.sdes.src=GETUINT32(argc-1, argv+1);
    } else if(A_FLOAT == argv->a_type) { // 'SDES <id> <type> <string>'
      int index = atom_getint(argv+1);

      if(argc<3) {
        pd_error(x, "usage: %s <#> <type> <VALhi> <VALlo>...", s->s_name);
        return;
      }
      if(iemrtp_rtcp_ensureSDES(&x->x_rtcp, index+1) && setSDES(x->x_rtcp.r.sdes.item+index, argc-1, argv+1)) {
      } else {
        pd_error(x, "unable to set %s/ITEM @ %d", s->s_name, index);
        return;
      }
    } else {

      pd_error(x, "invalid field-id '%s'", s->s_name);
      return;
    }
    return;
  }
  pd_error(x, "syntax: %s <field> <VALhi> <VALlo>", s->s_name);
}

static void packRTCP_bye(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_BYE);

  if(argc==3) { // <index> <hi> <lo>
    u_int32 index = atom_getint(argv+0);
    u_int32    id = GETUINT32(argc-1, argv+1);
    if(iemrtp_rtcp_ensureBYE(&x->x_rtcp, index+1))
      x->x_rtcp.r.bye.src[index]=id;
    else
      pd_error(x, "unable to write BYE to index %d", index);
  } else if (argc) {
    pd_error(x, "syntax: %s <index> <SRChi> <SRClo>", s->s_name);
  }
}

static int packRTCP_setFBSSRC(rtcp_t*x, int argc, t_atom*argv) {
  rtcp_common_fbsrc_t*fbsrc = NULL;
  t_symbol*s;
  u_int32 ssrc;

  if(argc<3)return 0;
  switch(x->common.pt) {
  case(RTCP_RTPFB):fbsrc= &x->r.rtpfb.ssrc;break;
  case(RTCP_PSFB ):fbsrc= &x->r.psfb .ssrc;break;
  default: return 0;
  }
  s=atom_getsymbol(argv);
  ssrc = GETUINT32(argc-1, argv+1);

  if (SELECTOR_RTCP_RTPFB_SENDER_SSRC == s)     fbsrc->sender=ssrc;
  else if (SELECTOR_RTCP_RTPFB_MEDIA_SSRC == s) fbsrc->media =ssrc;
  else return 0;

  return 1;
}

static void packRTCP_rtpfb(t_packRTCP *x, t_symbol*s0, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_RTPFB);
  if(argc>0) {
    const t_symbol*s1=atom_getsymbol(argv);
    if(packRTCP_setFBSSRC(&x->x_rtcp, argc, argv))return;
    argv++; argc--;
    if(SELECTOR_RTCP_RTPFB_NACK == s1) {
      if(3==argc) {
        int index=atom_getint(argv+0);
        if(iemrtp_rtcp_ensureNACK(&x->x_rtcp, index+1) && setNACK(x->x_rtcp.r.rtpfb.nack.nack+index, argc-1, argv+1))
          return;
        pd_error(x, "%s/%s index (%d) must not be larger than %d",
                 s0->s_name, s1->s_name,
                 index, MAX_RTPFB_NACK_COUNT);
        return;
      }
      pd_error(x, "syntax: %s %s <index> <pid> <blp>", s0->s_name, s1->s_name);
      return;
    }
  }
  pd_error(x, "syntax: %s <field> <VAL>...", s0->s_name);
}
static void packRTCP_psfb(t_packRTCP *x, t_symbol*s, int argc, t_atom*argv) {
  iemrtp_rtcp_changetype(&x->x_rtcp, RTCP_PSFB);
  if(argc>0) {
    const t_symbol*s1=atom_getsymbol(argv);
    int typ=iemrtp_rtcp_atom2psfbtype(argv);
    if(typ<0) {
      pd_error(x, "invalid PSFB type '%s'", s1->s_name);
    }
    iemrtp_rtcp_psfb_changetype(&x->x_rtcp, typ);

    if(packRTCP_setFBSSRC(&x->x_rtcp, argc, argv))return;
    argv++; argc--;
    switch(typ) {
    case RTCP_PSFB_PLI : x->x_rtcp.common.subtype = typ; break;
    case RTCP_PSFB_SLI : x->x_rtcp.common.subtype = typ;
      /* syntax: PSFB SLI <index> <first> <number> <pictureid> */
      if((argc != 4)
         || (A_FLOAT != argv[0].a_type)
         || (A_FLOAT != argv[1].a_type)
         || (A_FLOAT != argv[2].a_type)
         || (A_FLOAT != argv[3].a_type)) {
        pd_error(x, "syntax: %s %s <index> <first> <number> <PicutreID>", s->s_name, s1->s_name);
      } else {
        int index=atom_getint(argv+0);
        if(iemrtp_rtcp_ensureSLI(&x->x_rtcp, index+1)) {
          x->x_rtcp.r.psfb.psfb.sli.sli[index].first     = atom_getint(argv+1);
          x->x_rtcp.r.psfb.psfb.sli.sli[index].number    = atom_getint(argv+2);
          x->x_rtcp.r.psfb.psfb.sli.sli[index].pictureid = atom_getint(argv+3);
        }
      }
      break;
    case RTCP_PSFB_RPSI: x->x_rtcp.common.subtype = typ;
    case RTCP_PSFB_AFB : x->x_rtcp.common.subtype = typ;
    default:
      pd_error(x, "invalid field-type for '%s'", s->s_name);
    }

  } else
  pd_error(x, "syntax: %s <field> <VALhi> <VALlo>", s->s_name);
}
static t_packRTCP *packRTCP_new(void)
{
  t_packRTCP *x = (t_packRTCP *)pd_new(packRTCP_class);

  x->x_rtcp.common.version = 2;
  x->x_rtcp.common.pt = RTCP_BYE;

  x->x_outlet=outlet_new(&x->x_obj, &s_float);
  return (x);
}



static void packRTCP_free(t_packRTCP *x) {
  iemrtp_rtcp_freemembers(&x->x_rtcp);
  outlet_free(x->x_outlet);
}

void packRTCP_setup(void)
{
  packRTCP_class = class_new(gensym("packRTCP"), (t_newmethod)packRTCP_new, (t_method)packRTCP_free,
    sizeof(t_packRTCP), 0,0);

  class_addbang  (packRTCP_class, (t_method)packRTCP_bang);
  class_addmethod(packRTCP_class, (t_method)packRTCP_version, SELECTOR_RTCP_HEADER_VERSION, A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_p,       SELECTOR_RTCP_HEADER_P,       A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_pt,      SELECTOR_RTCP_HEADER_TYPE,    A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_count,   SELECTOR_RTCP_HEADER_COUNT,   A_GIMME, 0);

  class_addmethod(packRTCP_class, (t_method)packRTCP_sr,      SELECTOR_RTCP_SR,      A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_rr,      SELECTOR_RTCP_RR,      A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_sdes,    SELECTOR_RTCP_SDES,    A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_bye,     SELECTOR_RTCP_BYE,     A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_app,     SELECTOR_RTCP_APP,     A_GIMME, 0);

  class_addmethod(packRTCP_class, (t_method)packRTCP_rtpfb,   SELECTOR_RTCP_RTPFB,   A_GIMME, 0);
  class_addmethod(packRTCP_class, (t_method)packRTCP_psfb,    SELECTOR_RTCP_PSFB,    A_GIMME, 0);
}
