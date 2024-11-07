#ifndef INTERVAL_H_
#define INTERVAL_H_

#include "EveDef.hpp"
#include "EventQueue.hpp"
#include "Feature.hpp"
#include <algorithm>
#include <chrono>
#include <concepts>
#include <list>
#include <vector>
namespace eve {

    namespace modules {

        using IntervalHandle = void*;
    }
    namespace features {
        template<class E>
        concept SpawnInterval = requires(E &eve, E::Event ev, std::chrono::milliseconds ms, modules::IntervalHandle h) {
            {eve.addInterval(ev, ms, false)} -> std::same_as<modules::IntervalHandle>;
            eve.removeInterval(h);
        };
    }

    namespace modules {

        template<EveType EV>
        class Interval : public Require<EV, features::Emit>
        {
            using Event = typename EV::Event;

            struct TimedEvent {
                std::chrono::time_point<std::chrono::steady_clock> creation;
                std::chrono::microseconds timeout;
                bool persistent;
                Event ev;
            };

            std::list<TimedEvent> m_timeouts; // list required for ptr valid
        public:
            auto addInterval(Event &&event, std::chrono::milliseconds delay, bool persistent=false) -> IntervalHandle
            {
                m_timeouts.emplace_back(std::chrono::steady_clock::now(),
                                        delay, persistent, std::forward<Event>(event));
                return &m_timeouts.back();
            }
            auto removeInterval(IntervalHandle h) -> void
            {
                m_timeouts.remove_if([h](const TimedEvent &e){return &e==h;});
            }
            auto emit(EV &spawn) -> void {
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
