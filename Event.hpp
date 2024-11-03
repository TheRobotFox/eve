#ifndef EVENT_H_
#define EVENT_H_

#include <any>
#include <bits/move_only_function.h>
#include <cassert>
#include <functional>
#include <print>
#include <source_location>
#include <stacktrace>
#include <string>
#include <type_traits>

namespace eve::event {

    class EventAny
    {
    public:
        using id = std::string;
        using generic = std::true_type;
        using handle_copy = std::true_type;
        using handle_ref = std::true_type;

    protected:
        id name;
        std::any data;

        template<class C, class T>
        auto get(this const C& self) -> T
        {
            return C::template unwrap<T>();
        }

        template<class T>
        auto unwrap() -> T
        {
            return std::any_cast<T>(data);
        }
    public:
        template<class T>
        void apply(std::move_only_function<void(T)> fn_copy)
        {
            function(get<T>());
        }
        template<class T>
        void apply(std::move_only_function<void(const T&)> fn_ref)
        {
            function(get<const T&>());
        }
    };

    class Debug : public EventAny
    {
        std::stacktrace creation=std::stacktrace::current();
        template<class T>
        auto unwrap() -> T
        {
            try{
                return std::any_cast<T>(data);
            } catch(std::bad_any_cast e) {
                std::println("[ERROR] in {}: std::any_cast<{}> failed handeling event for class {} at {}:{}!", std::source_location::current().function_name(),
                             typeid(T).name(),
                             src.function_name(),
                             src.file_name(),
                             src.line());
                std::println("Event({}) has type {} but expected {}", ev.name, ev.data.type().name(), typeid(T).name());
                std::println("Note: Event created at\n{}", ev.creation);
                exit(5);
            }
        }
    };

}
#endif // EVENT_H_
