#pragma once

// As per https://medium.com/@felixstridsberg/the-zip-file-format-6c8a160d1c34

#define EMBR_ZIP_CENTRAL_DIRECTORY_SIGNATURE    0x02014B50
#define EMBR_ZIP_LOCAL_FILE_SIGNATURE           0x04034b50
#define EMBR_ZIP_EOCD_SIGNATURE                 0x06054b50

namespace embr { namespace zip { namespace header {

// "All multi byte numbers are [...] little-endian"

// TODO: I thought I made an auto-endian-converting word, but embr::word
// itself shows no sign of this.  Would be pretty useful to have that
// TODO: 'bytes' units might come in handy here too

struct __attribute__((packed)) local_file
{
    uint32_t signature;
    uint16_t version;

    struct __attribute__((packed)) header
    {
        uint16_t flags;
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

    char filename[];

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

    char filename[];
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

}}}