#include "GC.hpp"
#include "GS.hpp"
#include "GFN.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

static bool AU = false;
static bool rearm = false;
static bool dcy = false;
static bool acy = false;
static bool validation = false;
static bool manual = false;
static bool automatic = false;
static bool temp_atteinte = false;
static bool dr = false;
static bool ga = false;

GS::Grafcet gs;  // GRAFCET de securite
GC::Grafcet gc;  // GRAFCET de controle
GFN::Grafcet gfn; // GRAFCET de fonctionement normal

// GRAFCET de securite
namespace GS {
  // Forcage GFN et GC
  void Grafcet::P0() { gfn.reset(); gc.reset(); }
  // Rearmement et désactivation bouton d'arret d'urgence
  bool Grafcet::T0() const { return AU && rearm; }
  // Attente du bouton d'arret durgence
  void Grafcet::P1() { }
  // Bouton d'arret d'urgence
  bool Grafcet::T1() const { return !AU; }
  // MQTT
  void Grafcet::onConnected(int) { std::cout << "GS connected to MQTT broker" << std::endl; }
  void Grafcet::onMessageReceived(const struct mosquitto_message&) {}
} // namespace GS

// GRAFCET de controle
namespace GC {
  // Verification manuel en desordre
  void Grafcet::P0()
  {
    if (dr) { std::cout << "Avance Convoyeur" << std::endl; }
    if (ga) { std::cout << "Recule Convoyeur" << std::endl; }
    gfn.reset();
  }
  // Mode automatique + validation
  bool Grafcet::T0() const { return automatic && validation; }
  // GRAFCET de fonctionement normal
  void Grafcet::P1() { std::cout << "GPN" << std::endl; }
  // Mode manuel + validation
  bool Grafcet::T1() const { return manual && validation; }
  // Chauffer four
  void Grafcet::P2() { std::cout << "Chauffe Four" << std::endl; }
  // Mode manuel + validation
  bool Grafcet::T2() const { return manual && validation; }
  // Choix mode
  void Grafcet::P3() { }
  // Temperature four atteinte
  bool Grafcet::T3() const { return temp_atteinte; }
  // Recule Convoyeur
  void Grafcet::P4() { std::cout << "Recule Convoyeur" << std::endl; }
  // Mode automatique + validation
  bool Grafcet::T4() const { return automatic && validation; }
  // Init
  void Grafcet::P5() { }
  // Validation
  bool Grafcet::T5() const { return validation; }
  // Attendre 30 s
  bool Grafcet::T6() const { return true; }
  // MQTT
  void Grafcet::onConnected(int) { std::cout << "GC connected to MQTT broker" << std::endl; }
  void Grafcet::onMessageReceived(const struct mosquitto_message&) {}
} // namespace GC

// GRAFCET de fonctionement normal
namespace GFN {
  // Repos
  void Grafcet::P2() { }
  // Attente départ cycle et attente OK du GRAFCET de controle
  bool Grafcet::T2() const { return gc.states()[1] && dcy; }
  // Chauffe et recule convoyeur
  void Grafcet::P1() { std::cout << "Chauffe four + Recule convoyeur" << std::endl; }
  // Attente 30 secondes
  bool Grafcet::T1() const { return true; }
  // Chauffe et avance convoyeur
  void Grafcet::P0() { std::cout << "Chauffe four + Avance convoyeur" << std::endl; }
  // Arret cycle
  bool Grafcet::T0() const { return acy; }
  // MQTT
  void Grafcet::onConnected(int) { std::cout << "GFN connected to MQTT broker" << std::endl; }
  void Grafcet::onMessageReceived(const struct mosquitto_message&) {}
} // namespace GFN

// *****************************************************************************
//! \brief
// *****************************************************************************
class GEMMA: public MQTT
{
public:

    GEMMA();
    void step();

private: // MQTT

    //-------------------------------------------------------------------------
    //! \brief Callback when this class is connected to the MQTT broker.
    //-------------------------------------------------------------------------
    virtual void onConnected(int /*rc*/) override;

    //-------------------------------------------------------------------------
    //! \brief Callback when this class is has received a new message from the
    //! MQTT broker.
    //-------------------------------------------------------------------------
    virtual void onMessageReceived(const struct mosquitto_message& message) override;

private:

    GS::Grafcet m_gcs;  // GRAFCET de securite
    GC::Grafcet m_gcc;  // GRAFCET de controle
    GFN::Grafcet m_gfn; // GRAFCET de fonctionement normal
};

GEMMA::GEMMA()
{
    connect("localhost", 1883);
    gs.connect("localhost", 1883);
    gc.connect("localhost", 1883);
    gfn.connect("localhost", 1883);
}

void GEMMA::step()
{
//std::cout << "Manual: " << manual << " Auto: " << automatic << " Valid " << validation << std::endl;

    gs.step(); //gs.debug();
    gc.step(); //gc.debug();
    gfn.step(); //gfn.debug();
}

void GEMMA::onConnected(int /*rc*/)
{
    std::cout << "GEMMA connected to MQTT broker" << std::endl;
    subscribe("GEMMA/AU", 0);
    subscribe("GEMMA/Dcy", 0);
    subscribe("GEMMA/Acy", 0);
    subscribe("GEMMA/Rearm", 0);
    subscribe("GEMMA/Valid", 0);
    subscribe("GEMMA/Manu", 0); // "GEMMA/mode"
    subscribe("GEMMA/Temp", 0);
    subscribe("GEMMA/Avance", 0);
    subscribe("GEMMA/Recule", 0);
}

void GEMMA::onMessageReceived(const struct mosquitto_message& msg)
{
    std::string message(static_cast<char*>(msg.payload));
    std::string topic(static_cast<char*>(msg.topic));
    //std::cout << "Topic: " << topic << std::endl;

    if (topic == "GEMMA/AU")
    {
        AU = !!(message[0] - '0');
    }
    else if (topic == "GEMMA/Dcy")
    {
        dcy = !!(message[0] - '0');
    }
    else if (topic == "GEMMA/Acy")
    {
        acy = !!(message[0] - '0');
    }
    else if (topic == "GEMMA/Rearm")
    {
        rearm = !!(message[0] - '0');
    }
    else if (topic == "GEMMA/Valid")
    {
        validation = !!(message[0] - '0');
    }
#if 0
    else if (topic == "Mode")
    {
        if (message == "manual")
        {
            manual = true;
            automatic = false;
        }
        else if (message == "automatic")
        {
            manual = false;
            automatic = true;
        }
        else
        {
            manual = false;
            automatic = false;
        }
    }
    else if (topic == "Temperature")
    {
        temp_atteinte = (message[0] > 30);
    }
#else
    else if (topic == "GEMMA/Manu")
    {
        if (message == "1")
        {
            manual = true;
            automatic = false;
        }
        else if (message == "0")
        {
            manual = false;
            automatic = true;
        }
        else
        {
            manual = false;
            automatic = false;
        }
    }
    else if (topic == "GEMMA/Temp")
    {
        temp_atteinte = !!(message[0] - '0');
    }
#endif
    else if (topic == "GEMMA/Avance")
    {
        if (message[0] - '0')
        {
            dr = true;
            ga = false;
        }
        else
        {
            dr = false;
            ga = false;
        }
    }
    else if (topic == "GEMMA/Recule")
    {
        if (message[0] - '0')
        {
            dr = false;
            ga = true;
        }
        else
        {
            dr = false;
            ga = false;
        }
    }
    else
    {
        printf("INVALID MESSAGE: %s %d %s\n", msg.topic, msg.qos, message.c_str());
    }
}

// g++ --std=c++14 -Wall -Wextra -I../../src -I../../src/utils/ GEMMA.cpp ../../src/utils/MQTT.cpp -o GEMMA `pkg-config --libs --cflags libmosquitto`
int main()
{
    GEMMA g;

    while (true)
    {
        g.step();
        std::this_thread::sleep_for(1000ms);
    }

    return 0;
}
