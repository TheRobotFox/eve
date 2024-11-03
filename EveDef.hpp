#ifndef EVEDEF_H_
#define EVEDEF_H_

#include <concepts>
#include <queue>
namespace eve {

    template<class T>
    concept Event = requires(T ev) {
        typename T::id;
        typename T::generic;
    };

    template<class Event>
    class EveCore
    {
        std::queue<Event> m_queue;

    public:
        using EventID = typename Event::identifier;

        template<typename T>
        auto addEvent(EventID event, T &&data)
        {
            m_queue.emplace(event, std::forward<T>(data));
        }
    };

    template<class Event, class T>
    concept Module = requires(T c, std::queue<Event> &queue, EveCore<Event> core)
    {
        T::T(std::ref(core));
        {c.m_queue} -> std::same_as<decltype(queue)&>;
        {c.run()} -> std::same_as<void>;
    };


    template<class Event, Module<Event>... Mp>
    class EventQueue : public EveCore<Event>, public Mp...
    {
    public:
        EventQueue()
            : Mp(*this) ...
        {}
        auto run() -> void
        {
            (Mp::run(), ...);
        }
    };

    using Eve = EventQueue<Timeouts, Async, Handlers>;

}
#endif // EVEDEF_H_
