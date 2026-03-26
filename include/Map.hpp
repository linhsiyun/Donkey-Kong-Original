// Map.hpp / .cpp：繼承自你的 Util::GameObject，將圖片與邏輯（LevelData）封裝在一起

#ifndef MAP_HPP
#define MAP_HPP

#include "Util/GameObject.hpp"
#include "LevelData.hpp"
#include <string>

class Map : public Util::GameObject {
public:
    // 假設地圖上一格是 117x120 像素 (你可以依照你的真實圖片比例修改)
    static constexpr float TILE_WIDTH = 117.0f;
    static constexpr float TILE_HEIGHT = 120.0f;

    // 建構子同時接收「圖片路徑」與「TXT路徑」
    Map(const std::string& imagePath, const std::string& txtPath);

    // 讓未來的碰撞系統、角色可以取得這張地圖的邏輯資料
    const LevelData& GetLevelData() const { return m_LevelData; }

    // 【新增】給未來角色的 API：輸入角色的 X, Y 像素座標，回傳他踩到什麼格子
    TileType GetTileAtPosition(float worldX, float worldY) const;

    // 用來在遊戲途中切換地圖的 API
    void LoadNewMap(const std::string& imagePath, const std::string& txtPath);

private:
    LevelData m_LevelData;
};

#endif // MAP_HPP