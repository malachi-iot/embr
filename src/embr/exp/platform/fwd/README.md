# transport

Potential readable feature indicator flags:

## transport itself

- connectable (maintains a connection ala stream)
    - whether connecting results in a new transport, or modifies current one
- read flags for blocking, async/transaction and callback 
    - callback likely to have some platform specificity
    - callback traits: per transport/singleton or per request
    - callback indicates duration which variables live: callback only, or longer
- write flags for blocking, async/transaction and callback (CTS style, probably)

async/transaction model useful for chained read/writes which pbuf somewhat resembles
then again, it adds complication to transport and perhaps should be relegated only to buffer

## buffer

- chain/chunkable
- seekable

### buffer location / zerocopy

Use cases

- system-provided buffer which can be peeked into
-- either signal or ref counter to indicate buffer usage is over (might be optional)
-- allocate freestanding/empty to pass into a read who then allocates it
-- allocate more traditionally (but with transport alloc) and pass in 'sparse' buffer for read
-- allocate more traditionally (but with transport alloc) and pass in filled buffer for write (this closely resembles a user buffer, but just satisfies a technical requirement that transport use its own memory)
-- allocated by a read callback system in which case it is handed directly to us
-- allocated by a write callback system, in which it's like the traditional write buffer mentioned above
- user-provided buffer which must be passed to API
-- async hold on buffer, signal complete
-- block until buffer is complete

## tags

Tags make promises about the expected api

### connection

Connection oriented traits have:

* `connect`
* `disconnect`

And indicate that read and write operations exist without explicit endpoint parameters

### connectionless

Indicates that read and write operations have endpoints specified

### read_polling / write_polling

Both of these imply a timeout is at play, though the timeout may be 0 - instant.

