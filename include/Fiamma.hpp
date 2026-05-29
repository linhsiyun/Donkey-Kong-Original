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

    // 簡單的 AABB 碰撞偵測，傳入對方的座標與尺寸
    [[nodiscard]] bool IfCollides(const glm::vec2& otherPos, const glm::vec2& otherSize) const {
        const auto self_half_size = GetSize() / 2.0f;
        const auto other_half_size = otherSize / 2.0f;
        const auto& self_pos = GetPosition();

        // 檢查 X 軸與 Y 軸是否重疊
        return std::abs(self_pos.x - otherPos.x) < (self_half_size.x + other_half_size.x) &&
               std::abs(self_pos.y - otherPos.y) < (self_half_size.y + other_half_size.y);
    }

    void SetMap(std::shared_ptr<Map> map) { m_Map = map; }

    enum class State { WALKING, CLIMBING, FALLING };
    enum class Direction { LEFT, RIGHT };
    void SetState(State state) { m_State = state; }

private:
    State m_State = State::FALLING;
    Direction m_Direction = Direction::RIGHT;

    float m_MoveSpeed = 80.0f;  // 水平移動速度
    float m_ClimbSpeed = 60.0f; // 爬梯速度

    float m_RandomTurnTimer = 0.0f; // 隨機轉向的計時器
    int m_LastCheckedLadderCol = -1; // 記錄剛剛判定過的梯子

    std::shared_ptr<Map> m_Map;
};

#endif // FIAMMA_HPP