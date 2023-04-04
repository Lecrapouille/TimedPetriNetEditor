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

#include "utils/MQTT.hpp"
#include <iostream>

//------------------------------------------------------------------------------
MQTT::MQTT()
{
    MQTT::Mosquitto::instance();
}

//------------------------------------------------------------------------------
MQTT::~MQTT()
{
    if (m_mosquitto != nullptr)
    {
        mosquitto_destroy(m_mosquitto);
    }
}

//------------------------------------------------------------------------------
bool MQTT::connect(std::string const& addr, size_t const port)
{
    if (m_mosquitto == nullptr)
    {
        m_mosquitto = mosquitto_new(nullptr, true, this);
        if (m_mosquitto == nullptr)
        {
            std::cerr << "MQTT Error: cannot malloc mosquitto" << std::endl;
            return false;
        }
    }
    else
    {
        mosquitto_disconnect(m_mosquitto);
    }

    mosquitto_connect_callback_set(m_mosquitto, on_connected_wrapper);
    mosquitto_disconnect_callback_set(m_mosquitto, on_disconnected_wrapper);
    mosquitto_publish_callback_set(m_mosquitto, on_published_wrapper);
    mosquitto_subscribe_callback_set(m_mosquitto, on_subscribed_wrapper);
    mosquitto_unsubscribe_callback_set(m_mosquitto, on_unsubscribed_wrapper);
    mosquitto_message_callback_set(m_mosquitto, on_message_received_wrapper);

    int rc = mosquitto_connect(m_mosquitto, addr.c_str(), int(port), 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "MQTT connection error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }

    rc = mosquitto_loop_start(m_mosquitto);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        std::cerr << "MQTT loop_start error: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }

    return true;
}

bool MQTT::publish(std::string const& topic, std::string const& payload, int qos)
{
    int rc = mosquitto_publish(m_mosquitto, nullptr, topic.c_str(),
                               payload.length(), payload.data(), qos, false);
	if( rc != MOSQ_ERR_SUCCESS)
    {
		std::cerr << "MQTT Error publishing: " << mosquitto_strerror(rc)
                  << std::endl;
        return false;
    }
    return true;
}

bool MQTT::unsubscribe(std::string const& topic)
{
    if (topic.size() == 0u)
        return true;

    std::cout << "MQTT unsubscribe to '" << topic << "'" << std::endl;
    int rc = mosquitto_unsubscribe(m_mosquitto, nullptr, topic.c_str());
    if (rc != MOSQ_ERR_SUCCESS)
    {
		std::cerr << "MQTT Error unsubscribing: " << mosquitto_strerror(rc)
                  << std::endl;
        return false;
    }
    return true;
}

bool MQTT::subscribe(std::string const& topic, int qos)
{
    std::cout << "MQTT subscribe to '" << topic << "'" << std::endl;
    int rc = mosquitto_subscribe(m_mosquitto, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS)
    {
		std::cerr << "MQTT Error subscribing: " << mosquitto_strerror(rc)
                  << std::endl;
        return false;
    }
    return true;
}