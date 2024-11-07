#ifndef FEATURE_H_
#define FEATURE_H_

#include <concepts>
#include "EveDef.hpp"

namespace eve {
    template<template<class> class F, class EV>
    concept Feature = true || requires(F<EV> &feature, EV &e){
        {feature.template runFeature(e)} -> std::same_as<void>;
    };

    template<EveType EV, template<class> class... features> requires (Feature<features, EV> && ...)
    class Require : public features<EV>...
    {
        template<class C, template<class> class  F, template<class> class... Features>
        constexpr auto runNot(EV &ev) -> void
        {
            if constexpr (!(std::same_as<F<EV>, Features<EV>> || ...))
                static_cast<F<EV>&>(*this).template runFeature<C>(ev);
        }
        template<class C>
        constexpr auto runF(EV &/* ignore */) -> void
        {}
        template<class C, template<class> class  F, template<class> class... Features>
        constexpr auto runF(EV &ev) -> void
        {
            ([&ev]<class T>(T& self){
                if constexpr(std::same_as<F<EV>, T>){
                    self.template runFeature<C>(ev);
                }
            }(static_cast<features<EV>&>(*this)), ...);
            runF<C, Features...>(ev);
        }
    public:
        template<class C, template<class> class... Features>
        auto run(this C& self, EV &ev){
            self.template runF<C, Features...>(ev);
        }
        template<class C, template<class> class... Features>
        auto runRest(this C& self, EV &ev){
            (self.template runNot<C, features, Features...>(ev), ...);
        }
    };
}

#endif // FEATURE_H_
