#pragma once

#include <esp_log.h>
#include <driver/spi_master.h>

namespace embr { namespace spi {

// Experimental, mainly could be useful for queuing/dequeuing since we can emplace
// this more easily
class transaction
{
    spi_transaction_t trans_desc;
protected:
public:
    transaction(const spi_transaction_t& copy_from) : trans_desc(copy_from) {}
    transaction(const transaction& copy_from) = default;
    transaction() = default;

    operator spi_transaction_t&() { return trans_desc; }
    operator const spi_transaction_t&() const { return trans_desc; }
};

}}