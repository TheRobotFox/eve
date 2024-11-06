#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include "Event.hpp"
#include <algorithm>
#include <concepts>
#include <queue>
#include <utility>
#include <optional>

namespace eve {

    namespace features {
        template<class T>
        concept SpawnEvent = event::Event<typename T::Event> && requires(T& se, T::Event &&ev)
        {
            {se.addEvent(std::move(ev))} -> std::same_as<void>;
        };

        template<class T>
        concept HandleEvent = event::Event<typename T::Event> && requires(T &he) {
            {he.peek()} -> std::same_as<std::optional<const typename T::Event&>>;
        };

        template<class T>
        concept ConsumeEvent = event::Event<typename T::Event> && requires(T &ce) {
            ce.pop();
        };
    }
    namespace event_queue{

        template<class Q>
        concept EventQueue = features::SpawnEvent<Q> && features::HandleEvent<Q> && features::ConsumeEvent<Q> && requires(Q &q){
            {q.empty()} -> std::same_as<bool>;
        };

        template<class Queue>
        class EventQueueCore
        {
            Queue m_queue;

        public:
            using Event = Queue::value_type;
            using EventID = typename Event::id;

            auto addEvent(const Event &event) -> void
            {
                m_queue.emplace(event);
            }
            auto peek() -> std::optional<const Event&>
            {
                if(m_queue.empty()) return {};
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
            EventQueueCore(const EventQueueCore&) = delete;
            EventQueueCore() = default;

        };
        template<event::Event E>
        class DefaultQueue : public EventQueueCore<std::queue<E>>
        {};

        template<event::Event E, typename Q = std::priority_queue<std::pair<int, E>,
                                        std::vector<std::pair<int, E>>,
                                        decltype([](auto &a, auto &b){return a.first>b.first;})>>
        class PriorityQueue : public EventQueueCore<Q>
        {
        public:
            using Event = E;
            auto addEvent(Event &&event, int prio=0) -> void
            {
                EventQueueCore<Q>::addEvent(std::make_pair(prio, event));
            }
                auto peek() -> std::optional<const Event&>
            {
                if(EventQueueCore<Q>::empty()) return {};
                return EventQueueCore<Q>::peek().second;
            }
        };

    }

}

#endif // EVENTQUEUE_H_
