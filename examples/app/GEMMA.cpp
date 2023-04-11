#include "GEMMA.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
namespace GEMMA {

// GS
bool Grafcet::T0() const { return (!AU) && rearm; }
bool Grafcet::T1() const { return AU; }

// GPN
bool Grafcet::T2() const { return X[9] && dcy; }
bool Grafcet::T3() const { return acy; }
bool Grafcet::T4() const { return true; } // 30s/X3

// GC
bool Grafcet::T5() const { return validation; }
bool Grafcet::T6() const { return true; }
bool Grafcet::T7() const { return automatic && validation; }
bool Grafcet::T8() const { return temp; }
bool Grafcet::T9() const { return manual && validation; }
bool Grafcet::T10() const { return manual && validation; }
bool Grafcet::T11() const { return automatic && validation; }

// Actions on steps
void Grafcet::P0()
{
  // GPN
  X[2] = true;
  X[3] = X[4] = false;

  // GC
  X[5] = true;
  X[6] = X[7] = X[8] = X[9] = X[10] = false;
}

void Grafcet::P1() { }
void Grafcet::P2() { }
void Grafcet::P3() { std::cout << "Chauffe Four + Moteur Avance Droit" << std::endl; }
void Grafcet::P4() { std::cout << "Chauffe Four + Moteur Avance Droit" << std::endl; }
void Grafcet::P5() { }
void Grafcet::P6() { std::cout << "Moteur Avance Gauche" << std::endl; }
void Grafcet::P7() { }
void Grafcet::P8() { std::cout << "Chauffe Four" << std::endl; }
void Grafcet::P9() { std::cout << "GPN" << std::endl; }
void Grafcet::P10()
{
    if (dr) { std::cout << "Moteur Avance Droit" << std::endl; }
    if (ga) { std::cout << "Moteur Avance Gauche" << std::endl; }

  // GPN
  X[2] = true;
  X[3] = X[4] = false;
}

void Grafcet::onConnected(int /*rc*/)
{
    std::cout << "MQTT on connected" << std::endl;
    subscribe("GRAFCET/GEMMA", 0);
}

void Grafcet::onMessageReceived(const struct mosquitto_message& msg)
{
    std::string message(static_cast<char*>(msg.payload));
    if (message == "AU")
    {
        std::cout << "RECEIVED: AU\n";
        AU = true;
    }
    else if (message == "0AU")
    {
        std::cout << "RECEIVED: !AU\n";
        AU = false;
    }
    else if (message == "rearm")
    {
        std::cout << "RECEIVED: rearm\n";
        rearm = true;
    }
    else if (message == "dcy")
    {
        std::cout << "RECEIVED: dcy\n";
        dcy = true;
    }
    else if (message == "acy")
    {
        std::cout << "RECEIVED: acy\n";
        acy = true;
    }
    else if (message == "validation")
    {
        std::cout << "RECEIVED: validation\n";
        validation = true;
    }
    else if (message == "manual")
    {
        std::cout << "RECEIVED: manual\n";
        manual = true;
    }
    else if (message == "automatic")
    {
        std::cout << "RECEIVED: automatic\n";
        automatic = true;
    }
    else if (message == "temp")
    {
        std::cout << "RECEIVED: temp\n";
        temp = true;
    }
    else if (message == "dr")
    {
        std::cout << "RECEIVED: dr\n";
        dr = true;
    }
    else if (message == "ga")
    {
        std::cout << "RECEIVED: ga\n";
        ga = true;
    }
    else
    {
        printf("NVALID MESSAGE: %s %d %s\n", msg.topic, msg.qos, message.c_str());
    }
    //received = true;
}

} // namespace GEMMA

// g++ --std=c++14 -Wall -Wextra -Isrc -Isrc/utils/ GEMMA.cpp src/utils/MQTT.cpp -o GEMMA `pkg-config --libs --cflags libmosquitto`
int main()
{
    //size_t cycle = 0u;
    GEMMA::Grafcet g;
    g.connect("localhost", 1883);

    while (true)
    {
       //if (g.received)
       {
          //std::cout << cycle++ << " =====================================\n";
          //g.received = false;

          g.step();
          //g.debug();
       }
      std::this_thread::sleep_for(1000ms);
    }

    return 0;
}
