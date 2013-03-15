iemrtp - RTP support for Pd
===========================

this is a small set of objects for transmitting audio over the internet,
in a standardized format.

these objects convert audio data to byte-packets and vice-versa.
the actual transmission (sending/receiving) has to be handled by
other means, e.g. mrpeach's or iemnet's [udpsend]/[udpreceive].

RTP (and the accompanying RTCP) are described in 
	RFC3550

iemrtp does not give you a ready solution for streaming audio.
instead it gives you all the building blocks needed to create a
solution for streaming audio.

working examples can be found in examples/

RTP
===
RTP is the "Real-time Transport Protocol", a lightweight protocol for
transmitting media data (e.g. audio) over the internet.

payloader: [rtp*pay~]
---------------------
generate packets containing multichannel audio-data for a given profile
packets have a valid RTP-header that can be modified via messages
(timestamps, sequence number, payload-type).

depayloader: [unpackRTP] + [*decode]
------------------------------------
parse and decode an RTP-packet.
[unpackRTP] will give you all the meta-information (as found in the header),
and will output the raw payload data.
feeding the raw payload data to the appropriate decoder-object will
reconstruct the original data.

convenience: [rtp*depay~]
-------------------------
using [unpackRTP] and a decoder it is easy to reconstruct audio-signal from
RTP-packets. for convenience this is exemplified in [rtp*depay~]

implemented profiles
--------------------
the way, media data is encoded is called "profile" in RTP-language.
currently the following formats are implemented:

- L16: signed 16bit integer, interleaved

   [rtpL16pay~]: convert audio-signals into L16 encoded RTP-packets
   [L16decode] : convert L16 encoded data into de-interleaved
                 floating point samples 

performance
-----------
for performance reasons, the conversion *to* RTP-packets is done in a single
monolithic object, whereas conversion *from* RTP-packets is modular.
(that's because our current target RTP-sender is a mobile platform, whereas
the RTP-receiver is a full-fledged PC)

LATER we hope to make the encoder modular as well.

RTCP
====
the "RTP Control Protocol" is used to monitor an RTP-connection
[unpackRTCP] decodes an RTCP-package into Pd-messages.
[packRTCP] synthesizes RTCP-packages from Pd-messages.

AUTHORS
=======
IOhannes m zmölnig - Institute of Electronic Music and Acoustics

LICENSE
=======
LGPL-2.1
