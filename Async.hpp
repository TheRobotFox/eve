#ifndef ASYNC_H_
#define ASYNC_H_

#include "EveDef.hpp"
#include "Event.hpp"
#include "EventQueue.hpp"
#include "Feature.hpp"
#include <future>
#include <list>
#include <memory>

namespace eve::modules {

    template<EveType EV> requires event::EventConstruction<typename EV::Event>
    class Async{
        public:
        auto run(Q&_) -> void {
            static_assert(false, "Async requires specialisation on Event eg. Data or Generic");
        }
    };

    template<EveType EV> requires event::EventConstruction<typename EV::Event>
            && event::DataEvent<typename EV::Event>
    class Async<EV> : public Require<EV, features::Emit>
    {
        using Event = EV::Event;
        using T = Event::value_type;
        std::list<std::pair<typename Event::id, std::future<T>>> m_tasks;

    public:
        auto addAsync(const Event::id &eventName, std::future<T> &&future)
        {
            m_tasks.push_back({eventName, std::move(future)});
        }
        // callback
        auto emit(EV &spawn) -> void
        {
            std::erase_if(m_tasks.begin(), m_tasks.end(), [&spawn](auto &pair){
                if(pair.second.wait_for(std::chrono::microseconds(0))!=std::future_status::timeout){
                    // future done!
                    spawn.addEvent({pair.first, pair.second.get()});
                    return true;
                }
                return false;
            });
        }
    };


    template<EveType EV> requires  event::GenericEvent<typename EV::Event>
        && event::EventConstruction<typename EV::Event>
    class Async<EV> : public Require<EV, features::Emit>
    {
        using Event = EV::Event;


        struct FutureInterface {
            virtual auto ready() -> bool = 0;
            virtual auto getEvent() -> Event = 0;
        };

        template<typename T>
        struct FutureHandle : public FutureInterface
        {
            Event::id event;
            std::future<T> future;
            auto ready() -> bool override
            {
                return future.wait_for(std::chrono::microseconds(0))!=std::future_status::timeout;
            }
            auto getEvent() -> Event override
            {
                return {std::move(event), future.get()};
            }
        };

        std::list<std::unique_ptr<FutureInterface>> m_tasks;

    public:
        template<typename T>
        auto addAsync(const Event::id &eventName, std::future<T> &&future)
        {
            m_tasks.emplace_back(std::make_unique<FutureHandle<T>>(eventName, std::move(future)));
        }
        // callback
        auto emit(EV &spawn) -> void
        {
            std::erase_if(m_tasks.begin(), m_tasks.end(), [&spawn](auto &fh){
                if(fh.ready()){
                    spawn.addEvent(fh.get());
                    return true;
                }
                return false;
            });
        }
    };


}
#endif // ASYNC_H_
