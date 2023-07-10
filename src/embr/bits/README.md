# embr::bits

Examples all rely on:

```c++
uint8_t be_example1[] = { 0x12, 0x34, 0x56, 0x78 };
uint8_t le_example1[] = { 0x78, 0x56, 0x34, 0x12 };
uint32_t endian_example1 = 0x12345678;

struct descriptor
{
    const unsigned bitpos;
    const unsigned length;
};
```

Document v0.3

## Introduction

### embr::bits namespace

### embr::bits::detail namespace

The getters and setters provided in this namespace demand
a compile time indicator of the `bitpos` and `length` involved

## Terms and conventions

| Term                      | Description                                                           |
|---------------------------|-----------------------------------------------------------------------|
| bit material              | Specific bit range composing the target value of encode or decode     |
| excluded bits             | Bits within bytes housing bit material which are *not* material       |
| lsb_to_msb                | Indicates that bit material begins at lsb and ends at msb (inclusive) |
| msb_to_lsb                | Indicates that bit material begins at msb and ends at lsb (inclusive) |
| bitpos                    | Indicates where within a byte bit material begins.  e.g. bitpos 2 in `lsb_to_msb` could describe 0b111111xx |
| `x` in bit representation | means bits excluded based on `bitpos`                                 |
| `.` in bit representation | means bits excluded based on `length`                                 |
| inside bits               | Material bits in a particular byte.  Associated to `bitpos` or `length`  |
| outside bits              | Excluded bits in a particular byte.  Associated to `bitpos` or `length`  |


For this document, bits to the left are considered more significant.  i.e.

`1010 0011` is always interpreted as `0xA3`

For this document, examples always start in big endian (network order) format. i.e.

`00010011 10100011` is interpreted by default as `0x13A3`

## 1. Byte operations

In this case, operations happen within a single byte only.
Endian concerns are therefore irrelevant

### 1.1. lsb_to_msb

Bit material begins at `bitpos` counting from LSB to MSB
Bit material continues for `length` in an MSB direction

So, bitpos of 4 and length of 2 means that raw bits:

`1010 0011` filter to `..10 xxxx`

## 2.1 Word Operations

Word operations are undefined for lengths which do not span more than one byte.

### 2.1.1. Big Endian, lsb_to_msb

bitpos of 4 and length of 7 means that raw bits:

`1010 0011` filter to `1010 xxxx`

#### 2.1.1.1. remainder is lsb_to_msb

Remaining 3 bit material is expected in rightmost bits of next byte.

`10100011 11000101` -> `1010xxxx .....101` -> `01010101`

#### 2.1.1.2. remainder is msb_to_lsb

Remaining 3 bit material is expected in leftmost bits of next byte.  
For example:

`10100011 11000101` -> `1010xxxx 110.....` -> `01010110` 

### 2.1.2. Big Endian, msb_to_lsb

bitpos of 4 and length of 7 means that raw bits:

`1010 0011` filter `xxx0 0011`

Remaining 2 bit material is expected in leftmost bits of next byte.

`10100011 11000101` -> `xxx0 0011` `11.. ....`

### 2.1.3. Little Endian, lsb_to_msb

bitpos of 4 and length of 7 mean that raw bits:

`10100011 11000101` filter to `1010xxxx 11000101` for bit pos

#### 2.1.3.1. remainder is lsb_to_msb

`1010xxxx .....101` after length is considered.  Then, conceptually we swap the bytes
since it's little endian:

`.....101 1010xxxx` resulting ultimately in `1011010`

#### 2.1.3.2. remainder is msb_to_lsb

`1010xxxx 110.....` after length is considered.  Then, conceptually we swap the bytes
since it's little endian:

`110..... 1010xxxx` resulting ultimately in `1101010`

### 2.1.4. Little Endian, msb_to_lsb

bitpos of 4 and length of 7 mean that raw bits:

`10100011 11000101` filter to `xxx00011 11......` for bit pos+length.  Conceptually
we swap them since it's little endian:

`11...... xxx00011` resulting ultimately in `01100011`