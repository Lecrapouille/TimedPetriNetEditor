// This file has been generated and you should avoid editing it.
// Note: the code generator is still experimental !

#ifndef GENERATED_GRAFCET_HOWARD2_HPP
#  define GENERATED_GRAFCET_HOWARD2_HPP

#  include <iostream>
#  include "MQTT.hpp"

namespace Howard2 {

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
    Grafcet() { initGPIO(); reset(); }

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
        X[0] = true;  // P0
        X[1] = false; // P1
        X[2] = false; // P2
        X[3] = false; // P3
        X[4] = false; // P4
        doActions();
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void step()
    {
        readInputs();
        setTransitions();
        setSteps();
        doActions();
    }

private:

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void initGPIO();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void readInputs();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void doActions()
    {
        if (X[0]) { P0(); }
        if (X[1]) { P1(); }
        if (X[2]) { P2(); }
        if (X[3]) { P3(); }
        if (X[4]) { P4(); }
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void setTransitions()
    {
        T[0] = X[0] && T0();
        T[1] = X[1] && T1();
        T[2] = X[2] && X[4] && T2();
        T[3] = X[3] && T3();
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
            X[3] = true;
        }
        if (T[1])
        {
            X[1] = false;
            X[2] = true;
        }
        if (T[2])
        {
            X[2] = false;
            X[4] = false;
            X[0] = true;
        }
        if (T[3])
        {
            X[3] = false;
            X[4] = true;
        }
    }

private: // You have to implement the following methods in the C++ file

    //-------------------------------------------------------------------------
    //! \brief Transition 0: "T0"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T0() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 1: "T1"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T1() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 2: "T2"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T2() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 3: "T3"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T3() const;
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 0: P0
    //-------------------------------------------------------------------------
    void P0();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 1: P1
    //-------------------------------------------------------------------------
    void P1();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 2: P2
    //-------------------------------------------------------------------------
    void P2();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 3: P3
    //-------------------------------------------------------------------------
    void P3();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 4: P4
    //-------------------------------------------------------------------------
    void P4();

public:

    bool dcy = false;
    bool motor_ready = false;
    bool cardboard_arrived = false;

private:

    static const size_t MAX_STEPS = 5u;
    static const size_t MAX_TRANSITIONS = 4u;

    //! \brief Steps
    bool X[MAX_STEPS];
    //! \brief Transitions
    bool T[MAX_TRANSITIONS];
    //! \brief MQTT topic to communicate with the Petri net editor
    std::string m_topic = "pneditor/Howard2";
};

} // namespace Howard2
#endif // GENERATED_GRAFCET_HOWARD2_HPP
