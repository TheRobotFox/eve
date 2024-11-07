#ifndef ASYNC_H_
#define ASYNC_H_

#include "EveDef.hpp"
#include "Event.hpp"
#include "EventQueue.hpp"
#include "Feature.hpp"
#include <functional>
#include <future>
#include <list>
#include <memory>

namespace eve {

    // Helper to bind Class constructor
    template<class C, class... Args>
    constexpr auto construct(Args... args)
    {
        return [...args=std::forward<Args>(args)]<class T>(T && data){return C{args..., std::move(data)};};
    }

    namespace modules {

        template<EveType EV>
        class Async;

        template<EveType EV> requires event::DataEvent<typename EV::Event>
        class Async<EV> : public Require<EV, features::Emit>
        {
            using Event = EV::Event;
            using T = Event::value_type;
            using F = std::move_only_function<Event(T &&data)>;
            std::list<std::pair<F, std::future<T>>> m_tasks;

        public:
            auto addAsync(F &&construct, std::future<T> &&future)
            {
                m_tasks.push_back({std::move(construct), std::move(future)});
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


        template<EveType EV> requires event::GenericEvent<typename EV::Event>
        class Async<EV> : public Require<EV, features::Emit>
        {
            using Event = EV::Event;

            struct FutureInterface {
                virtual auto ready() const -> bool = 0;
                virtual auto getEvent() -> Event = 0;
                virtual ~FutureInterface() = default;
            };

            template<typename T, class F>
            struct FutureHandle : public FutureInterface
            {
                F construct;
                std::future<T> future;
                auto ready() const -> bool override
                {
                    return future.wait_for(std::chrono::microseconds(0))!=std::future_status::timeout;
                }
                auto getEvent() -> Event override
                {
                    return construct(future.get());
                }
                FutureHandle(F &&construct, std::future<T> &&future)
                    : construct(std::move(construct)),
                    future(std::move(future))
                {}
            };

            std::list<std::unique_ptr<FutureInterface>> m_tasks;

        public:
                template<typename T> requires requires(Event::id name, T data){Event{name, std::move(data)};}
            auto addAsync(Event::id &&name, std::future<T> &&future)
            {
                addAsync(std::move([name=std::move(name)](T &&data)->Event{return Event{std::move(name), std::move(data)};}), std::move(future));
            }
            template<typename T, class F> requires requires(F &&fn, T data){fn(std::move(data));}
            auto addAsync(F &&construct, std::future<T> &&future)
            {
                m_tasks.emplace_back(std::make_unique<FutureHandle<T, F>>(std::move(construct), std::move(future)));
            }
            // callback
            auto emit(EV &spawn) -> void
            {
                m_tasks.remove_if([&spawn](auto &fh){
                    if(fh->ready()){
                        spawn.addEvent(fh->getEvent());
                        return true;
                    }
                    return false;
                });
            }
        };


    }
}
#endif // ASYNC_H_
