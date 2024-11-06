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
    struct Require : public features<EV>... {
        template<class C>
        auto run(this C& self, EV &ev){
            (static_cast<features<EV>&>(self).template runFeature<C>(ev), ...);
        }
    };
}

#endif // FEATURE_H_
