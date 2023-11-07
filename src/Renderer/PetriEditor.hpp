#ifndef PETRIEDITOR_HPP
#  define PETRIEDITOR_HPP

#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "Renderer/Application.hpp"
#  include "Renderer/Messages.hpp"

namespace tpne {

struct Exporter;

// *****************************************************************************
//! \brief Graphical representation and manipulation of the Petri net using the
//! SFML library for the rendering.
// *****************************************************************************
class Editor: public Application
{
    //friend void menu(Editor& editor);

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] project_path the path of the Petri net fil to load. Let it
    //! dummy if you do not want to open a file.
    //--------------------------------------------------------------------------
    Editor(size_t const width, size_t const height, std::string const& title,
           std::string const& project_path = {});

public:

    // ************************************************************************
    //! \brief Look and fell of the editor.
    // ************************************************************************
    struct LayoutConfig
    {
        struct Grid
        {
            float step = 64.0f;
            bool  enable = true;
            bool  menu = true;
        };

        struct Color
        {
            // selected
        };

        Grid grid;
    };

public: //private:

    Node* getNode(float const x, float const y);
    Place* getPlace(float const x, float const y);
    Transition* getTransition(float const x, float const y);
    bool changeTypeOfNet(TypeOfNet const type);
    void load();
    void exportTo(Exporter const& exporter);
    void saveAs();
    void close();

    void onHandleInput();
    void onDragged(ImVec2 const& mouse_delta); // Grid
    ImVec2 getMousePosition();
    void handleArcOrigin();
    void handleArcDestination();
    void handleAddNode(ImGuiMouseButton button);
    void handleMoveNode();
    bool IsMouseClicked(ImGuiMouseButton& button, bool& dragging);
    bool IsMouseReleased(ImGuiMouseButton& key);

    /*
      void align();
      PetriNet::CriticalCycleResult findCriticalCycle();
      bool screenshot();
      bool changeTypeOfNet(TypeOfNet const type);
    */
    void clear();

    std::string getError() const
    {
        if (m_messages.getMessages().empty())
            return {};
        return m_messages.getMessage().message;
    }
    std::vector<Messages::TimedMessage> const& getLogs() const { return m_messages.getMessages(); }
    void clearLogs() { m_messages.clear(); }

private: // Inheritnace from Application class

    virtual void onStartUp() override;
    virtual void onDraw() override;

private: // A deplacer dans Renderer:

    void reshape();
    void drawGrill();
    void drawArc(Arc const& arc);
    void drawPlace(Place const& place);
    void drawTransition(Transition const& transition);
    void drawPetriNet();
    void drawToken(float const x, float const y);

public: // FIXME private:

    //! \brief The single Petri net the editor can edit
    Net m_net; // TODO faire plusieurs pour le GRAFCET
    //! \brief Path of the Petri net file: not empty when the net was loaded
    //! from file, else empty when created from scratch.
    std::string m_filename;

public: // FIXME A deplacer dans Grid:
    LayoutConfig m_layout_config;
    ImVec2 scrolling{0.0f, 0.0f};
    ImVec2 canvas_p0;
    ImVec2 canvas_p1;
    ImVec2 canvas_sz;
    ImVec2 origin;
    ImDrawList* draw_list;

    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

private: // gestion de la souris

    //! \brief Mouse cursor position.
    ImVec2 m_mouse;
    //! \brief Selected origin node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_from = nullptr;
    //! \brief Selected destination node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_to = nullptr;
    //! \brief The user has select a node to be displaced.
    std::vector<Node*> m_selected_modes;
    // Ugly stuffs needed when trying to determine which node the user wants to
    // create.
    ImVec2 m_click_position; bool m_arc_from_unknown_node = false;

private:

    // *************************************************************************
    //! \brief State machine for the Petri net simulation.
    // *************************************************************************
    enum States
    {
        STATE_IDLE,     //! Waiting the user request to start the simulation.
        STATE_STARTING, //! Init states before the simulation.
        STATE_ENDING,   //! Restore states after the simulation.
        STATE_ANIMATING //! Simulation on-going: animate tokens.
    };

private:

    //! \brief Memorize number of tokens in places.
    std::vector<size_t> m_backup_tokens;
    //! \brief Some algorithms indicate arcs (i.e. critical cycles, or
    //! if a Petri net is an event graph).
    std::vector<Arc*> m_selected_arcs;
    //! \brief State machine for the simulation.
    std::atomic<States> m_state{STATE_IDLE};
    //! \brief
    Messages m_messages;

public: // FIXME

    //! \brief Set true for starting the simulation the Petri net and to
    //! maintain the simulation running. Set false to halt the simulation.
    std::atomic<bool> m_simulating{false};
};

// FIXME export mandatory because of friendship
//void menu(Editor& editor);

} // namespace tpne

#endif
