


connection establisher decoupled from connection responder
(inbound packets using rr/dns-delegation/hashing consistently go to the correct
server, which finishes establishing the connection. As soon as it gets the
first actual packet [which we can assume will be the GET request] it can drop
it [if malformed or bad] or signal another server to respond - at that point
all inbound packets for that connection are forwarded to the responder. [that
last part is the tricky part- or not? GRE tunnel & maintained 4-tuple+dest
table?])



1.  Client:cP sends SYN packet to any quicktwitch server QTi:80
2.  QTi:80 sends ACK/SYN to Client:cP
3.  Client:cP sends ACK to QTi:80
4.  Client:cP sends GET request to QTi:80
5.  QTi:80 ACKs the GET request (assume it will almost always be one packet)
6.  QTi figures out who should actually serve up the content, actual QT: QTa
7.  QTi sets up iptables/iproute2 to route everything for Client:cP to QTa (DNAT) (how to deal w/ ack not getting to client & client retransmitting request packet?)
8.  QTi transfers GET request packet to QTa which also transfers socket context
9.  QTa creates a duplicate socket structure, "pre-connected" to Client
10. QTa begins responding to the request, masquerading as QTi


wrt #9 - Don't know how to do it- skip accept somehow...?


QTi sets up DNAT to route C:cP to QTa
QTi opens tcp socket w/ QTa:9070
  - masquerades as C:cP but changes dest, of course, to QTa:9070 (in the exact
    same way that the DNAT will do it for packets coming from C:cP)
  - tries to replay communication from the client exactly- at least so that
    timestamps, sequence number, and buffers are all exactly "pre-GET-response."
QTa masquerades source as QTi (SNAT? How does it know who QTi is??) and begins answering client request.
AHH, by having each QTa:--- listening port correspond to a different origin!

When can QTi remove the DNAT entries? How can it tell when the connection is
closed. Possibly inspect stuff going through?

port = qphash(ip-addr-str) + 8000   (or gperf if config instead of convention)

port = 8000 + (dest-ipaddr - src-ipaddr)


QT1  199.9.250.137 
QT2  199.9.250.138  c709fa8a
QT3  199.9.250.139
QT4  199.9.252.140  c709fc8c

CL1  61.74.37.16


1. CL1:89218 -> SYN       -> QT2:80
2. QT2:80    -> ACK/SYN   -> CL1:89218
3. CL1:89218 -> ACK       -> QT2:80
4. CL1:89218 -> "GET ..." -> QT2:80
5. QT2:80    -> ACK       -> CL1:89218
6. Determines that it should really be served from QT4
7. on QT2: iptables -t nat -A PREROUTING -p tcp -s 61.74.37.16 --sport 89218 -d 199.9.250.138 --dport 80 -j DNAT
   --to-destination 199.9.250.140:8514
8.1 QT2[CL1:89218] -> SYN       -> QT4:8514 (don't wait for ACK/SYN)
8.2 QT4:8514       -> ACK/SYN   -> CL1:89218 (same as original seq number, so gets dropped)
8.3 QT2[CL1:89218] -> ACK       -> QT4:8514 (don't wait for ACK)
8.4 QT2[CL1:89218] -> "GET ..." -> QT4:8514 (don't wait for ACK)
8.5 QT4:8514       -> ACK       -> CL1:89218 (client considers it a dup. drops)
9. ??? (ideally after 8.1, but for sure after 8.5) on QT4: iptables -t nat -A POSTROUTING -p tcp -j SNAT --




sudo strace iptables -t nat -A PREROUTING -p tcp -s 61.74.37.16 --sport 89218 -d 199.9.250.137 --dport 80 -j DNAT --to-destination 199.9.250.137:8080




1. Connection to QA & GET request
2. QA determines real server QB
3. Forward all subsequent packets to QB (Rule 1)
4. Cause QB to masq as QA when sending to client (Rule 2)
5. Cause QB ACK/SYN (and possibly first ACK afterward) to get back to QA somehow (Rule 3)
6. Open connection to QB, masqed as client, using raw socket
7. Repeat GET request to QB exactly, for correct state
8. Remove Rule 3
9. Stuff gets delivered, socket closes
10. Cause QB to remove Rule 2
11. Cause QA to remove Rule 1 (possibly via tapping fin/reset packets)


1. Connection to QA & GET request (conn1)
2. QA determines real server QB
3. QA opens connection with QB (conn2) - sets conn2a:s = Client
4. QB sets conn2b:s = QB
4. QA replays packets to conn2












* 2nd band of communication (sctp probably)
* linux module that allows a ioctl call to modify connection's
  source/destination




* * * * * *



### fe-module:
Allows a user-space process to turn a socket into a forwarded socket.

API: ioctl ....   be-ip-addr, be-port, catchup-data
     (closes the fd from this process' perspective)

Implementation:
  * Calculates new initial sequence # such that after the handoff-header &
    catchup-data is transfered the recipient socket will be in the same state
    as the original fe socket.
  * Build a handoff-header- most important is the front-end's ipaddress/port
    that the be-module will use to masqurade as and possibly additional socket
    state parameters that need to be fed into the be sock/sk structures.
  * Open connection to BE _without_ masquerading as the originating client yet
    - after usual 3-way handshake send handoff header and GET request.
  * Forward all subsequent packets to BE, masquerading as client.

Thoughts:
  * Possibly do handoff-header as OOB-data (depending on Linux's implementation
    in the TCP stack) to make a non-dsr backend degrade gracefully.
  * Ideally do handoff-header etc. in such a way that if given to a BE without
    the be-module loaded it would degrade into a TCP-splice / dynamic NAT.

### be-module:
Processes handoff-header and uses info to fix anything w/ the socket and
masquerade as the front-end. In other words, implement DSR.


