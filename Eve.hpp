#ifndef EVE_H_
#define EVE_H_

#include "EveDef.hpp"
#include "Event.hpp"
#include "Interval.hpp"
#include "Reactive.hpp"
#include <type_traits>

namespace eve {

    template<EveType EV, template<class> class ...Mp>
    requires (Module<Mp, EV> && ...) && features::EveCore<EV>
    class Eve : public EV, public Mp<EV>...
    {
    public:
        using Event = EV::Event;
        auto run() -> void
        {
            (Mp<EV>::run(*this), ...); // sort Emit, Filter, Handle
            if(!EV::empty()) EV::pop();

        }
    };

    using Default = Eve<event_queue::DefaultQueue<event::EventAny>, modules::Interval, modules::React>;
}


#endif // EVE_H_
