#include <embr/platform/esp-idf/twai.h>

using namespace embr::esp_idf;

void twai_init()
{
}

void twai_loop()
{
    itwai_streambuf s;

    int v = s.sbumpc();

    otwai_streambuf s2;

    s2.sputc(v);
    s2.sputn("hi", 2);
}
