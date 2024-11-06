#ifndef REACTIVE_H_
#define REACTIVE_H_

#include "Event.hpp"
#include "EventQueue.hpp"
#include <algorithm>
#include <concepts>
#include <functional>
#include <memory>
#include <source_location>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <cassert>

#include "EveDef.hpp"
#include "Feature.hpp"

namespace eve {

    template<event::Event E>
    struct ReactiveInterface
    {
        virtual auto notify(const E &ev) -> void = 0;
    };

    namespace features {
        template<class EV>
        concept Listeners = EveType<EV> && requires(EV &r, EV::Event::id ev, ReactiveInterface<typename EV::Event> *actor) {
            r.setListen(ev, actor);
            r.unsetListen(ev, actor);
        };
    }

    namespace modules {

            // TODO React Static -> templated<clsas... ReactImpls>
        template<EveType EV>
        class React : public Require<EV, features::Handle>
        {
        public:
            using Event = EV::Event;

            auto unsetListen(Event::id ev, ReactiveInterface<Event>* actor)
            {
                if(!m_actors.contains(ev)) m_actors[ev]={};
                else m_actors[ev].erase(actor);
            }
            auto setListen(Event::id ev, ReactiveInterface<Event>* actor)
            {
                if(!m_actors.contains(ev)) m_actors[ev]={actor};
                else m_actors[ev].insert(actor);
            }
            // callback
            auto handle(const Event &ev) -> void
            {
                if(m_actors.contains(ev.name)){
                    std::ranges::for_each(m_actors[ev.name], [&ev](auto a){a->notify(ev);});
                }
            }
        private:
            std::unordered_map<typename Event::id,
                std::unordered_set<ReactiveInterface<Event>*>> m_actors;
        };
    }

    namespace reactive {

        // template<typename... Tp>
        // class ReactiveStatic : public ReactiveInterface
        // {
        //     using EventID = EventQueue::Event::identifier;
        //     // std::unordered_map<EventQueue::Event::identifier, > // TODO dynamic handlers
        //     EventQueue &m_queue;
        //     std::tuple<std::pair<EventID, std::function<void(const Tp&)>>...> m_layout;

        //     template<class F>
        //     auto forLayout(F &&fn)
        //     {
        //         std::apply([&fn](auto&&... pair){
        //             (fn(pair.first, pair.second), ...);
        //         }, m_layout);
        //     }

        //     public:
        //         template<class...>
        //         ReactiveStatic(EventQueue &queue,
        //                     std::pair<EventID, std::function<void(const Tp&)>>... layout)
        //             : m_queue(queue), m_layout(std::forward(layout...))
        //         {
        //             forLayout([this](const EventID &name, auto& /* ignore */){
        //                 m_queue.handlers[name].insert(this);
        //             });
        //         }

        //         ~ReactiveStatic()
        //         {
        //             forLayout([this](const EventID &name, auto& /* ignore */){
        //                 m_queue.handlers[name].erase(this);
        //             });
        //         }

        //         auto notify(Event ev) -> void override
        //         {
        //             forLayout([&ev]<class T>(const EventID &name, const std::function<void(const T&)> &f){
        //                     if(name!=ev.name) return;
        //                     f(std::any_cast<const T&>(ev));
        //                 });
        //         }

        // };

        template<event::Event E>
        struct HandlerInterface
        {
            virtual auto handle(const E &ev) -> void = 0;
            virtual ~HandlerInterface() = default;
        };

        template<event::Event E, typename T>
        class Handler : public HandlerInterface<E>
        {
            std::vector<std::move_only_function<void(const T&)>> m_callbacks;
            std::vector<std::move_only_function<void(T)>> m_callbacks2;

        public:
            auto handle(const E &ev) -> void override
            {
                for_each(m_callbacks, [&ev](auto &f){f(ev.getData());});
                for_each(m_callbacks2, [&ev](auto &f){f(ev.getData());});
            }
            auto addHandle(std::move_only_function<void(T)> &&f)
            {
                m_callbacks2.push_back(f);
            }
            auto addHandle(std::move_only_function<void(const T&)> &&f)
            {
                m_callbacks.push_back(f);
            }

        };

        template<EveType EV> requires features::Listeners<EV> && features::SpawnEvent<EV>
        class _Reactive : public ReactiveInterface<typename EV::Event>
        {
            using Event = EV::Event;
            using Handlers = std::unordered_map<typename Event::id, std::move_only_function<void(const Event &)>>;
            Handlers handlers;
        protected:

                EV &m_eve;
                _Reactive(EV &e)
                    : m_eve(e)
                {}
                ~_Reactive()
                {
                    std::ranges::for_each(handlers, [this](const Event::id &eid){
                        m_eve.unsetListen(eid, this);
                    }, &Handlers::value_type::first);
                }

            public:
                template<typename C, typename A>
                auto addHandle(this C& self, Event::id &&evName, void(C::* fn)(A)) -> void
                {
                    self.m_eve.setListen(evName, &self);
                    self.handlers[std::move(evName)] = std::move([&self, fn](const Event &ev)->void{(self.*fn)(std::forward<A>(ev.template getData<A>()));});
                }
                auto notify(const Event &ev) -> void override
                {
                    std::ranges::for_each(handlers, [&ev](auto &pair){
                        if(ev.name==pair.first) pair.second(ev);
                    });
                }
        };

        template<EveType EV>
        struct Reactive : public _Reactive<EV> {
                Reactive(EV &e)
                    : _Reactive<EV>(e)
                {}
        };


        // Debug specialisation
        template<EveType EV> requires std::same_as<typename EV::Queue::Event, event::Debug>
        class Reactive<EV> : public _Reactive<EV>{
        public:
            template<class C, typename A>
            auto addHandle(this C& self,
                            EV::Event::id &&evName, void(C::* fn)(A),
                            std::source_location src = std::source_location::current()) -> void
            {
                self.m_eve.setListen(evName, &self);
                self.handlers.emplace({std::move(evName),
                            std::function([&self, fn, src](const A&& ev){(self.*fn)(std::forward<A>(ev.template get<A>(src)));})});
            }
            Reactive(EV &e)
                : _Reactive<EV>(e)
            {}
        };
    }
}

#endif // REACTIVE_H_
