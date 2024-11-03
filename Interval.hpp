#ifndef INTERVAL_H_
#define INTERVAL_H_

#include <chrono>
namespace eve::modules {

    template<class Event>
    struct TimedEvent {
        std::chrono::time_point<std::chrono::steady_clock> creation;
        std::chrono::microseconds timeout;
        bool persistent;
        Event ev;
    };


    class Interval
    {
        std::list<TimedEvent> m_timeouts;
    }

}

#endif // INTERVAL_H_
