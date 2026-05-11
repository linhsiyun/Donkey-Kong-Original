#ifndef CONVEYOR_SYSTEM_HPP
#define CONVEYOR_SYSTEM_HPP

#include "TileType.hpp"
#include "Util/Logger.hpp"
#include <map>

class ConveyorSystem {
public:
    struct BeltState {
        float speed;      // 基礎速度 (px/s)
        float timer;      // 當前計時
        float interval;   // 反轉間隔 (ms)
        int direction;    // 1 或 -1
    };

    ConveyorSystem() {
        // 初始化三條傳送帶的狀態
        m_Belts[TileType::CONVEYOR1] = { 60.0f, 0.0f, 10000.0f, 1 };  // 長傳送帶
        m_Belts[TileType::CONVEYOR2] = { 60.0f, 5000.0f, 10000.0f, 1 }; // 左傳送帶
        m_Belts[TileType::CONVEYOR3] = { 60.0f, 0.0f, 10000.0f, -1 };  // 右傳送帶
    }

    void Update(float dtMs) {
        for (auto& pair : m_Belts) {
            auto& belt = pair.second;
            belt.timer += dtMs;
            if (belt.timer >= belt.interval) {
                belt.timer = 0.0f;
                belt.direction *= -1; // 反轉方向
                LOG_DEBUG("Conveyor {:d} reversed direction to: {:d}", (int)pair.first, belt.direction);
            }
        }
    }

    float GetVelocity(TileType type) {
        if (m_Belts.find(type) != m_Belts.end())
        {
            return m_Belts[type].speed * m_Belts[type].direction;
        }
        return 0.0f;
    }

    int GetDirection(TileType type) {
        if (m_Belts.find(type) != m_Belts.end()) {
            return m_Belts[type].direction;
        }
        return 0;
    }

private:
    std::map<TileType, BeltState> m_Belts;
};

#endif