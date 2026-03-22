#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"

#include "Util/Renderer.hpp"
#include "Character.hpp"
//not used, #include "Util/Text.hpp"
//not used, #include "PhaseResourceManger.hpp"
//not used, #include "AnimatedCharacter.hpp"

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

private:
    State m_CurrentState = State::START;
    //not used, Phase m_Phase = Phase::CHANGE_CHARACTER_IMAGE;

    Util::Renderer m_Root;

    std::shared_ptr<Character> m_Mario;
    std::shared_ptr<Character> m_Mario1;
    std::shared_ptr<Character> m_Mario2;

    int marioState = 0;
};

#endif
