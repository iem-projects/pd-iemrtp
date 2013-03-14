/*
 * rtp: RTP support for Pd (RTP component)
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

#define RTP_HEADERSIZE 13
#define RTP_BYTESPERSAMPLE 2
#define EMPTYPACKETBYTES 100


#define SELECTOR_RTPHEADER_VERSION gensym("version")
#define SELECTOR_RTPHEADER_P       gensym("padding")
#define SELECTOR_RTPHEADER_X       gensym("extension")
#define SELECTOR_RTPHEADER_CC      gensym("cc")
#define SELECTOR_RTPHEADER_M       gensym("marker")
#define SELECTOR_RTPHEADER_PT      gensym("payload_type")
#define SELECTOR_RTPHEADER_SEQ     gensym("sequence_number")
#define SELECTOR_RTPHEADER_TS      gensym("timestamp")
#define SELECTOR_RTPHEADER_SSRC    gensym("SSRC")
#define SELECTOR_RTPHEADER_CSRC    gensym("CSRC")

typedef struct _rtpheader {
  /* byte#1 */
  unsigned int version:2;   /* protocol version */
  unsigned int p:1;         /* padding flag */
  unsigned int x:1;         /* header extension flag */
  unsigned int cc:4;        /* CSRC count */
  /* byte#2 */
  unsigned int m:1;         /* marker bit */
  unsigned int pt:7;        /* payload type */
  /* byte#3-4 */
  unsigned int seq:16;      /* sequence number */
  /* byte#5-8 */
  u_int32 ts;               /* timestamp */
  /* byte#9-12 */
  u_int32 ssrc;             /* synchronization source */
  /* optional byte#13...(12+pt*4) */
  u_int32*csrc;             /* optional CSRC list */
} t_rtpheader;


/**
 * @brief free dynamically allocated members in RTP-header.
 *        the rtp-header itself is not touched;
 *        the freed members are set to valid (zero) values.
 * @param rtpheader pointer to initialized RTP-header
 */
void iemrtp_rtpheader_freemembers(t_rtpheader*rtpheader);

/**
 * @brief make sure the CSRC-array is large enough
 * @param rtpheader pointer to initialized RTP-header
 * @param size minimum size of CSRC array.
 * @return the size of the CSRC-field after a possible resize
 *         on error, 0 is returned
 */
int iemrtp_rtpheader_ensureCSRC(t_rtpheader*rtpheader, int size);

/**
 * @brief synthesize a byte-package (atom list) from an rtpheader.
 * @param argv array of bytes (as atoms) to write to
 * @param rtpheader pointer to initialized RTP-header, that is used as input.
 * @return the number of atoms consumed by the header
 *         on error, 0 is returned
 */
u_int32 iemrtp_rtpheader2atoms(t_rtpheader*rtpheader, t_atom*argv);

/**
 * @brief parse a byte-package (atom list) to an rtpheader.
 * @param argc total length of the list
 * @param argv array of bytes (as atoms)
 * @param rtpheader pointer to initialized RTP-header, that is used as output.
 * @note any CSRC array in the rtpheader will be deleted and replaced by CSRC fields found in the byte-package
 * @return the number of bytes consumed by the header
 *         on error, 0 or a negative number (minimum expected packet size ) is returned
 */
int iemrtp_atoms2rtpheader(int argc, t_atom*argv, t_rtpheader*rtpheader);



typedef void (*t_rtppay_perform)(u_int32 vecsize, u_int32 channels, t_sample**samples, u_int8*buffer);

typedef struct _rtppay
{
	t_object x_obj;
  t_sample**x_in;
  u_int8 x_running;
  u_int8 x_banged;

  u_int32 x_channels;  // number of channels
  u_int32 x_vecsize;   // Pd's blocksize
  u_int32 x_mtu;       // MTU of the socket

  t_rtpheader  x_rtpheader; // the RTP-header
  u_int32 x_rtpheadersize; // the size of the RTP-header

  /* converting */
  t_rtppay_perform x_perform;

  /* buffer to store the bytes into */
  u_int8*x_buffer;
  u_int32 x_buffersize;
  u_int8  x_bytespersample;

  /* buffer for outputting bytes as list */
  t_atom*x_atombuffer;
  u_int32 x_atombuffersize;
  t_clock*x_clock;
  t_outlet*x_outlet;

  /* packetizing */
  u_int32 x_payload;
} t_rtppay;

void *iemrtp_rtppay_new(t_rtppay*x, int chan, int bytespersample, t_rtppay_perform perform);
void iemrtp_rtppay_free(t_rtppay*x);
void iemrtp_rtppay_classnew(t_class*rtppay_class);
