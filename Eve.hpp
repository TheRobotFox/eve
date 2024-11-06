#ifndef EVE_H_
#define EVE_H_

#include "Async.hpp"
#include "Event.hpp"
#include "Interval.hpp"
#include "Reactive.hpp"
#include <type_traits>

namespace eve {

    template<event_queue::EventQueue Q, template<class q> class ...Mp> requires (Module<Mp, Q> && ...)
    class Eve : public Q, public Mp<Q>...
    {
    public:
        using Queue = Q;
        using Event = Queue::Event;
        auto run() -> void
        {
            (Mp<Q>::run(*this), ...);
            Queue::pop(); // Consume event
        }
    };

    using Default = Eve<event_queue::DefaultQueue<event::EventAny>, modules::Interval, modules::React>;
}


#endif // EVE_H_
