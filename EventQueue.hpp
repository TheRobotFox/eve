#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "Event.hpp"
#include <algorithm>
#include <chrono>
#include <concepts>
#include <future>
#include <iostream>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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

    struct FutureInterface {
        virtual auto ready() -> bool = 0;
        virtual auto getEvent() -> Event = 0;
    };

    template<typename T>
    struct FutureHandle : public FutureInterface
    {
        Event::identifier event;
        std::future<T> future;
        auto ready() -> bool override
        {
            return future.wait_for(std::chrono::microseconds(0))!=std::future_status::timeout;
        }
        auto getEvent() -> Event override
        {
            return {event, future.get()};
        }
    };
    class Handleing
    {
        friend reactive::ReactiveStatic<>;
        friend reactive::Reactive;
        using Event = Event;
        std::list<std::unique_ptr<FutureInterface>> m_futures;
        std::unordered_map<Event::identifier,
            std::unordered_set<ReactiveInterface*>> m_handlers;

        auto addHandler(Event::identifier ev, ReactiveInterface *self)
        {
            if(!m_handlers.contains(ev)) m_handlers[ev]={self};
            else m_handlers[ev].insert(self);
        }
    public:
        template<typename T>
        auto addEvent(Event::identifier event, T &&data, std::chrono::milliseconds delay, bool persistent=false)
        {
            m_timeouts.emplace_back(std::chrono::steady_clock::now(),
                              delay, persistent, Event{event, std::forward<T>(data)});
        }
        template<typename T>
        auto addAsync(Event::identifier event, std::future<T> future)
        {
            m_futures.emplace_back(new FutureHandle<T>(event, future));
        }

        auto run() -> void {
            m_timeouts.remove_if([this](TimedEvent &te) -> bool{
                auto now = std::chrono::steady_clock::now();
                if(te.creation+te.timeout>now) return false;
                if(te.persistent){
                    m_queue.push(te.ev);
                    te.creation += te.timeout;
                    return false;
                }
                m_queue.push(std::move(te.ev));
                return true;
            });
            while(!m_queue.empty()){
                Event &e = m_queue.front();
                if(!m_handlers.contains(e.name) || m_handlers[e.name].size()==0){ std::cout << "[WARNING] Unhandled Event: " << e.name << std::endl;
                    goto next;
                }
                std::ranges::for_each(m_handlers[e.name], [&e](ReactiveInterface *r){r->notify(e);});

            next:
                m_queue.pop();
            }
        }
    };

    template<EVModule... Fs>
    struct Eve : public Fs...
    {
        auto run()
        {
            Fs::run()
        }
    };

}

#endif // EVENTQUEUE_H_
