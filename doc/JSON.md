# Zero-allocation JSON

Wouldn't it be nice if you could work with JSON without a ton of dynamic overhead?
You see where I'm going here, right?

## Encoder

In a 32-bit environment, encoder itself takes at most 3 words of memory, and 1 word in a pinch.

This does not include memory used by the output buffer, as maintained by
the `estd::detail::basic_ostream` of your preference.  Naturally, the
`estd::detail::basic_ostream` also doesn't specifically mandate dynamic
allocation, though you may find you want to (i.e. the LwIP version)

The following core and fluent interfaces both output the following to a stack allocated 128 byte string buffer

```
"user":{"age":30,"name":"Fred"}
```

### Core Interface

Takes only 32 bits.  Gets the job done in the leanest fashion possible

```
estd::detail::basic_ostream<estd::layer1::basic_out_stringbuf<char, 128> out;
embr::json::v1::encoder<> e;

e.begin(out, "user");
e.add(out, "age", 30);
e.add(out, "name", "Fred");
e.end(out);
```

### Fluent Interface

Takes space of core encoder (one word) and one references (pointer) to `out`

```
estd::detail::basic_ostream<estd::layer1::basic_out_stringbuf<char, 128> out;
auto make_fluent(out)

("user")
    ("age", 30)
    ("name", "Fred")
();
```

An additional `make_fluent` is available accepting an encoder reference resulting in a slightly different memory signature.

### Configurability

### Limitations

Maximum nested levels to 16.

## Decoder

Not implemented at this time
