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
        {ev.name} -> std::same_as<const typename T::id&>;
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
    protected:
        std::any data;

    public:
        using id = std::string;
        using generic = std::true_type;

        const id name;

        template<class T>
        auto getData() const -> const T
        {
            return std::any_cast<T>(data);
        }
        template<class T>
        EventAny(id name, T &&data) : data(std::move(data)), name(std::move(name))
        {}
    };

}
#endif // EVENT_H_
