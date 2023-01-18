#pragma once

#include "../observer.h"

namespace embr { namespace experimental {

namespace event {

struct PropertyChanged
{
    int id;
};

}

namespace service {

enum States
{
    Unstarted = 0,
    Starting,
    Started,
    Running,
    Stopping,
    Stopped
};

}

template <class TSubject = embr::void_subject>
class PropertyNotifier : public TSubject
{
    typedef TSubject subject_type;

    struct
    {
        service::States service_ : 4;

    }   state_;

protected:
    void state(service::States s)
    {
        if(s != state_.service_)
        {
            state_.service_ = s;

            subject_type::notify(event::PropertyChanged{0}, *this);
        }
    }

public:
    PropertyNotifier() = default;
    PropertyNotifier(TSubject&& subject) : subject_type(std::move(subject))
    {}
};

}}