#include "EventQueue.hpp"
#include "Reactive.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>

class Test : public eve::reactive::Reactive
{
    auto onTest(const int &i) -> void {
        std::cout << "Recv Test: " << i << std::endl;
    }
    public:
        Test(eve::EventQueue &queue)
            : Reactive(queue)
        {
            addHandle("Test", &Test::onTest);
            addHandle("Pi", &Test::onTest);
            // m_queue.addEvent("Pi", "c");
        }
};

int main()
{
    using namespace std::chrono_literals;
    eve::EventQueue q;

    q.addEvent("Test", 6, 400ms);
    q.addEvent("PI", 8);

    while(true){
        q.runAll();
    }
}
