#include "datapump.h"

namespace embr {

template <class TTransportDescriptor, class TPolicy>
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
auto DataPump<TTransportDescriptor, TPolicy>::transport_in(netbuf_type&& in, const addr_t& addr) ->
    const Item&
{
    return incoming.emplace(std::move(in), addr);
}
#else
const typename DataPump<TTransportDescriptor, TPolicy>::Item&
    DataPump<TTransportDescriptor, TPolicy>::transport_in(netbuf_t& in, const addr_t& addr)
{
    Item item(in, addr);

    incoming.push(item);

    return incoming.back();
}
#endif

}
