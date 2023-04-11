#include "Howard2.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
namespace Howard2 {

class Grafcet::Context
{
public:

    void readInputs() {}

public:

    bool dcy = false;
    bool motor_ready = false;
    bool cardboard_arrived = false;
};

//Grafcet::Grafcet() { m_context = std::make_unique<Grafcet::Context>(); reset(); }

// GPIOs
//void Grafcet::initGPIO() {}
//void Grafcet::readInputs() {}

// Receptivities
bool Grafcet::T0() const { std::cout << "T0: DCY: " << m_context->dcy << std::endl; return m_context->dcy; }
bool Grafcet::T1() const { std::cout << "T1: Motor ready: " << m_context->motor_ready << std::endl; return m_context->motor_ready; }
bool Grafcet::T2() const { std::cout << "T2: New cycle: 1" << std::endl; return true; }
bool Grafcet::T3() const { std::cout << "T3: Carton arrived: " << m_context->cardboard_arrived << std::endl; return m_context->cardboard_arrived; }

// Actions on steps
void Grafcet::P0() { std::cout << "Do P0 actions" << std::endl; }
void Grafcet::P1() { std::cout << "Do P1 actions" << std::endl; }
void Grafcet::P2() { std::cout << "Do P2 actions" << std::endl; }
void Grafcet::P3() { std::cout << "Do P3 actions" << std::endl; }
void Grafcet::P4() { std::cout << "Do P4 actions" << std::endl; m_context->cardboard_arrived = false; }

void Grafcet::onConnected(int /*rc*/)
{
    std::cout << "MQTT on connected" << std::endl;
    subscribe("GRAFCET/Howard2", 0);
}

void Grafcet::onMessageReceived(const struct mosquitto_message& msg)
{
    std::string message(static_cast<char*>(msg.payload));
    if (message == "dcy")
    {
        std::cout << "RECEIVED: DCY\n";
        m_context->dcy = true;
    }
    else if (message == "motor_ready")
    {
        std::cout << "RECEIVED: Motor ready\n";
        m_context->motor_ready = true;
    }
    else if (message == "cardboard_arrived")
    {
        std::cout << "RECEIVED: Cardboard arrived\n";
        m_context->cardboard_arrived = true;
    }
    else
    {
        printf("NVALID MESSAGE: %s %d %s\n", msg.topic, msg.qos, message.c_str());
    }
}

} // namespace Howard2

// g++ --std=c++14 -Wall -Wextra -I../../src -I../../src/utils/ Howard2.cpp ../../src/utils/MQTT.cpp -o Howard2 `pkg-config --libs --cflags libmosquitto`
int main()
{
   size_t cycle = 0u;
   //Howard2::Grafcet g(std::make_unique<Howard2::Grafcet::Context>());
   Howard2::Grafcet g;
   g.connect("localhost", 1883);

   // In the case the Petri net editor changed of topic, you can set the new one.
   // g.topic() = "pneditor/Howard2";

   // The loop is for simulating the runtime loop of your task
   while (true)
   {
      std::cout << cycle++ << " =====================================\n";

      g.step();
      g.debug();

      // Let suppose here the time step is 0.5 Hz.
      std::this_thread::sleep_for(1000ms);
   }

   return 0;
}
