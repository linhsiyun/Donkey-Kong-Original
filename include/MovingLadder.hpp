#ifndef MOVING_LADDER_HPP
#define MOVING_LADDER_HPP

#include "Character.hpp"
#include "TileType.hpp"
#include "Map.hpp"
#include "Util/Time.hpp"
#include <algorithm>
#include "config.hpp"

enum class LadderState {
  EXTENDED,     // 完全伸長狀態（可以爬）
  EXTENDING,    // 伸長中 (播放動畫，可以爬）
  RETRACTING,   // 縮回中（播放動畫，不能爬）
  RETRACTED     // 完全縮回（不可爬）
};

class MovingLadder : public Character {
public:
    enum class Side { LEFT, RIGHT };

private:
    LadderState m_state;
    Side    m_side;
    float   m_timer;
    float   m_y1; // 伸長時的 Y 座標 (頂部)
    float   m_y2; // 縮回時的 Y 座標 (底部)

    // 設定各個狀態的持續時間 (單位：毫秒，配合 dt 使用)
    const float TIME_EXTENDED = 4000.0f;
    const float TIME_RETRACTING = 800.0f;
    const float TIME_RETRACTED = 4000.0f;
    const float TIME_EXTENDING = 800.0f;

public:
    // 透過傳入兩個 Y 座標，可以在 App.cpp 中自由決定每一條伸縮梯子的移動幅度，而不需要修改類別內部的硬編碼。
    MovingLadder(float x, float y1, float y2, Side side, LadderState initialState)
        : Character(RESOURCE_DIR"/Images/Ladder2.png"), m_state(initialState), m_side(side), m_timer(0.0f), m_y1(y1), m_y2(y2) {
        SetPosition({x, (initialState == LadderState::EXTENDED) ? y1 : y2});
        SetZIndex(10); // 確保在背景層，但可見
    }

    // 1. 線性插值 (Lerp)：公式 pos.y = startY + (endY - startY) * t 確保了梯子會隨著時間均勻移動。std::min(..., 1.0f) 則是為了防止計時器稍微超過總時間時導致座標衝過頭。
    // 2. 狀態同步：在 EXTENDED 與 RETRACTED 靜止狀態下，我們強制設定 pos.y 為 y1 或 y2，消除移動結束後可能存在的浮點數微小誤差。
    void Update(float dtMs) {
        m_timer += dtMs;
        glm::vec2 pos = GetPosition();

        // 狀態機邏輯
        switch (m_state) {
            case LadderState::EXTENDED:
                pos.y = m_y1;
                if (m_timer >= TIME_EXTENDED) SwitchState(LadderState::RETRACTING);
                break;
            case LadderState::RETRACTING: {
                float t = std::min(m_timer / TIME_RETRACTING, 1.0f);
                pos.y = m_y1 + (m_y2 - m_y1) * t; // 從 y1 移動到 y2
                if (m_timer >= TIME_RETRACTING) SwitchState(LadderState::RETRACTED);
                break;
            }
            case LadderState::RETRACTED:
                pos.y = m_y2;
                if (m_timer >= TIME_RETRACTED) SwitchState(LadderState::EXTENDING);
                break;
            case LadderState::EXTENDING: {
                float t = std::min(m_timer / TIME_EXTENDING, 1.0f);
                pos.y = m_y2 + (m_y1 - m_y2) * t; // 從 y2 移動到 y1
                if (m_timer >= TIME_EXTENDING) SwitchState(LadderState::EXTENDED);
                break;
            }
        }
        SetPosition(pos);
    }

    void SwitchState(LadderState newState) {
        m_state = newState;
        m_timer = 0.0f;
    }

    // 關鍵：供 Player 檢查是否可爬
    bool IsClimbable() const {
        // 只有在 完全伸長 或 伸長中 玩家才能抓得住
        return (m_state == LadderState::EXTENDED || m_state == LadderState::EXTENDING);
    }

    LadderState GetState() const { return m_state; }
    Side GetSide() const { return m_side; }
};

#endif // MOVING_LADDER_HPP