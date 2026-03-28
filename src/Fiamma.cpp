#include "Fiamma.hpp"

Fiamma::Fiamma() : AnimatedCharacter({
    RESOURCE_DIR"/Images/fiamma1.png",
    RESOURCE_DIR"/Images/fiamma2.png"
}) {
    // 設定初始屬性
    SetZIndex(45);           // 稍微位於 Mario (50) 後方
    SetScale({2.0f, 2.0f});  // 縮放比例

    // 設定動畫參數
    SetLooping(true);
    SetInterval(200);        // 每 200ms 切換一次圖片
    Play();
}

void Fiamma::Update() {
    // 取得當前位置並增加位移
    glm::vec2 pos = GetPosition();
    pos += m_Velocity;

    // 簡單的跑來跑去邏輯：碰到設定的邊界就往回跑
    if (pos.x > m_BoundX) {
        pos.x = m_BoundX;
        m_Velocity.x *= -1.0f;
        SetScale({-std::abs(GetScale().x), GetScale().y}); // 轉向左
    } else if (pos.x < -m_BoundX) {
        pos.x = -m_BoundX;
        m_Velocity.x *= -1.0f;
        SetScale({std::abs(GetScale().x), GetScale().y});  // 轉向右
    }

    // 更新物理位置
    SetPosition(pos);
}