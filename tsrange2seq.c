/*
 * tsrange2seq: check which seq-numbers cover a given timestamp-range
 *              (and seq-numbers are missing)
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

static u_int32 seqwrap(u_int32 oldseq, unsigned short newseq) {
  unsigned short o16seq = oldseq & 0xFFFF;
  u_int32 epoch = (oldseq >> 16 ) & 0xFFFF;
  if(o16seq>newseq && (o16seq-newseq)>0x7FFF){
    // wrap around, move to new epoch
    epoch++;
  } else if (newseq>o16seq && (newseq-o16seq)>0x7FFF){
    // wrap around backwards, move to prior epoch
    if(epoch>0)
      epoch--;
  }
  return (epoch<<16)+newseq;
}

typedef struct tsrange_ {
  u_int32 seq;
  u_int32 start;
  u_int32 stop;
  struct tsrange_*next;
  struct tsrange_*prev;
} tsrange_t;

static tsrange_t*tsrange_create(u_int32 seq, u_int32 start, u_int32 stop) {
  tsrange_t*result=(tsrange_t*)calloc(1, sizeof(tsrange_t));
  result->seq   = seq;
  result->start = start;
  result->stop  = stop;
  result->next  = NULL;
  result->prev  = NULL;

  return result;
}
static void tsrange_destroy(tsrange_t*t) {
  t->seq   = 0;
  t->start = 0;
  t->stop  = 0;
  t->next  = NULL;
  t->prev  = NULL;
  free(t);
}
/* deletes all elements from (and including) t */
static u_int32 tsrange_delFrom(tsrange_t*t) {
  u_int32 count=0;
  tsrange_t*next=t;

  if(t && t->prev)
    t->prev->next=NULL;
  while(t) {
    next=t->next;
    tsrange_destroy(t);
    t=next;
    count++;
  }
  return count;
}
/* deletes all elements until (and including) t */
static u_int32 tsrange_delTill(tsrange_t*t) {
  u_int32 count=0;
  tsrange_t*prev=t;

  if(t && t->next)
    t->next->prev=NULL;
  while(t) {
    prev=t->prev;
    tsrange_destroy(t);
    t=prev;
    count++;
  }
  return count;
}


/* adds a new range-list just after the 'rangelist' element */
static tsrange_t*tsrange_addTail(tsrange_t*rangelist, tsrange_t*tail) {
  tsrange_t*prev=tail, *next=tail;

  if(!tail)   return rangelist;
  if(!rangelist)return tail;

  /* find start/end of range to be inserted */
  while(prev->prev)prev=prev->prev;
  while(next->next)next=next->next;

  /* insert the prev/next after the rangelist-element */
  next->next=rangelist->next;

  if(rangelist->next)rangelist->next->prev=next;

  prev->prev=rangelist;
  rangelist->next=prev;

  return rangelist;
}

/* adds a new range-list just before the 'rangelist' element */
static tsrange_t*tsrange_addHead(tsrange_t*rangelist, tsrange_t*head) {
  tsrange_t*prev=head, *next=head;

  if(!head)   return rangelist;
  if(!rangelist)return head;

  /* find start/end of range to be inserted */
  while(prev->prev)prev=prev->prev;
  while(next->next)next=next->next;

  /* insert the prev/next before the rangelist-element */
  prev->prev=rangelist->prev;

  if(rangelist->prev)rangelist->prev->next=prev;

  next->next=rangelist;
  rangelist->prev=next;

  return rangelist;
}

static tsrange_t*tsrange_first(tsrange_t*rangelist) {
  while(rangelist->prev)
    rangelist=rangelist->prev;
  return rangelist;
}
static tsrange_t*tsrange_last(tsrange_t*rangelist) {
  while(rangelist->next)
    rangelist=rangelist->next;
  return rangelist;
}

static int tsrange_match(tsrange_t*t, u_int32 start, u_int32 stop) {
  return (t->start<=stop && start<=t->stop);
}

typedef struct _tsrange2seq {
  t_object x_obj;
  t_outlet*x_haveout;
  t_outlet*x_missout;

  tsrange_t*x_start;
  tsrange_t*x_stop;

  u_int32   x_lastseq; /* for period detection */
} t_tsrange2seq;

static void tsrange2seq_out(t_outlet*out, t_symbol*s, tsrange_t*ts) {
  t_atom ap[4];
  const u_int32 start=ts->start;
  const u_int32 stop =ts->stop ;
  const unsigned short TShi = (start >> 16) & 0xFFFF;
  const unsigned short TSlo = (start >>  0) & 0xFFFF;
  const u_int32 dur = stop-start;
  const u_int32 seq = (ts->seq) & 0xFFFF;
  //post("%d: %d..%d", seq, start, stop);

  SETFLOAT(ap+0, seq );
  SETFLOAT(ap+1, TShi);
  SETFLOAT(ap+2, TSlo);
  SETFLOAT(ap+3, dur );
  outlet_anything(out, s, 4, ap);

}

static int tsrange2seq_docheck(t_tsrange2seq *x, const u_int32 start, const u_int32 stop) {
  /* find all seqs that match start..stop */
  tsrange_t*ts;
  //post("CHECK: %d..%d", start, stop);
  if(!x->x_start)return 0;
  if(x->x_start->start > stop)return 0;

  for(ts=x->x_start;ts; ts=ts->next) {
    if(tsrange_match(ts, start, stop)) {
      u_int32 seq=ts->seq;
      for(ts=ts->next; ts && stop>=ts->stop; ts=ts->next) {
        for(seq=seq+1;seq<ts->seq; seq++) {
          outlet_float(x->x_missout, seq);
        }
        if(tsrange_match(ts, start, stop)) {
          seq=ts->seq;
          tsrange2seq_out(x->x_haveout, gensym("list"), ts);
        }
      }
      return 1;
    }
  }
  return 0;
}
static void tsrange2seq_check(t_tsrange2seq *x, t_symbol*s, int argc, t_atom*argv) {
  u_int32 start, stop;
  if(argc<2) {
    pd_error("usage: '%s <TS0hi> <TS0lo> [<TS1hi> <TS1lo>]", s->s_name);
    return;
  }
  start=GETUINT32(2, argv);
  stop=(argc>2)?GETUINT32(argc-2, argv+2):start;
  tsrange2seq_docheck(x, start, stop);
}

static void tsrange2seq_clear(t_tsrange2seq *x) {
  if(x->x_start)tsrange_delFrom(x->x_start);
  x->x_start = x->x_stop = NULL;
}
static void tsrange2seq_dump(t_tsrange2seq *x) {
  tsrange_t*ts;
  for(ts=x->x_start;ts; ts=ts->next) {
    tsrange2seq_out(x->x_haveout, gensym("dump"), ts);
  }
}

static void tsrange2seq_add(t_tsrange2seq *x,
                            t_float f_seq,
                            t_float f_TShi, t_float f_TSlo,
                            t_float f_dur) {
  const u_int16 seq16= f_seq;
  const u_int32 TShi = f_TShi;
  const u_int32 TSlo = f_TSlo;
  const u_int32 TS0  = (TShi<<16)+TSlo;
  const u_int16 dur  = f_dur;
  const u_int32 TS1  = TS0+dur;

  const u_int32 seq  = seqwrap(x->x_lastseq, seq16);
  if(seq>x->x_lastseq)x->x_lastseq=seq;

  tsrange_t*ts=tsrange_create(seq, TS0, TS1);

  if  (!x->x_stop  && x->x_start)x->x_stop =tsrange_last(x->x_start); else
    if(!x->x_start && x->x_stop) x->x_start=tsrange_last(x->x_stop );

  /* add the new tsrange object (sorted along seq) */
  if(x->x_stop) {
    /* find the correct spot */
    tsrange_t*prior=x->x_stop;
    while(prior && (prior->seq > seq))prior=prior->prev;
    if(prior) {
      if(seq == prior->seq) {
        /* replace element */
        if(prior->prev)prior->prev->next=ts;
        if(prior->next)prior->next->prev=ts;

        ts->prev=prior->prev;
        ts->next=prior->next;

        if(prior==x->x_stop )x->x_stop =ts;
        if(prior==x->x_start)x->x_start=ts;

        tsrange_destroy(prior);
      } else { /* seq != prior->seq */
        /* insert element after prior */
        tsrange_addTail(prior, ts);
        x->x_stop =tsrange_last(x->x_stop);
        x->x_start=tsrange_first(x->x_start);
      }
    } else { /* !prior */
      /* new first element */
      tsrange_addHead(x->x_start, ts);
      x->x_start=tsrange_first(x->x_start);
    }
    x->x_stop=tsrange_last(x->x_stop);
  } else { /* !x->x_stop */
    x->x_start = x->x_stop = ts;
  }
}

static t_class *tsrange2seq_class=NULL;
static void *tsrange2seq_new(t_float UNUSED(f)) {
  t_tsrange2seq *x = (t_tsrange2seq *)pd_new(tsrange2seq_class);
  x->x_haveout=outlet_new(&x->x_obj, &s_list);
  x->x_missout=outlet_new(&x->x_obj, &s_list);

  return x;
}
static void tsrange2seq_free(t_tsrange2seq *x) {
#define FREEOUTLET(x)  if(x)outlet_free(x);x=NULL
  FREEOUTLET(x->x_haveout);
  FREEOUTLET(x->x_missout);

  tsrange_delFrom(x->x_start);
  x->x_start=NULL;
  x->x_stop =NULL;
}
void tsrange2seq_setup(void) {
  tsrange2seq_class = class_new(gensym("tsrange2seq"), (t_newmethod)tsrange2seq_new, (t_method)tsrange2seq_free,
                                sizeof(t_tsrange2seq), 0, A_DEFFLOAT, A_NULL);

  class_addbang(tsrange2seq_class, (t_method)tsrange2seq_dump);
  class_addmethod(tsrange2seq_class, (t_method)tsrange2seq_clear, gensym("clear"), A_NULL);
  class_addmethod(tsrange2seq_class, (t_method)tsrange2seq_check, gensym("check"),
                  A_GIMME, A_NULL);

  class_addmethod(tsrange2seq_class, (t_method)tsrange2seq_add, gensym("add"),
                  A_FLOAT,
                  A_FLOAT, A_FLOAT,
                  A_DEFFLOAT,
                  A_NULL);
}
