#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>
#include "Eve.hpp"
#include "EveDef.hpp"
#include "Reactive.hpp"

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
            this->addHandle("Test", &Test::onTest);
            this->addHandle("Pi", &Test::onTest);
            // m_queue.addEvent("Pi", "c");
        }
};

int main()
{
    using namespace std::chrono_literals;
    eve::Default q;

    q.addInterval(eve::event::EventAny{"Test", 6}, 400ms);
    q.addEvent({"PI", 8});

    Test t(q);

    while(true){
        q.run();
    }
}
