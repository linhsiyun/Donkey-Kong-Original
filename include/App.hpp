#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"

#include "Util/Renderer.hpp"
#include "Character.hpp"
//not used, #include "Util/Text.hpp"
//not used, #include "PhaseResourceManger.hpp"
#include "AnimatedCharacter.hpp"

enum class MarioState {
    IDLE,       // 靜止
    WALKING,    // 走路
    CLIMBING,   // 爬梯子
    CLIMB_IDLE, // 停在梯子上
    JUMPING,    // 跳躍中（包含上升與下降）,  一經發動即不可控
    FALLING,    // 墜落（例如從平台邊緣直接掉下去，不是因為跳躍）
    HAMMERING,  // 拿著槌子（這是一個特殊狀態，因為此時不能跳、不能爬）
    HAMMER_IDLE,// 拿槌原地
    DEAD,       // 死亡動畫
    WIN         // 抵達 Pauline 身邊的過場
};

enum class MarioDIR {
    NONE,  // 靜止/無輸入
    LEFT,  // 面向左
    RIGHT, // 面向右
    UPDOWN,   // 爬梯向上or向下 (不影響Mario圖片)
};

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

    std::shared_ptr<Character> m_Mario;
    std::shared_ptr<AnimatedCharacter> m_MarioWalk;
    std::shared_ptr<AnimatedCharacter> m_MarioClimb;
    std::shared_ptr<Character> m_MarioJump;
    std::shared_ptr<Character> m_MarioFall;     
    std::shared_ptr<Character> m_MarioHammer;   
    std::shared_ptr<Character> m_MarioDead;     
    std::shared_ptr<Character> m_MarioWin;      

    bool m_IsJumping = false;
    float m_JumpTimer = 0.0f;
    glm::vec2 m_JumpStartPosition;  
    int m_JumpDirection = 0;        // 0: None, 1: Right, -1: Left

    MarioState marioState = MarioState::IDLE;
    MarioDIR  marioDir = MarioDIR::NONE;
    glm::vec2 marioPosition;    // 根據目前的 marioState 決定顯示哪個物件 

    void ShowMario(void);
};

#endif
