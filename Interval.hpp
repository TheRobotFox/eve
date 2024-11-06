#ifndef INTERVAL_H_
#define INTERVAL_H_

#include "EventQueue.hpp"
#include <chrono>
#include <list>
namespace eve {

    namespace features {
        template<class E>
        concept SpawnInterval = requires(E &eve, E::Event ev, std::chrono::milliseconds ms) {
            eve.addInterval(ev, ms, false);
        };
    }

    namespace modules {

        template<class Q> requires features::SpawnEvent<Q>
        class Interval
        {
            using Event = typename Q::Event;

            struct TimedEvent {
                std::chrono::time_point<std::chrono::steady_clock> creation;
                std::chrono::microseconds timeout;
                bool persistent;
                Event ev;
            };

            std::list<TimedEvent> m_timeouts;
        public:

            auto addInterval(Event &&event, std::chrono::milliseconds delay, bool persistent=false)
            {
                m_timeouts.emplace_back(std::chrono::steady_clock::now(),
                                        delay, persistent, std::forward<Event>(event));
            }
            auto run(Q &spawn) -> void {
                m_timeouts.remove_if([&spawn](TimedEvent &te) -> bool{
                    auto now = std::chrono::steady_clock::now();
                    if(te.creation+te.timeout>now) return false;
                    if(te.persistent){
                        spawn.addEvent(te.ev);
                        te.creation += te.timeout;
                        return false;
                    }
                    spawn.addEvent(std::move(te.ev));
                    return true;
                });
            }
        };
    }
}

#endif // INTERVAL_H_
