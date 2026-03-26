#include "Map.hpp"
#include "Util/Image.hpp"

Map::Map(const std::string& imagePath, const std::string& txtPath) {
    // 1. 設定地圖的視覺 (掛載整張大圖)
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 設定 Z-Index，確保地圖渲染在最底層 (可根據你的系統需求調整)
    SetZIndex(-10.0f);

    // 2. 載入地圖的邏輯陣列
    m_LevelData.LoadFromFile(txtPath);
}

// 【新增】實作座標轉換
TileType Map::GetTileAtPosition(float worldX, float worldY) const {
    // 假設你的地圖左上角是 (0, 0)，將真實像素除以 Tile Size 就能得到陣列的 Index
    // 例如 X = 100，100 / 32 = 3.125，取整數就是 3，代表在陣列的第 3 行
    int gridX = static_cast<int>(std::floor(worldX / TILE_SIZE));
    int gridY = static_cast<int>(std::floor(worldY / TILE_SIZE));

    return m_LevelData.GetTile(gridX, gridY);
}