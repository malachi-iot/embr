# transport

Potential readable feature indicator flags:

## transport itself

- connectable (maintains a connection ala stream)
    - whether connecting results in a new transport, or modifies current one
- read flags for blocking, async/transaction and callback 
    - callback likely to have some platform specificity
- write flags for blocking, async/transaction and callback (CTS style, probably)

## buffer

- chain/chunkable
- seekable
