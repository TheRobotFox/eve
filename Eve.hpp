#ifndef EVE_H_
#define EVE_H_

// EV => EventSystem
// EV<Modules> | Modules => Subsystems supplying Features
// Features => tasks eg. SpawnEvent, Handle, Filter, Add Resource ...
//
// Modules<EV> use Require<EV, Features...> to ensure EV supports requested Features.
// Requires<EV, Features> will also use static dispatch to invoke callbacks from derived Module

#include "Async.hpp"
#include "EveDef.hpp"
#include "Event.hpp"
#include "EventQueue.hpp"
#include "Interval.hpp"
#include "Reactive.hpp"

namespace eve {

    template<EveType EV, template<class> class ...Mp>
    requires (Module<Mp, EV> && ...) && features::EveCore<EV>
    class Eve : public EV, public Mp<EV>...
    {
    public:
        using Event = EV::Event;
        auto collect() -> void
        {
            EV::collect();
            (Mp<EV>::template run<Mp<EV>, features::Emit>(*this), ...);
        }
        auto handle() -> void
        {
            EV::handle();
            (Mp<EV>::template run<Mp<EV>, features::Filter,
                                              features::Handle>(*this), ...);
        }
        auto other() -> void
        {
            EV::other();
            (Mp<EV>::template runRest<Mp<EV>, features::Emit,
                                              features::Filter,
                                              features::Handle>(*this), ...);
        }
        auto step() -> void
        {

            collect();
            if(!EV::empty()) handle();
            if(!EV::empty()) EV::pop();

        }
    };

    template<event::Event e>
    using Default = Eve<event_queue::DefaultQueue<e>, modules::Interval, modules::React, modules::Async>;
}


#endif // EVE_H_
