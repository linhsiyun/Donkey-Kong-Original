#ifndef CEMENT_PAN_HPP
#define CEMENT_PAN_HPP

#include "Character.hpp"
#include "TileType.hpp"
#include "Map.hpp"
#include "ConveyorSystem.hpp"
#include "Util/Time.hpp"
#include "config.hpp"

class CementPan : public Character {
public:
    CementPan(TileType targetBelt)
        : Character(RESOURCE_DIR"/Images/cement.png"), m_TargetBelt(targetBelt) {
        SetZIndex(45); // 位於 Mario 下方，背景上方
        SetScale({2.5f, 2.5f}); // 保持與 Mario 一致的視覺比例
    }

    bool ShouldRemove() const { return m_ShouldRemove; }

    /**
     * @brief 更新水泥塊的位移。
     * @param map 用於地圖格子偵測 (Tile Detection)。
     * @param conveyorSystem 用於查詢當前傳送帶速度。
     */
    void Update(const std::shared_ptr<Map>& map, const std::shared_ptr<ConveyorSystem>& conveyorSystem) {
        float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
        glm::vec2 pos = GetPosition();
        glm::vec2 size = GetSize();
        float footY = pos.y - (size.y / 2.0f);

        // 1. 偵測腳下的 Tile
        TileType currentTile = map->GetTileAtPosition(pos.x, footY - 5.0f);
        TileType deeperTile  = map->GetTileAtPosition(pos.x, footY - 20.0f);
        if (currentTile == TileType::EMPTY && deeperTile == TileType::EMPTY) {
            // 2. 只有在地圖內偵測到「空位」(EMPTY) 時才移除，代表掉進洞裡
            m_ShouldRemove = true;
            return;
        } else {
            // 3. 只根據「所屬傳送帶」的方向與速度進行 X 軸位移
            float velocity = conveyorSystem->GetVelocity(m_TargetBelt);
            pos.x += velocity * dt;
        }

        SetPosition(pos);
    }

private:
    TileType m_TargetBelt;
    bool m_ShouldRemove = false;
};

#endif