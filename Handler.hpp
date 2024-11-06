#ifndef HANDLER_H_
#define HANDLER_H_


#include "EveDef.hpp"
#include <functional>

namespace eve::modules {
    template<class Q> requires features::HandleEvent<Q> && event::Event<Q>
    class Handler
    {
        static_assert(false, "Handler requires specialisation on Event eg. Data or Generic");
    };

    template<class Q> requires features::HandleEvent<Q> && event::DataEvent<Q>
    class Handler<Q>
    {
        using Event = Q::Event;
        std::unordered_map<typename Event::id, std::move_only_function<void(const typename Event::value_type&)>> m_ref;
        std::unordered_map<typename Event::id, std::move_only_function<void(typename Event::value_type)>> m_copy;
    };
    template<class Q> requires features::HandleEvent<Q> && event::GenericEvent<Q>
    class Handler<Q>
    {
        using Event = Q::Event;
        // std::unordered_map<typename Event::id, // tempated std::mo_fn wrapper or smth
        //     std::move_only_function<Event>> m_actors;
    public:
    };
}
#endif // HANDLER_H_
