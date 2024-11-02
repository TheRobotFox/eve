#ifndef REACTIVE_H_
#define REACTIVE_H_

#include "Event.hpp"
#include "EventQueue.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <memory>
#include <source_location>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cassert>

namespace eve::reactive {

    template<typename... Tp>
    class ReactiveStatic : public ReactiveInterface
    {
        using EventID = EventQueue::Event::identifier;
        // std::unordered_map<EventQueue::Event::identifier, > // TODO dynamic handlers
        EventQueue &m_queue;
        std::tuple<std::pair<EventID, std::function<void(const Tp&)>>...> m_layout;

        template<class F>
        auto forLayout(F &&fn)
        {
            std::apply([&fn](auto&&... pair){
                (fn(pair.first, pair.second), ...);
            }, m_layout);
        }

        public:
            template<class...>
            ReactiveStatic(EventQueue &queue,
                        std::pair<EventID, std::function<void(const Tp&)>>... layout)
                : m_queue(queue), m_layout(std::forward(layout...))
            {
                forLayout([this](const EventID &name, auto& /* ignore */){
                    m_queue.handlers[name].insert(this);
                });
            }

            ~ReactiveStatic()
            {
                forLayout([this](const EventID &name, auto& /* ignore */){
                    m_queue.handlers[name].erase(this);
                });
            }

            auto notify(Event ev) -> void override
            {
                forLayout([&ev]<class T>(const EventID &name, const std::function<void(const T&)> &f){
                        if(name!=ev.name) return;
                        f(std::any_cast<const T&>(ev));
                    });
            }

    };


    struct HandlerInterface
    {
        virtual auto handle(const Event &ev) -> void = 0;
        virtual ~HandlerInterface() = default;
    };
    template<typename T>
    class Handler : public HandlerInterface
    {
        std::vector<std::function<void(T)>> handle_copy;
        std::vector<std::function<void(const T&)>> handle_ref;

        public:
    #ifdef NDEBUG
            auto handle(const Event &ev) -> void override
            {
                using namespace std::ranges;

                T d = std::any_cast<T>(ev.data);

                for_each(handle_ref, [&d](auto f){f(d);});
                for_each(handle_copy, [d](auto f){f(std::move(d));});
            }
    #else
            Handler(std::source_location parrent=std::source_location::current())
            : src(parrent){}
            const std::source_location src;
        auto handle(const Event &ev) -> void override
        {
            try{
                using namespace std::ranges;
                T d = std::any_cast<T>(ev.data);

                for_each(handle_ref, [&d](auto f){f(d);});
                for_each(handle_copy, [d](auto f){f(std::move(d));});

            } catch(std::bad_any_cast e) {
                std::println("[ERROR] in {}: std::any_cast<{}> failed handeling event for class {} at {}:{}!", std::source_location::current().function_name(), typeid(T).name(), src.function_name(), src.file_name(), src.line());
                std::println("Event({}) has type {} but expected {}", ev.name, ev.data.type().name(), typeid(T).name());
                std::println("Note: Event created at\n{}", ev.creation);
                exit(5);
            }
        }
    #endif
            auto addHandle(std::function<void(T)> &&f)
            {
                handle_copy.push_back(f);
            }
            auto addHandle(std::function<void(const T&)> &&f)
            {
                handle_ref.push_back(f);
            }

    };

    class Reactive : public ReactiveInterface
    {
        protected:
            using EventID = EventQueue::Event::identifier;
        private:
            using Handlers = std::unordered_map<EventID, std::unique_ptr<HandlerInterface>>;
            Handlers handlers;
        protected:

            EventQueue &m_queue;
            Reactive(EventQueue &queue)
                : m_queue(queue)
            {}
            ~Reactive()
            {
                std::ranges::for_each(handlers, [this](const EventID &eid){
                    m_queue.handlers[eid].erase(this);
                }, &Handlers::value_type::first);
            }

        public:
            #ifdef NDEBUG
            template<typename C, typename A>
            auto addHandle(this C& self, EventID &&evName, void(C::* fn)(A)) -> void
            {
                self.m_queue.addHandler(evName, &self);
                self.addHandle(std::move(evName), std::function([&self, fn](A&& val){(self.*fn)(std::forward<A>(val));}));
            }
            template<typename A>
            auto addHandle(EventID &&evName, std::function<void(A)> &&fn) -> void
            {
                using T = std::remove_cvref_t<A>;
                if(!handlers.contains(evName)) handlers[evName] = std::make_unique<Handler<T>>();

                auto *handler = dynamic_cast<Handler<T>*>(handlers[evName].get());
                handler->addHandle(std::function<void(A)>(fn));
            }
            #else

            template<typename C, typename A>
            auto addHandle(this C& self, EventID &&evName, void(C::* fn)(A), std::source_location src=std::source_location::current()) -> void
            {
                self.m_queue.addHandler(evName, &self);
                self.addHandle(std::move(evName), std::function([&self, fn](A&& val){(self.*fn)(std::forward<A>(val));}), src);
            }
            template<typename A>
            auto addHandle(EventID &&evName, std::function<void(A)> &&fn, std::source_location src = std::source_location::current()) -> void
            {
                using T = std::remove_cvref_t<A>;
                if(!handlers.contains(evName)) handlers[evName] = std::make_unique<Handler<T>>(src);

                auto *handler = dynamic_cast<Handler<T>*>(handlers[evName].get());
                handler->addHandle(std::function<void(A)>(fn));
            }
            #endif
            auto notify(Event ev) -> void override
            {
                std::ranges::for_each(handlers, [&ev](auto &pair){
                    if(ev.name==pair.first) pair.second->handle(ev);
                });
            }

    };
}

#endif // REACTIVE_H_
