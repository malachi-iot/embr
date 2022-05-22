#include "process.h"

void process_out(ipbufstream& in, opbufstream& out)
{
    const char* TAG = "process_out";

    //embr::lwip::ipbuf_streambuf& in_rdbuf = *in.rdbuf();
    auto& in_rdbuf = *in.rdbuf();
    //int tot_len = in_rdbuf.cnetbuf().total_size();

    if(in.peek() == '!')
    {
        in.ignore();
        switch(int ch = in.get())
        {
            case '1':
                out << "123";
                break;

            default:
                out << '!';
                out.put(ch);
                break;
        }
    }

    int in_avail = in_rdbuf.in_avail();

    // NOTE: Stack crash on this line sometimes, may need sdkconfig adjustment
    // or perhaps from an lwip callback we are expected to use LWIP_DEBUGF ?
    ESP_LOGD(TAG, "in_avail = %d", in_avail);

    if(in_avail > 0)
    {
        char* inbuf = in_rdbuf.gptr();

        // DEBT: in_avail() does not address input chaining
        out.write(inbuf, in_avail);

        in.ignore(in_avail);
    }
}