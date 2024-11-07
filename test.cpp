#include "Debug.hpp"
#include "Eve.hpp"
#include "Event.hpp"
#include "Reactive.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

template<eve::EveType EV>
class Test : public eve::reactive::Reactive<EV>
{
    auto onTest(int i) -> void {
        std::cout << "Recv Test: " << i << std::endl;
    }
    public:
        Test(EV &ev)
            : eve::reactive::Reactive<EV>(ev)
        {
            this->addHandler("Test", &Test::onTest);
        }
};
int main()
{
    using namespace std::chrono_literals;
    eve::Default<eve::debug::EventAny> q;

    Test t(q);

    q.addEvent({"Test", '*'});

    while(true){
        q.run();
    }
}
