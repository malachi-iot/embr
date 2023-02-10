#pragma once

#include "../enum.h"
#include "../fwd.h"

namespace embr {

inline namespace service { inline namespace v1 {

struct ServiceBase : ServiceEnum
{
    struct state_result
    {
        // DEBT: Frustratingly can't use const here because we fall into that operator=
        // trap
        States state : bitsize::state;
        Substates substate : bitsize::substate;

        operator bool() const { return state == Started && substate == Running; }

        static constexpr state_result started()
        {
            return state_result{Started, Running};
        }
    };

    // pre-processing (such as wake, resume, etc.) preceding a 'starting' event
    static inline void on_starting() {  }

    //template <class TSubject, class TImpl>
    //static constexpr state_result on_start(runtime<TSubject, TImpl>&)
    static constexpr state_result on_start()
    {
        return state_result::started();
    }

    constexpr static const char* instance() { return ""; }
};

namespace host {



}


}}

}
