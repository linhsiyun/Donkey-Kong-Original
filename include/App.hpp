#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"

//#include "Character.hpp"
//not used, #include "Util/Text.hpp"
//not used, #include "PhaseResourceManger.hpp"
//#include "AnimatedCharacter.hpp"

#include "Mario.hpp"
#include "Fiamma.hpp"



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

    //void ShowMario(void);
    std::shared_ptr<Mario> m_Mario;
    std::shared_ptr<Fiamma> m_Fireball;
};

#endif
