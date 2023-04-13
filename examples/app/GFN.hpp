// This file has been generated and you should avoid editing it.
// Note: the code generator is still experimental !

#ifndef GENERATED_GRAFCET_GFN_HPP
#  define GENERATED_GRAFCET_GFN_HPP

#  include <iostream>
#  include "MQTT.hpp"

namespace GFN {

// *****************************************************************************
//! \brief
// *****************************************************************************
class Grafcet: public MQTT
{
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

    //-------------------------------------------------------------------------
    //! \brief Transmit to the Petri net editor all transitions that have been
    //! fired.
    //-------------------------------------------------------------------------
    void publish()
    {
        static char message[MAX_TRANSITIONS + 1u] = { 'T' };

        for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
            message[i + 1u] = T[i];

        MQTT::publish(topic().c_str(), std::string(message, MAX_TRANSITIONS + 1u), 0);
    }

public:

    //-------------------------------------------------------------------------
    //! \brief Restore all states of the GRAFCET to their initial states.
    //-------------------------------------------------------------------------
    Grafcet() { reset(); }

    //-------------------------------------------------------------------------
    //! \brief Return the MQTT topic to talk with the Petri net editor.
    //! Call Grafcet grafcet
    //-------------------------------------------------------------------------
    std::string& topic() { return m_topic; }

    //-------------------------------------------------------------------------
    //! \brief Print values of transitions and steps
    //-------------------------------------------------------------------------
    void debug() const
    {
       std::cout << "Transitions:" << std::endl;
       for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
       {
          std::cout << "  Transition[" << i << "] = " << T[i]
                    << std::endl;
       }

       std::cout << "Steps:" << std::endl;
       for (size_t i = 0u; i < MAX_STEPS; ++i)
       {
          std::cout << "  Step[" << i << "] = " << X[i]
                    << std::endl;
       }
    }

    //-------------------------------------------------------------------------
    //! \brief Desactivate all steps except the ones initially activated
    //-------------------------------------------------------------------------
    void reset()
    {
        X[0] = false; // Chauffe et avance convoyeur
        X[1] = false; // Chauffe et recule convoyeur
        X[2] = true;  // Repos
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void step()
    {
        doActions();
        //readInputs();
        setTransitions();
        setSteps();
    }

private:

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    //void initGPIO();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    //void readInputs();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void doActions()
    {
        if (X[0]) { P0(); }
        if (X[1]) { P1(); }
        if (X[2]) { P2(); }
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void setTransitions()
    {
        T[0] = X[0] && T0();
        T[1] = X[1] && T1();
        T[2] = X[2] && T2();
        publish();
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void setSteps()
    {
        if (T[0])
        {
            X[0] = false;
            X[1] = true;
        }
        if (T[1])
        {
            X[1] = false;
            X[2] = true;
        }
        if (T[2])
        {
            X[2] = false;
            X[0] = true;
        }
    }

private: // You have to implement the following methods in the C++ file

    //-------------------------------------------------------------------------
    //! \brief Transition 0: "Acy"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T0() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 1: "Attente 30s"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T1() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 2: "GC:X14 . Dcy"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T2() const;
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 0: Chauffe et avance convoyeur
    //-------------------------------------------------------------------------
    void P0();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 1: Chauffe et recule convoyeur
    //-------------------------------------------------------------------------
    void P1();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 2: Repos
    //-------------------------------------------------------------------------
    void P2();

private:

    static const size_t MAX_STEPS = 3u;
    static const size_t MAX_TRANSITIONS = 3u;
    //! \brief Steps
    bool X[MAX_STEPS];
    //! \brief Transitions
    bool T[MAX_TRANSITIONS];
    //! \brief MQTT topic to communicate with the Petri net editor
    std::string m_topic = "pneditor/GFN";
};

} // namespace GFN
#endif // GENERATED_GRAFCET_GFN_HPP
