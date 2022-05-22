#pragma once

#include <estd/internal/value_evaporator.h>

#include <embr/scheduler.h>

// Where's retry v3?  That's under platform/freertos/exp/transport-retry

namespace embr { namespace experimental { namespace mk4 {

template <class TTransport, class TProtocol>
class RetryManager :
    protected estd::internal::struct_evaporator<TTransport>
{
    typedef typename estd::remove_cv<TTransport>::type transport_type;
    typedef estd::internal::struct_evaporator<TTransport> transport_provider;
    typedef typename transport_provider::evaporated_type _transport_type;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef TProtocol protocol_type;
    typedef typename transport_type::istreambuf_type istreambuf_type;

    _transport_type transport() { return transport_provider::value(); }

    struct SchedulerImpl
    {
        typedef estd::chrono::steady_clock::time_point time_point;

        struct control_structure
        {
            typedef SchedulerImpl::time_point time_point;

            time_point t;

            virtual bool process(time_point current_time) = 0;
        };

        typedef control_structure* value_type;

        static time_point get_time_point(value_type v) { return v->t; }

        static bool process(value_type v, time_point t)
        {
            return v->process(t);
        }
    };

    struct retry_item : SchedulerImpl::control_structure
    {
        typedef typename SchedulerImpl::time_point time_point;
        int retries;
        buffer_type buffer;
        const endpoint_type endpoint;

        retry_item(const buffer_type& buffer, const endpoint_type& endpoint) :
            retries(0),
            buffer(buffer),
            endpoint(endpoint)
        {

        }

        virtual bool process(time_point current_time) override
        {
            return false;
        }
    };


    embr::internal::layer1::Scheduler<SchedulerImpl, 10> scheduler;

public:
    void send(const buffer_type& buffer, const endpoint_type& endpoint)
    {
        // DEBT: Definitely do not want to do traditional new/delete here.  Only doing so
        // for proof of concept/experimentation
        scheduler.schedule(new retry_item(buffer, endpoint));
    }

    void process()
    {

    }

    void process_incoming(istreambuf_type& s, const endpoint_type& endpoint)
    {
        if(protocol_type::is_ack(s))
        {
            // Evaluate to see if this particular buffer is an ACK that we are looking for
            // NOTE: With current architecture that's a brute force search through our scheduler.  Bummer.
        }
    }

    void process_incoming(const buffer_type& buffer, const endpoint_type& endpoint)
    {
        istreambuf_type s(buffer);

        return process_incoming(s, endpoint);
    }

    RetryManager(TTransport&& transport) : transport_provider(std::move(transport)) {}
    RetryManager(const TTransport& transport) : transport_provider(transport) {}
    RetryManager() = default;
};

}}}