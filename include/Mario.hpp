#ifndef MARIO_HPP
#define MARIO_HPP

#include <memory>
#include "Util/Renderer.hpp"
#include "Character.hpp"
#include "AnimatedCharacter.hpp"

// Mario 的狀態列舉，定義了 Mario 在遊戲中可能的各種行為狀態
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
};

enum class CLIMB_DIR {
    UP,
    DOWN
};

class Mario {
public:
    // 建構子：負責初始化所有的靜態圖片與動畫
    Mario();

    // 將 Mario 的所有內部組件註冊到 App 的 Renderer 中
    void AddToRenderer(Util::Renderer& renderer);

    // 每幀更新：給 App的Update 呼叫
    // 根據 CurrentState 來決定誰該顯示 (Visible)、是否播放/停止動畫 (Play/Stop)、以及其他邏輯處理
    void Update();

    // 統一設定座標，並同步給內部的所有圖層
    void SetPosition(const glm::vec2& position);

    // 取得當前位置
    glm::vec2 GetPosition() const { return m_Position; }

    // 取得 Mario 的碰撞尺寸 (以 Idle 狀態為基準)
    glm::vec2 GetSize() const;

    // 取得當前狀態
    MarioState GetState() const { return m_CurrentState; }

    // 設定當前狀態
    void SetState(MarioState newState); // { m_CurrentState = newState; }

    // 改變狀態（例如從 IDLE 變成 JUMPING）
    //void ChangeState(MarioState newState);

    void IDLE();
    void Walk(MarioDIR dir);
    void Climb(CLIMB_DIR dir);
    void ClimbIdle();
    void JumpStart(); // 這個函式專門負責觸發跳躍狀態，並初始化相關變數
    void Jump();
    void Fall();
    void Hammer();
    void HammerIdle();
    void Dead();
    void Win();

    const float marioScale = 2.5F;  // Mario 的縮放比例
    const float movingStep = 2.0F;  // 走路與跳躍速度
    const float climbingStep = 1.0F; // 攀爬速度
    bool IsJumping() const { return m_IsJumping; }

private:
    // 將所有動作圖層封裝在這裡：
    std::shared_ptr<Character>         m_Idle;
    std::shared_ptr<AnimatedCharacter> m_Walk;
    std::shared_ptr<AnimatedCharacter> m_Climb;
    std::shared_ptr<Character>         m_Jump;
    std::shared_ptr<Character>         m_Fall;
    std::shared_ptr<AnimatedCharacter> m_Hammer;
    std::shared_ptr<Character>         m_HammerIdle;
    std::shared_ptr<AnimatedCharacter> m_Dead;
    std::shared_ptr<Character>         m_DiedFinal;
    std::shared_ptr<Character>         m_Win;

    // 狀態與邏輯變數也封裝進來
    MarioState m_CurrentState = MarioState::IDLE;
    glm::vec2 m_Position;

    MarioDIR m_Direction = MarioDIR::RIGHT;
    MarioDIR m_BackupDirection = MarioDIR::RIGHT; // for jump direction backup (when jump is vertical, use the last walking direction)

    bool m_IsJumping = false;
    float m_JumpTimer = 0.0f;
    float m_DeadTimer = 0.0f;
    float m_HammerTimer = 0.0f; // 槌子剩餘時間
    glm::vec2 m_JumpStartPosition;
};

#endif // MARIO_HPP