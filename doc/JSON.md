# Zero-allocation JSON

Wouldn't it be nice if you could work with JSON without a ton of dynamic overhead?
You see where I'm going here, right?

## Encoder

Encoder itself takes at most 3 words of memory, and 1 word in a pinch.

This does not include memory used by the output buffer, as maintained by
the `estd::detail::basic_ostream` of your preference.  Naturally, the
`estd::detail::basic_ostream` also doesn't specifically mandate dynamic
allocation, though you may find you want to (i.e. the LwIP version)

### Core Interface

Takes only 32 bits

### Fluent Interface

Takes space of only two references (pointers)

```
("user")
    ("age", 30)
    ("name", "Fred")
();
```

This snippet yields:

```
"user":{"age":30,"name":"Fred"}
```

## Decoder

Not implemented at this time
