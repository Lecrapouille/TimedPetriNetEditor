// This file has been generated and you should avoid editing it.
// Note: the code generator is still experimental !

#ifndef GENERATED_GRAFCET_HOWARD2_HPP
#  define GENERATED_GRAFCET_HOWARD2_HPP

#  include <iostream>
#  include <memory>
#  include "Context.hpp"
#  include "MQTT.hpp"

namespace Howard2 {

// *****************************************************************************
//! \brief
// *****************************************************************************
class Grafcet: public MQTT
{
public:

    class Context;

    //-------------------------------------------------------------------------
    //! \brief Restore all states of the GRAFCET to their initial states.
    //-------------------------------------------------------------------------
    template<class T = Context>
    Grafcet()
       : m_context(std::make_unique<T>())
    {
       reset();
    }

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
        X[0] = true;  // Pliage carton
        X[1] = false; // Auto-test
        X[2] = false; // P2
        X[3] = false; // Attente Carton
        X[4] = false; // Carton present

        doActions();
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void step()
    {
        m_context->readInputs();
        setTransitions();
        setSteps();
        doActions();
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
    //! \brief Update transitions is fired when all previous steps (places) are
    //! enabled and when the receptivity of the transition is true.
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
    //! \brief Disable all previous steps (places) and enable all following
    //! steps.
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
    //! \brief Return the MQTT topic to talk with the Petri net editor.
    //! Call Grafcet grafcet
    //-------------------------------------------------------------------------
    std::string& topic() { return m_topic; }

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

private: // You have to implement the following methods in the C++ file

    //-------------------------------------------------------------------------
    //! \brief Transition 0: "Dcy"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T0() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 1: "Sys nomimal"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T1() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 2: "Attente depart"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T2() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 3: "Capteur"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T3() const;
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 0: Pliage carton
    //-------------------------------------------------------------------------
    void P0();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 1: Auto-test
    //-------------------------------------------------------------------------
    void P1();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 2: P2
    //-------------------------------------------------------------------------
    void P2();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 3: Attente Carton
    //-------------------------------------------------------------------------
    void P3();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 4: Carton present
    //-------------------------------------------------------------------------
    void P4();

private:

    //! \brief Number of Steps in the GRAFCET (aka Places in Petri net)
    static const size_t MAX_STEPS = 5u;
    //! \brief Number of Transitions in the GRAFCET
    static const size_t MAX_TRANSITIONS = 4u;
    //! \brief Steps
    bool X[MAX_STEPS];
    //! \brief Transitions
    bool T[MAX_TRANSITIONS];
    //! \brief MQTT topic to communicate with the Petri net editor
    std::string m_topic = "pneditor/Howard2";
    std::unique_ptr<Context> m_context;
};

} // namespace Howard2
#endif // GENERATED_GRAFCET_HOWARD2_HPP
