#ifndef FIAMMA_HPP
#define FIAMMA_HPP

#include "AnimatedCharacter.hpp"
#include "Util/Animation.hpp"
#include <cmath>

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

private:
    // 取得目前動畫幀的尺寸
    [[nodiscard]] glm::vec2 GetSize() const {
        return std::dynamic_pointer_cast<Util::Animation>(m_Drawable)->GetSize() * glm::abs(GetScale());
    }

    glm::vec2 m_Velocity = {1.5f, 0.0f}; // 移動速度，可自行調整
    const float m_BoundX = 300.0f;       // 左右移動的邊界範圍
};

#endif // FIAMMA_HPP