#ifndef DEBUG_H_
#define DEBUG_H_

#include "EveDef.hpp"
#include "Event.hpp"
#include "Reactive.hpp"

namespace eve {
    namespace debug {
        class EventAny : public eve::event::EventAny
        {
            const std::stacktrace m_creation=std::stacktrace::current();
            const std::source_location m_src;
        public:
            template<class T>
            auto getData(std::optional<std::source_location> handler_location={}) const -> const T
            {
                try{
                    return event::EventAny::getData<T>();
                } catch(std::bad_any_cast e) {
                    if(auto src = handler_location){
                        std::println("[ERROR] Failed handeling for class {} at {}:{}!\n Generic Event has wrong Type ",
                                    src->function_name(),
                                    src->file_name(),
                                     src->line());
                    } else {
                        std::println("[ERROR] in {}: std::any_cast<{}> failed unknown Handler!",
                                    std::source_location::current().function_name(),
                                    typeid(T).name());
                    }
                    std::println("Event({}) has type {} but expected {}", name, data.type().name(), typeid(T).name());
                    std::println("Note: Event spawned in {} at {}:{}", m_src.function_name(), m_src.file_name(), m_src.line());
                    std::println("Note: Event creation stacktrace\n{}", m_creation);
                    exit(5);
                }
            }
            using id = std::string;
            using generic = std::true_type;
                template<class T>
                EventAny(id &&name, T &&data, std::source_location src = std::source_location::current())
                    : event::EventAny(name, std::forward<T>(data)), m_src(src)
                {}
        };


    }

    namespace reactive {
        
        template<EveType EV> requires std::same_as<typename EV::Event, debug::EventAny>
        struct Reactive<EV> : private _Reactive<EV>{
            using Event = EV::Event;
            template<typename C>
            auto addHandler(this C& self, Event::id &&evName, void(C::* fn)(),
                           std::source_location src = std::source_location::current()) -> void
            {
                static_cast<_Reactive<EV>&>(self).addHandler(evName, std::move([&self, fn, src](const Event &/**/)->void{(self.*fn)(src);}));
            }
            template<class C, typename A>
            auto addHandler(this C& self,
                            EV::Event::id &&evName, void(C::* fn)(A),
                            std::source_location src = std::source_location::current()) -> void
            {
                static_cast<_Reactive<EV>&>(self).addHandler(std::move(evName), std::move([&self, fn, src](const Event &ev){(self.*fn)(std::forward<A>(ev.template getData<A>(src)));}));
            }
            Reactive(EV &e)
                : _Reactive<EV>(e)
            {}
        };
    }
}


#endif // DEBUG_H_
