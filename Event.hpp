#ifndef EVENT_H_
#define EVENT_H_

#include <any>
#include <bits/move_only_function.h>
#include <cassert>
#include <print>
#include <source_location>
#include <stacktrace>
#include <string>
#include <type_traits>
#include <utility>

namespace eve::event {

    template<class T>
    concept Event = requires(T ev) {
        {ev.getName()} -> std::same_as<const typename T::id&>;
    };

    template<class E>
    concept DataEvent = Event<E> && requires(E ev, E::id name) {
        {ev.getData()} -> std::same_as<const typename E::value_type&>;
    };

    template<class E>
    concept GenericEvent = Event<E> && !DataEvent<E> && requires(E ev) {
        {ev.template getData()};
    };
    template<class E, class T>
    E construct(typename E::id &name, T data){return {name, data};}
    template<class E>
    concept EventConstruction = (DataEvent<E> &&
        requires(E::id name, E::value_type &&data){
            E{name, data}; // Trivial construction})
        }) || (GenericEvent<E> && requires(E::id name){
                typename E::id;
                // construct<E>(name, 0);
        });

    class EventAny
    {
    public:
        using id = std::string;
        using generic = std::true_type;

    protected:
        id name;
        std::any data;

    public:
        auto getName() const -> const id&{
            return name;
        }
        template<class T>
        auto getData() const -> const T
        {
            return std::any_cast<T>(data);
        }
        template<class T>
        EventAny(id name, T &&data) : name(std::move(name)), data(std::move(data))
        {}
    };

    class Debug : public EventAny
    {
        std::stacktrace creation=std::stacktrace::current();
        template<class T>
        auto get(std::optional<std::source_location> handler_location={}) -> const T&
        {
            try{
                return EventAny::getData<T>();
            } catch(std::bad_any_cast e) {
                if(auto src = handler_location){
                std::println("[ERROR] in {}: std::any_cast<{}> failed handeling event for class {} at {}:{}!", std::source_location::current().function_name(),
                             typeid(T).name(),
                             src->function_name(),
                             src->file_name(),
                             src->line());
                } else {
                    std::println("[ERROR] in {}: std::any_cast<{}> failed unknown Handler! Please refer to stacktrace",
                                 std::source_location::current().function_name(),
                                 typeid(T).name());
                }
                std::println("Event({}) has type {} but expected {}", getName(), data.type().name(), typeid(T).name());
                std::println("Note: Event created at\n{}", creation);
                exit(5);
            }
        }
    public:
        using id = std::string;
        using generic = std::true_type;
    };



}
#endif // EVENT_H_
