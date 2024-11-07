#ifndef EVE_H_
#define EVE_H_

#include "EveDef.hpp"
#include "Event.hpp"
#include "EventQueue.hpp"
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
            (Mp<EV>::template run<Mp<EV>, features::Emit>(*this), ...);

            if(!EV::empty()){
                (Mp<EV>::template run<Mp<EV>, features::Filter,
                                              features::Handle>(*this), ...);
            }

            (Mp<EV>::template runRest<Mp<EV>, features::Emit,
                                              features::Filter,
                                              features::Handle>(*this), ...);
            if(!EV::empty()) EV::pop();

        }
    };

    template<event::Event e>
    using Default = Eve<event_queue::DefaultQueue<e>, modules::Interval, modules::React>;
}


#endif // EVE_H_
