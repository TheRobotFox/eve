#include "Debug.hpp"
#include "Eve.hpp"
#include "Event.hpp"
#include "Reactive.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

using namespace std::chrono_literals;

template<eve::EveType EV>
class Test : public eve::reactive::Reactive<EV>
{
    eve::modules::IntervalHandle inv;
    auto onPi(double i) -> void {
        std::cout << "Fav Number: " << i << std::endl;
    }
    auto onTest(int i) -> void {
        std::cout << "Recv Test: " << i << std::endl;
        ev.removeInterval(inv);
    }
    EV &ev;
    public:
        Test(EV &ev)
            : eve::reactive::Reactive<EV>(ev),
              ev(ev)
        {
            this->addHandler("Test", &Test::onTest);
            this->addHandler("Pi", &Test::onPi);
            inv = ev.addInterval({"Pi", 3.141}, 400ms, true);
        }
};
int main()
{
    eve::Default<eve::debug::EventAny> q;

    Test t(q);

    q.addInterval({"Test", 42}, 1300ms, false);
    q.addAsync([](auto &&data)->eve::debug::EventAny{return {"Pi", data};}, std::async(std::launch::async, []{std::this_thread::sleep_for(1500ms); return 3.0;}));

    while(true){
        q.step();
    }
}
