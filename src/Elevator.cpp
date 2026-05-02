#include "Elevator.hpp"
#include "config.hpp" // 取得 RESOURCE_DIR
#include "Util/Time.hpp"

Elevator::Elevator(Direction dir, float minY, float maxY, float speed)
    : Character(RESOURCE_DIR"/Images/Floor0.png"), m_Direction(dir), m_MinY(minY), m_MaxY(maxY), m_Speed(speed) {
    SetZIndex(30); // 放在背景前面，Mario 下方
}

void Elevator::Update() {
    glm::vec2 pos = GetPosition();
    float dt_ms = static_cast<float>(Util::Time::GetDeltaTimeMs()); // 取得毫秒數

    if (m_Direction == Direction::UP) {
        pos.y += m_Speed * (dt_ms / 1000.0f) * 60.0f; // 將毫秒轉換為秒，再套用速度邏輯
        if (pos.y > m_MaxY) pos.y = m_MinY; // 超出頂部，從底部重生
    } else {
        pos.y -= m_Speed * (dt_ms / 1000.0f) * 60.0f; // 將毫秒轉換為秒，再套用速度邏輯
        if (pos.y < m_MinY) pos.y = m_MaxY; // 超出底部，從頂部重生
    }

    SetPosition(pos);
}