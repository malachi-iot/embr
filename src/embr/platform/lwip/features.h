#pragma once

// FEATURE_ESTD_NETBUF_STREAMBUF represents the legacy netbuf-style
// approach of gluing streambuf to lwip/pbufs.  The new way goes direct
// from streambuf to pbuf
// TODO: Move this into a features.h
// NOTE: Keeping legacy code around for reference since it will probably work well
// with netconn approach
#ifndef FEATURE_EMBR_NETBUF_STREAMBUF
#define FEATURE_EMBR_NETBUF_STREAMBUF 0
#endif

