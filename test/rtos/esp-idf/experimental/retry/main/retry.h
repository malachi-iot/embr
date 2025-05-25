#pragma once

struct packet
{
    unsigned seq : 10;
    unsigned ack : 1;
};