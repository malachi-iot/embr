#pragma once

#include <estd/string_view.h>

// As per https://medium.com/@felixstridsberg/the-zip-file-format-6c8a160d1c34
// and https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip.html

#define EMBR_ZIP_CENTRAL_DIRECTORY_SIGNATURE    0x02014B50
#define EMBR_ZIP_LOCAL_FILE_SIGNATURE           0x04034b50
#define EMBR_ZIP_EOCD_SIGNATURE                 0x06054b50
#define EMBR_ZIP_CRC_MAGIC                      0xDEBB20E3

namespace embr { namespace zip { namespace header {

// "All multi byte numbers are [...] little-endian"

// TODO: I thought I made an auto-endian-converting word, but embr::word
// itself shows no sign of this.  Would be pretty useful to have that
// TODO: 'bytes' units might come in handy here too

namespace flags {

// Not making enum class since regular enums seem to fare better in bitwise operations
enum local_file : uint16_t
{
    encrypted_file =    0x0001,
    compression1 =      0x0002,
    compression2 =      0x0004,
    data_descriptor =   0x0008
};

}

// "The data descriptor is only present if bit 3 of the bit flag field is set"
struct __attribute__((packed)) data_descriptor
{
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
};

struct __attribute__((packed)) extra_field
{
    uint16_t id;
    uint16_t size;
};


struct __attribute__((packed)) local_file
{
    uint32_t signature;
    uint16_t version;

    struct __attribute__((packed)) header
    {
        flags::local_file flags;
        uint16_t compression_method;
        uint16_t time;
        uint16_t date;
        uint32_t crc;
    }   h;
    
    struct __attribute__((packed)) length_type
    {
        uint32_t compressed;
        uint32_t uncompressed;
        uint16_t filename;
        uint16_t extra;

    }   length;

    uint32_t total_size() const
    {
        uint32_t sz = length.filename + length.extra + length.compressed;

        if(h.flags & flags::data_descriptor)
            sz += sizeof(data_descriptor);

        return sz;
    }

    //char filename[];

    // TBD: extra hangs off the end of filename
};

struct __attribute__((packed)) central_directory
{
    uint32_t signature;
    uint16_t version_made_by;
    uint16_t version_to_extract;

    local_file::header h;
    
    struct __attribute__((packed)) length_type : local_file::length_type
    {
        uint16_t file_comment;

    }   length;

    uint16_t disk_number;
    uint16_t internal_attributes;
    uint32_t external_attributes;
    uint32_t local_file_offset;

    //char filename[];
};

struct __attribute__((packed)) end_of_central_directory
{
    uint32_t signature;

    struct __attribute__((packed))
    {
        uint16_t number;
        uint16_t central_directory_start;
        uint16_t central_directory_records;

    }   disk;

    uint16_t central_directory_records;
    uint32_t central_directory_size_bytes;
    uint32_t offset_to_start_of_central_directory;
    uint16_t comment_length;
    uint8_t comment[];
};

using EOCD = end_of_central_directory;

namespace layer1 {

template <uint16_t N>
struct __attribute__((packed)) local_file : header::local_file
{
    char data[N];

    /// "name of the file including an optional relative path.
    /// All slashes in the path should be forward slashes '/'."
    /// @return
    estd::string_view filename()
    {
        // DEBT: https://github.com/malachi-iot/estdlib/issues/24
        return { data, (int16_t)length.filename };
    }

    template <class Impl>
    void filename(const estd::detail::basic_string<Impl>& v)
    {
        length.filename = v.size();

        // DEBT: I think it's time to give lock/unlock the 'mutable' treatment
        memcpy(data, v.clock(), length.filename);

        v.cunlock();
    }

    estd::span<uint8_t> extra()
    {
        return { data + length.filename, length.extra };
    }
};

template <uint16_t N>
struct __attribute__((packed)) central_directory : header::central_directory
{
    char data[N];

    estd::string_view filename()
    {
        // DEBT: https://github.com/malachi-iot/estdlib/issues/24
        return { data, (int16_t)length.filename };
    }

    estd::span<uint8_t> extra()
    {
        return { data + length.filename, length.extra };
    }

    estd::span<uint8_t> comment()
    {
        return { data + length.filename + length.extra, length.file_comment };
    }
};


}

}}}