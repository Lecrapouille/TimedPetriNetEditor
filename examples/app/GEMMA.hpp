// This file has been generated and you should avoid editing it.
// Note: the code generator is still experimental !

#ifndef GENERATED_GRAFCET_GEMMA_HPP
#  define GENERATED_GRAFCET_GEMMA_HPP

#  include <iostream>
#  include "MQTT.hpp"
#include <atomic>

namespace GEMMA {

// *****************************************************************************
//! \brief
// *****************************************************************************
class Grafcet: public MQTT
{
public:

    //-------------------------------------------------------------------------
    //! \brief Restore all states of the GRAFCET to their initial states.
    //-------------------------------------------------------------------------
    Grafcet() { reset(); }

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
        X[0] = true;  // 100
        X[1] = false; // 101
        X[2] = true;  // 1
        X[3] = false; // 2
        X[4] = false; // 3
        X[5] = true;  // 10
        X[6] = false; // 11
        X[7] = false; // 12
        X[8] = false; // 13
        X[9] = false; // 14
        X[10] = false; // 15
        doActions();
    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void step()
    {

        //readInputs();
        setTransitions();
        setSteps();
        doActions();

rearm = dcy = acy = validation = dr = ga = false;
    }

private:

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
        if (X[5]) { P5(); }
        if (X[6]) { P6(); }
        if (X[7]) { P7(); }
        if (X[8]) { P8(); }
        if (X[9]) { P9(); }
        if (X[10]) { P10(); }
    }

    //-------------------------------------------------------------------------
    //! \brief Update transitions is fired when all previous steps (places) are
    //! enabled and when the receptivity of the transition is true.
    //-------------------------------------------------------------------------
    void setTransitions()
    {
        T[0] = X[0] && T0();
        T[1] = X[1] && T1();
        T[2] = X[2] && T2();
        T[3] = X[3] && T3();
        T[4] = X[4] && T4();
        T[5] = X[5] && T5();
        T[6] = X[6] && T6();
        T[7] = X[7] && T7();
        T[8] = X[8] && T8();
        T[9] = X[9] && T9();
        T[10] = X[7] && T10();
        T[11] = X[10] && T11();
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
        }
        if (T[1])
        {
            X[1] = false;
            X[0] = true;
        }
        if (T[2])
        {
            X[2] = false;
            X[3] = true;
        }
        if (T[3])
        {
            X[3] = false;
            X[4] = true;
        }
        if (T[4])
        {
            X[4] = false;
            X[2] = true;
        }
        if (T[5])
        {
            X[5] = false;
            X[6] = true;
        }
        if (T[6])
        {
            X[6] = false;
            X[7] = true;
        }
        if (T[7])
        {
            X[7] = false;
            X[8] = true;
        }
        if (T[8])
        {
            X[8] = false;
            X[9] = true;
        }
        if (T[9])
        {
            X[9] = false;
            X[10] = true;
        }
        if (T[10])
        {
            X[7] = false;
            X[10] = true;
        }
        if (T[11])
        {
            X[10] = false;
            X[6] = true;
        }
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
    //! \brief Transition 4: "T4"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T4() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 5: "T5"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T5() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 6: "T6"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T6() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 7: "T7"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T7() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 8: "T8"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T8() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 9: "T9"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T9() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 10: "T10"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T10() const;
    //-------------------------------------------------------------------------
    //! \brief Transition 11: "T11"
    //! \return true if the transition is enabled.
    //-------------------------------------------------------------------------
    bool T11() const;
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 0: 100
    //-------------------------------------------------------------------------
    void P0();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 1: 101
    //-------------------------------------------------------------------------
    void P1();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 2: 1
    //-------------------------------------------------------------------------
    void P2();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 3: 2
    //-------------------------------------------------------------------------
    void P3();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 4: 3
    //-------------------------------------------------------------------------
    void P4();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 5: 10
    //-------------------------------------------------------------------------
    void P5();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 6: 11
    //-------------------------------------------------------------------------
    void P6();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 7: 12
    //-------------------------------------------------------------------------
    void P7();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 8: 13
    //-------------------------------------------------------------------------
    void P8();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 9: 14
    //-------------------------------------------------------------------------
    void P9();
    //-------------------------------------------------------------------------
    //! \brief Do actions associated with the step 10: 15
    //-------------------------------------------------------------------------
    void P10();

private:

    //! \brief Number of Steps in the GRAFCET (aka Places in Petri net)
    static const size_t MAX_STEPS = 11u;
    //! \brief Number of Transitions in the GRAFCET
    static const size_t MAX_TRANSITIONS = 12u;
    //! \brief Steps
    bool X[MAX_STEPS];
    //! \brief Transitions
    bool T[MAX_TRANSITIONS];
    //! \brief MQTT topic to communicate with the Petri net editor
    std::string m_topic = "pneditor/GEMMA";

    bool AU = false;
    bool rearm = false;
    bool dcy = false;
    bool acy = false;
    bool validation = false;
    bool manual = false;
    bool automatic = false;
    bool temp = false;
    bool dr = false;
    bool ga = false;

    bool KM1 = false;
    bool KM2 = false;
    bool KM3 = false;
public:
std::atomic<bool> received{false};
};

} // namespace GEMMA
#endif // GENERATED_GRAFCET_GEMMA_HPP
