#ifndef FIAMMA_HPP
#define FIAMMA_HPP

#include "AnimatedCharacter.hpp"
#include "Map.hpp" // 【新增】：引入地圖標頭檔
#include <cmath>
#include <memory>

class Fiamma : public AnimatedCharacter {
public:
    // 建構子：初始化火球動畫
    Fiamma();

    // 每幀更新：處理移動邏輯與邊界偵測
    void Update();

    // 簡單的 AABB 碰撞偵測，縮減火球碰撞範圍使其更公平
    [[nodiscard]] bool IfCollides(const glm::vec2& otherPos, const glm::vec2& otherSize) const {
        // 將火球的碰撞有效範圍縮小為圖片的 60%
        const auto self_half_size = (GetSize() * 0.6f) / 2.0f;
        const auto other_half_size = otherSize / 2.0f;
        const auto& self_pos = GetPosition();

        return std::abs(self_pos.x - otherPos.x) < (self_half_size.x + other_half_size.x) &&
               std::abs(self_pos.y - otherPos.y) < (self_half_size.y + other_half_size.y);
    }

    void SetMap(std::shared_ptr<Map> map) { m_Map = map; }

    enum class State { WALKING, CLIMBING, FALLING };
    enum class Direction { LEFT, RIGHT };
    enum class VerticalDirection { UP, DOWN };
    void SetState(State state) { m_State = state; }

    // 根據關卡設定火球樣式 (Stage 4 使用 fiamma5, 6)
    void SetStageStyle(bool isRivet);

private:
    State m_State = State::FALLING;
    Direction m_Direction = Direction::RIGHT;
    VerticalDirection m_ClimbDir = VerticalDirection::UP;

    float m_MoveSpeed = 80.0f;  // 水平移動速度
    float m_ClimbSpeed = 60.0f; // 爬梯速度

    float m_RandomTurnTimer = 2.0f; // 隨機轉向的計時器
    int m_LastCheckedLadderCol = -1; // 記錄剛剛判定過的梯子

    std::shared_ptr<Map> m_Map;
};

#endif // FIAMMA_HPP