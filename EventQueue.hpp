#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "Event.hpp"
#include <algorithm>
#include <concepts>
#include <iostream>
#include <queue>
#include <string_view>
#include <utility>
#include <optional>

namespace eve {

    namespace features {

        template<class T>
        concept SpawnEvent = event::Event<typename T::Event> && requires(T& se, const T::Event &&ev)
        {
            {se.addEvent(std::move(ev))} -> std::same_as<void>;
            // se.template emplaceEvent;
        };
        template<SpawnEvent S>
        struct Emit
        {
            template<class C>
            auto runFeature(S& spawn){
                static_cast<C&>(*this).emit(spawn);
            }
        };

        template<class Q>
        concept PeekEvent = event::Event<typename Q::Event> && requires(Q &q) {
            {q.peek()} -> std::same_as<const typename Q::Event&>;
            {q.empty()} -> std::same_as<bool>;
        };
        template<PeekEvent H>
        struct Handle
        {
            template<class C>
            auto runFeature(H& events){
                static_cast<C&>(*this).handle(events.peek());
            }
        };

        template<class T>
        concept ConsumeEvent = event::Event<typename T::Event> && requires(T &ce) {
            ce.pop();
        };
        template<class HC> requires PeekEvent<HC> && ConsumeEvent<HC>
        struct Filter
        {
            template<class C>
            auto runFeature(HC& events){
                if(static_cast<bool>(static_cast<C&>(*this).filter(events.peek())))
                    events.pop();
            }
        };
        template<class Q>
        concept EveCore = SpawnEvent<Q> && PeekEvent<Q>;
    }

    namespace event_queue{

        template<class Queue>
        class EventQueue
        {
            Queue m_queue;

        public:
            using Event = Queue::value_type;

            template<class... Args>
            auto emplaceEvent(Args... args) -> void
            {
                m_queue.emplace(std::forward<Args>(args)...);
            }
            auto addEvent(const Event &event) -> void
            {
                m_queue.push(event);
            }
            auto peek() -> const Event&
            {
                return m_queue.front();
            }
            auto pop() -> void
            {
                m_queue.pop();
            }
            auto empty() -> bool
            {
                return m_queue.empty();
            }
            EventQueue(const EventQueue&) = delete;
            EventQueue() = default;

            auto step() -> void {}
            auto collect() -> void {}
            auto handle() -> void {}
            auto other() -> void {}
        };
        template<event::Event E>
        struct DefaultQueue : public EventQueue<std::queue<E>>
        {
            using Event = E;
        };

        template<event::Event E, typename Q = std::priority_queue<std::pair<int, E>,
                                        std::vector<std::pair<int, E>>,
                                        decltype([](auto &a, auto &b){return a.first>b.first;})>>
        struct PriorityQueue : public EventQueue<Q>
        {
            using Event = E;
            auto addEvent(Event &&event, int prio=0) -> void
            {
                EventQueue<Q>::addEvent(std::make_pair(prio, event));
            }
            auto peek() -> std::optional<const Event&>
            {
                if(EventQueue<Q>::empty()) return {};
                return EventQueue<Q>::peek().second;
            }
        };
    }

}

#endif // EVENTQUEUE_H_
