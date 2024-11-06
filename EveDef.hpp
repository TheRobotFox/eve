#ifndef EveDef_H_
#define EveDef_H_

#include "EventQueue.hpp"
#include <concepts>
namespace eve {

    template<class eve>
    concept EveType = event_queue::EventQueue<typename eve::Queue>;


    template<template<class Q> class T, class Q>
    concept Module = requires(T<Q> &c, Q &features)
    {
        {c.run(features)} -> std::same_as<void>;
    };

}

#endif // EveDef_H_
