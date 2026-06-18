#ifndef CEMENT_SPAWNER_HPP
#define CEMENT_SPAWNER_HPP

#include "Character.hpp"
#include "TileType.hpp"
#include <glm/vec2.hpp>

class CementSpawner : public Character {
public:
    enum class Side { LEFT, RIGHT };

    CementSpawner(TileType targetBelt, Side side)
        : Character(RESOURCE_DIR"/Images/cement.png"),
          m_TargetBelt(targetBelt), m_Side(side) {
        SetVisible(false); // Spawner 本身通常是不可見的
    }

    TileType GetTargetBelt() const { return m_TargetBelt; }
    Side GetSide() const { return m_Side; }

    /**
     * @brief 檢查並更新生成計時。
     * @return 如果傳送帶方向正確且冷卻時間到，回傳 true。
     */
    bool ShouldSpawn(float dtMs, int beltDir) {
        m_SpawnTimer += dtMs;

        // 判斷傳送帶方向是否朝向場內
        // 左側 Spawner (Side::LEFT): 如果方向為 1 (Right)，代表往場內推
        // 右側 Spawner (Side::RIGHT): 如果方向為 -1 (Left)，代表往場內推
        bool isPushingInward = (m_Side == Side::LEFT && beltDir == 1) ||
                               (m_Side == Side::RIGHT && beltDir == -1);

        if (isPushingInward && m_SpawnTimer >= m_SpawnInterval) {
            m_SpawnTimer = 0.0f;
            return true;
        }

        if (!isPushingInward) m_SpawnTimer = 0.0f; // 若方向不符則重置計時
        return false;
    }

private:
    // 在「傳送帶關卡」（Stage 2）中，畫面上有三種不同特性的傳送帶（例如長度、高度或初始方向不同），
    // 分別對應 TileType::CONVEYOR1、CONVEYOR2 與 CONVEYOR3。
    TileType m_TargetBelt;  // 記錄該生成器（Spawner）是屬於哪一條傳送帶。
    Side m_Side;
    float m_SpawnTimer = 0.0f; //0.0f;
    float m_SpawnInterval = 3500.0f; // 每 3.5 秒生成一個
};

#endif