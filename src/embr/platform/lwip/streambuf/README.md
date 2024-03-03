# LwIP streambuf notes

## tcp_pcb

### tcpip task

In order to use this streambuf, operations must occur on tcpip task.
This is a big ask, and makes this streambuf less practical.

### TCP_WRITE_FLAG_COPY

Leans heavily on this, making this streambuf very much not
zerocopy.  Bummer.

## netconn (nocopy)

Coming along well.  Currently the following areas need
ironing out:

* NETCONN_DONTBLOCK treatment
* NETCONN_MORE treatment
* Overall callback treatment
* Overall buffer treatment

Next steps are to listen more intently on callback and
move forward the `pos_ack_end_` variable to indicate to us
what memory is truly safe to write to.

### NETCONN_DONTBLOCK

aka zero-copy.
By virtue of using this we need a signal from
LwIP that a buffer is sent/free to clear.  Code does
not do this yet and therefore is deficient in that our
intermediate cache may be overwritten before netconn is
done reading from it.

This behavior is gleaned from https://www.nongnu.org/lwip/2_1_x/group__tcp__raw.html#ga6b2aa0efbf10e254930332b7c89cd8c5
which tells us "the memory behind dataptr must not change until the data is ACKed by the remote host"

We may elect to use NETCONN_COPY to overcome this,
but that would be a bummer.

A kind of "read only" mode for the streambuf would be handy
for this too, though not a complete solution (doesn't
overcome the backing store buffer overwrite issue for non-read-only
scenarios)

### NETCONN_MORE

aka TCP PSH (Push)
There's no std-conforming way to indicate a PSH ought
to happen, though a close match is a standalone `sync`
call.  Not as critical as NETCONN_MORE

### Callback

This source: https://doc.ecoscentric.com/ref/lwip-api-sequential-netconn-new-with-callback.html
Indicates the callback receives NETCONN_EVT_SENDPLUS on the ACK signaling we need.  No other sources say so.

### Buffers

Our buffer treatment is crude, presuming one contiguous
region provided by a pbuf.  What is truly called for here is
a circular buffer combined with resiliency to smaller non-contiguous
regions.

## netconn (copy)

Due to complexity of 'nocopy' variety, we have a stand-in which is not
as efficient and uses the netbuff copy mechanism.

Considered mild DEBT because:

* presence of streambuf implies efficiency, which this somewhat is not
* mild because we're still nonblocking and there *are* conceivable use
  cases specifically for this variety, even when our 'nocopy' comes online