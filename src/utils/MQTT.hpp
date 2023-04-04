//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef MQTT_HPP
#  define MQTT_HPP

#  include <mosquitto.h>
#  include <string>

// *****************************************************************************
//! \brief Base MQTT class based on the mosquitto implementation.
//! See https://github.com/eclipse/mosquitto
//! See https://www.howtoforge.com/how-to-install-mosquitto-mqtt-message-broker-on-debian-11/
// *****************************************************************************
class MQTT
{
public:

    MQTT();
    virtual ~MQTT();
    bool connect(std::string const& addr, size_t const port);
    bool subscribe(std::string const& topic, int qos);
    bool unsubscribe(std::string const& topic);
    bool publish(std::string const& topic, std::string const& payload, int qos);

protected:

    struct mosquitto* mosquitto() { return m_mosquitto; }

private:

    virtual void onConnected(int /*rc*/) = 0;
    virtual void onDisconnected(int /*rc*/) {}
    virtual void onPublished(int /*mid*/) {}
    virtual void onSubscribed(int /*mid*/, int /*qos_count*/, const int */*granted_qos*/) {}
    virtual void onUnsubscribed(int /*mid*/) {}
    virtual void onMessageReceived(const struct mosquitto_message& message) = 0;

    static void on_connected_wrapper(struct mosquitto* /*mosqitto*/, void* userdata, int rc)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onConnected(rc);
    }

    static void on_disconnected_wrapper(struct mosquitto* /*mosqitto*/, void* userdata, int rc)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onDisconnected(rc);
    }

    static void on_published_wrapper(struct mosquitto* /*mosqitto*/, void* userdata, int mid)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onPublished(mid);
    }

    static void on_subscribed_wrapper(struct mosquitto* /*mosqitto*/, void *userdata, int mid, int qos_count, const int *granted_qos)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onSubscribed(mid, qos_count, granted_qos);
    }

    static void on_unsubscribed_wrapper(struct mosquitto* /*mosqitto*/, void *userdata, int mid)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onUnsubscribed(mid);
    }

    static void on_message_received_wrapper(struct mosquitto* /*mosqitto*/, void *userdata, const struct mosquitto_message *message)
    {
        MQTT* m = static_cast<MQTT*>(userdata);
        m->onMessageReceived(*message);
    }

    class Mosquitto
    {
    public:

        static Mosquitto& instance() { static Mosquitto instance; return instance; }

    private:

        Mosquitto() { mosquitto_lib_init(); }
        ~Mosquitto() { mosquitto_lib_cleanup(); }
    };

private:

    struct mosquitto *m_mosquitto = nullptr;
};

#endif