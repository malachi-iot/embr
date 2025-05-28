#pragma once

#include <cstdint>

struct packet
{
    unsigned seq : 10;
    unsigned ack : 1;
    unsigned announce : 1;

    packet(unsigned seq = 0) : seq{seq}, ack{0}, announce{0}  {}
};


constexpr static uint8_t broadcast_mac[] { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
