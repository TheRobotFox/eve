#ifndef EVENT_H_
#define EVENT_H_

#include <any>
#include <cassert>
#include <print>
#include <source_location>
#include <stacktrace>
#include <string>

struct Event
{
    using identifier = std::string;
    identifier name;
    std::any data;

    #ifdef NDEBUG
    #else
    std::stacktrace creation=std::stacktrace::current();
    #endif
};

#endif // EVENT_H_
