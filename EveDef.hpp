#ifndef EveDef_H_
#define EveDef_H_

#include "EventQueue.hpp"
#include <concepts>
namespace eve {

    template<class EV>
    concept EveType = requires(EV &e) {
        typename EV::Event;
        {e.step()} -> std::same_as<void>;

        // Costum stages
        {e.collect()} -> std::same_as<void>;
        {e.handle()} -> std::same_as<void>;
        {e.other()} -> std::same_as<void>;
    };

    template<template<class Q> class T, class Q>
    concept Module = requires(T<Q> &c, Q &features)
    {
        {c.run(features)} -> std::same_as<void>;
    };

}

#endif // EveDef_H_
