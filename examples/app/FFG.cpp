#include "FFG.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
namespace FFG {

#define COMM(msg) std::cout << msg << std::endl; MQTT::publish("KM", msg, 0)

// GPIOs
void Grafcet::initGPIO() {}
void Grafcet::readInputs() {}

// Receptivities
bool Grafcet::T0() const { return true; }
bool Grafcet::T1() const { return true; }

// Actions on steps
void Grafcet::P0() { std::cout << "FFG Dcy" << std::endl; }
void Grafcet::P1() { COMM("Carton");  }

void Grafcet::onConnected(int /*rc*/)
{
    std::cout << "MQTT on connected" << std::endl;
}

void Grafcet::onMessageReceived(const struct mosquitto_message& msg)
{
    std::string message(static_cast<char*>(msg.payload));
    printf("%s %d %s\n", msg.topic, msg.qos, message.c_str());
}

} // namespace FFG

// g++ --std=c++14 -Wall -Wextra -I../../src -I../../src/utils/ FFG.cpp ../../src/utils/MQTT.cpp -o FFG `pkg-config --libs --cflags libmosquitto`
int main()
{
   size_t cycle = 0u;
   FFG::Grafcet g;
   g.connect("localhost", 1883);

   // In the case the Petri net editor changed of topic, you can set the new one.
   // g.topic() = "pneditor/FFG";

   // The loop is for simulating the runtime loop of your task
   while (true)
   {
      std::cout << cycle++ << " =====================================\n";

      g.step();
      g.debug();

      // Let suppose here the time step is 1 Hz.
      std::this_thread::sleep_for(1000ms);
   }

   return 0;
}
