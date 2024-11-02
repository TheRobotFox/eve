#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "Event.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <list>
#include <ranges>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace eve::reactive{
    template<typename... Tp>
    class ReactiveStatic;
    class Reactive;
}
namespace eve {

    struct ReactiveInterface
    {
        virtual auto notify(Event ev) -> void = 0;
    };

    struct TimedEvent {
        std::chrono::time_point<std::chrono::steady_clock> creation;
        std::chrono::microseconds timeout;
        bool persistent;
        Event ev;
    };

    class EventQueue
    {
        friend reactive::ReactiveStatic<>;
        friend reactive::Reactive;
        using Event = Event;
        std::queue<Event> queue;
        std::list<TimedEvent> intervals;
        std::unordered_map<Event::identifier,
            std::unordered_set<ReactiveInterface*>> handlers;

        auto addHandler(Event::identifier ev, ReactiveInterface *self)
        {
            if(!handlers.contains(ev)) handlers[ev]={self};
            else handlers[ev].insert(self);
        }
    public:
        template<typename T>
        auto addEvent(Event::identifier event, T &&data)
        {
            queue.emplace(event, std::forward<T>(data));
        }
        template<typename T>
        auto addEvent(Event::identifier event, T &&data, std::chrono::milliseconds delay, bool persistent=false)
        {
            intervals.emplace_back(std::chrono::steady_clock::now(),
                              delay, persistent, Event{event, std::forward<T>(data)});
        }

        auto runAll() -> void {
            intervals.remove_if([this](TimedEvent &te) -> bool{
                auto now = std::chrono::steady_clock::now();
                if(te.creation+te.timeout>now) return false;
                if(te.persistent){
                    queue.push(te.ev);
                    te.creation += te.timeout;
                    return false;
                }
                queue.push(std::move(te.ev));
                return true;
            });
            while(!queue.empty()){
                Event &e = queue.front();
                if(!handlers.contains(e.name) || handlers[e.name].size()==0){ std::cout << "[WARNING] Unhandled Event: " << e.name << std::endl;
                    goto next;
                }
                std::ranges::for_each(handlers[e.name], [&e](ReactiveInterface *r){r->notify(e);});

            next:
                queue.pop();
            }
        }
    };

}

#endif // EVENTQUEUE_H_
