# transport

Potential readable feature indicator flags:

## transport itself

- connectable (maintains a connection ala stream)
    - whether connecting results in a new transport, or modifies current one
- read flags for blocking, async/transaction and callback 
    - callback likely to have some platform specificity
    - callback traits: per transport/singleton or per request
- write flags for blocking, async/transaction and callback (CTS style, probably)

async/transaction model useful for chained read/writes which pbuf somewhat resembles
then again, it adds complication to transport and perhaps should be relegated only to buffer

## buffer

- chain/chunkable
- seekable
