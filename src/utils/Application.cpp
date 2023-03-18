//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Application.hpp"

// -----------------------------------------------------------------------------
Application::GUI::GUI(Application& application, std::string const& name,
                      sf::Color const& color)
    : background_color(color), m_application(application),
      m_renderer(application.renderer()), m_name(name)
{}

// -----------------------------------------------------------------------------
Application::Application(uint32_t const width, uint32_t const height,
                         std::string const& title)
{
    m_renderer.create(sf::VideoMode(width, height), title);
    setFramerate(60);
}

// -----------------------------------------------------------------------------
Application::~Application()
{
    halt();
}

// -----------------------------------------------------------------------------
void Application::setFramerate(unsigned int const limit)
{
    m_renderer.setFramerateLimit(limit);
    m_framerate = float(limit);
}

// -----------------------------------------------------------------------------
void Application::halt()
{
    // Clear the satck of GUIs
    std::stack<Application::GUI*>().swap(m_stack);
    // Stop the SFML renderer
    m_renderer.close();
}

// -----------------------------------------------------------------------------
// FIXME recurrsion ConcreteGUI::onDeactivate() { ConcreteGUI::push(NewGUI); }
void Application::push(Application::GUI& gui)
{
    GUI* current_gui = peek();
    if ((current_gui != nullptr) && (current_gui != &gui))
    {
        current_gui->onDeactivate();
    }
    m_stack.push(&gui);
    gui.onCreate();
    // Uncomment for debuging
    // printStack();
}

// -----------------------------------------------------------------------------
bool Application::pop()
{
    GUI* gui = peek();
    if (gui == nullptr)
        return false;

    m_stack.pop();
    gui->onRelease();

    gui = peek();
    if (gui != nullptr)
    {
        gui->m_closing = gui->m_halting = false;
        gui->onActivate();
    }

    // Uncomment for debuging
    // printStack();
    return true;
}

// -----------------------------------------------------------------------------
void Application::loop(Application::GUI& starting_gui)
{
    push(starting_gui);
    loop();
}

// -----------------------------------------------------------------------------
void Application::loop()
{
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;
    const sf::Time time_per_frame = sf::seconds(1.0f / m_framerate);

    while (m_renderer.isOpen())
    {
        GUI* gui = peek();
        if (gui == nullptr)
            return ;

        // Process events at fixed time steps
        timeSinceLastUpdate += clock.restart();
        while (timeSinceLastUpdate > time_per_frame)
        {
            timeSinceLastUpdate -= time_per_frame;
            gui->onHandleInput();
            gui->onUpdate(time_per_frame.asSeconds());
        }

        // Rendering
        m_renderer.clear(gui->background_color);
        gui->onDraw();
        m_renderer.display();

        // Halt the application
        if (gui->m_halting)
        {
            halt();
        }
        // Close the current GUI
        else if (gui->m_closing)
        {
            gui->m_closing = false;
            if (!pop())
            {
                m_renderer.close();
            }
        }
    }
}

// -----------------------------------------------------------------------------
bool Application::screenshot(std::string const& screenshot_path)
{
    sf::Texture t;
    t.create(m_renderer.getSize().x, m_renderer.getSize().y);
    t.update(m_renderer);

    sf::Image screenCap = t.copyToImage();
    return screenCap.saveToFile(screenshot_path);
}

// -----------------------------------------------------------------------------
void Application::printStack()
{
    std::cout << "Application stack of GUIs:" << std::endl;
    printStack(m_stack);
}

// -----------------------------------------------------------------------------
void Application::printStack(std::stack<Application::GUI*>& stack)
{
    if (stack.empty())
        return;
    Application::GUI* gui = stack.top();
    stack.pop();
    std::cout << "  " << gui->m_name << std::endl;
    printStack(stack);
    stack.push(gui);
}