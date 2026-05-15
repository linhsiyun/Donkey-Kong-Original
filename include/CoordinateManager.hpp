#ifndef COORDINATE_MANAGER_HPP
#define COORDINATE_MANAGER_HPP

#include <glm/glm.hpp>
#include "config.hpp"

class CoordinateManager {
public:
    // 定義地圖的邏輯大小（絕對座標範圍）
    static constexpr float MAP_LOGIC_SIZE = 720.0f;

    /**
     * @brief 將絕對座標 (0~720) 轉換為引擎中心座標 (-W/2 ~ W/2)
     * @param logicPos 絕對座標，(0,0) 為地圖左上角
     */
    static glm::vec2 LogicToEngine(const glm::vec2& logicPos) {
        // 1. 先將 (0,0) 移到地圖中心：(logicX - 360, logicY - 360)
        // 2. Y 軸反轉（邏輯座標下為正，引擎座標上為正）：-(logicY - 360)
        float engineX = logicPos.x - (MAP_LOGIC_SIZE / 2.0f);
        float engineY = (MAP_LOGIC_SIZE / 2.0f) - logicPos.y;
        
        return { engineX, engineY };
    }

    /**
     * @brief 將引擎座標轉換回絕對邏輯座標 (用於處理 Input)
     */
    static glm::vec2 EngineToLogic(const glm::vec2& enginePos) {
        float logicX = enginePos.x + (MAP_LOGIC_SIZE / 2.0f);
        float logicY = (MAP_LOGIC_SIZE / 2.0f) - enginePos.y;
        
        return { logicX, logicY };
    }
};

#endif