// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//=============================================================================
// TimedPetriNetEditor: example of the runtime in a ROS 2 node (not built by
// this project: it needs rclcpp; build it in a colcon package linking
// libTimedPetriRuntime and libTimedPetriNet).
//
// A timer of the node calls step(): the marking lives in the executor of
// ROS, the functions of the transitions send ROS actions and return at once
// (they are synchronous); the results of the actions come back by inject()
// on input transitions ("grasp_ok", "grasp_ko"), so no thread waits.
//=============================================================================

#include "PetriNet/PetriNet.hpp"
#include "Runtime/Runtime.hpp"

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

using namespace std::chrono_literals;

class Cell : public rclcpp::Node
{
public:

    Cell() : rclcpp::Node("cell")
    {
        bool springify;
        tpne::loadFromFile(m_net, declare_parameter("net", "cell.json"), springify);

        tpne::Runtime::Options options;
        options.clock = [this]() { return now().seconds(); };
        m_runtime = std::make_unique<tpne::Runtime>(m_net, std::make_shared<tpne::InlineExecutor>(),
                                                    options);

        // Send the goal of the action and return: its result is injected.
        m_runtime->on("grasp", [this]() { sendGrasp(); });
        m_instance = m_runtime->instantiate("cell", {}, { { "Done", tpne::Success } });
        m_timer = create_wall_timer(10ms, [this]() {
            if (!m_runtime->step(*m_instance))
                RCLCPP_INFO(get_logger(), "cell: %s", m_net.colors.name(m_instance->result()).c_str());
        });
    }

private:

    void sendGrasp()
    {
        // ... async_send_goal(); in its result callback:
        //   m_runtime->inject(*m_instance, succeeded ? "grasp_ok" : "grasp_ko");
    }

    tpne::Net m_net;
    std::unique_ptr<tpne::Runtime> m_runtime;
    std::shared_ptr<tpne::Instance> m_instance;
    rclcpp::TimerBase::SharedPtr m_timer;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Cell>());
    rclcpp::shutdown();
}
