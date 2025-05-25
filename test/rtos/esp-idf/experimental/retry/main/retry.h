#pragma once

struct packet
{
    unsigned seq : 10;
    unsigned ack : 1;
    unsigned announce : 1;

    packet() : seq{0}, ack{0}, announce{0}  {}
};