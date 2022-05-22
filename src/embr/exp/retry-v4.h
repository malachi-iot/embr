#pragma once

#include <estd/internal/value_evaporator.h>

#include <embr/scheduler.h>

// Where's retry v3?  That's under platform/freertos/exp/transport-retry

namespace embr { namespace experimental { namespace mk4 {

template <class TSchedulerImpl>
struct RetryListener
{
    static void on_notify(internal::events::Scheduled<TSchedulerImpl> e)
    {

    }

    static void on_notify(internal::events::Removed<TSchedulerImpl> e)
    {

    }
};

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

    typedef estd::chrono::steady_clock::time_point time_point;

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


    // IDEA: To aid in auto deletion on 'removed' event, after changing scheduler::process to only
    // fire that event when item is truly removed, rather than rescheduled.  Note that this would
    // probably make this code c++11 dependent.  Alternative is to add a 'process_one' to scheduler
    // which returns what item it processed and a flag about whether it's still scheduled
    //embr::internal::layer1::Scheduler<SchedulerImpl, 10, RetryListener<SchedulerImpl> > scheduler;
    embr::internal::layer1::Scheduler<SchedulerImpl, 10> scheduler;

    void process_incoming(istreambuf_type& s, const endpoint_type& endpoint)
    {
        typename protocol_type::ack_id_type id;
        if(protocol_type::is_ack(s, &id))
        {
            // Evaluate to see if this particular buffer is an ACK that we are looking for
            // NOTE: With current architecture that's a brute force search through our scheduler.  Bummer.
        }
    }

public:
    void send(const buffer_type& buffer, const endpoint_type& endpoint)
    {
        // DEBT: Definitely do not want to do traditional new/delete here.  Only doing so
        // for proof of concept/experimentation
        scheduler.schedule(new retry_item(buffer, endpoint));
    }

    void process(time_point current)
    {
        scheduler.process(current);
    }

    ///
    /// \param buffer
    /// \param endpoint endpoint from which this packet originated
    void process_incoming(const buffer_type& buffer, const endpoint_type& endpoint)
    {
        istreambuf_type s(buffer);

        return process_incoming(s, endpoint);
    }

    RetryManager(TTransport&& transport) :
        transport_provider(std::move(transport))
        //scheduler(*this)
        {}
    RetryManager(const TTransport& transport) :
        transport_provider(transport)
        //scheduler(*this)
    {}
    RetryManager() = default;
};

}}}