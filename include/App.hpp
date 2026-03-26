#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"
#include "Map.hpp"

#include "Util/GameObject.hpp"

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Start();

    void Update();

    void End(); // NOLINT(readability-convert-member-functions-to-static)

private:
    void ValidTask();
    Util::Renderer m_Renderer;
    std::shared_ptr<Map> m_Map;

    //測試用 std::shared_ptr<Util::GameObject> m_TestMarker;

private:
    State m_CurrentState = State::START;
};

#endif
