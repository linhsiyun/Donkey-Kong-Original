#include "Map.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp" // 【新增】引入 Logger 以解決 LOG_INFO 錯誤
#include <cmath>           // 【新增】引入 cmath 以使用 std::floor

Map::Map(const std::string& imagePath, const std::string& txtPath) {
    // 設定地圖的視覺 (掛載整張大圖)
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 設定 Z-Index，確保地圖渲染在最底層 (可根據你的系統需求調整)
    SetZIndex(-10.0f);

    // 載入地圖的邏輯陣列
    m_LevelData.LoadFromFile(txtPath);
}

// 新增實作座標轉換
TileType Map::GetTileAtPosition(float worldX, float worldY) const {
    // 取得地圖整體的寬高 (像素)
    float mapPixelWidth = m_LevelData.GetWidth() * TILE_WIDTH;
    float mapPixelHeight = m_LevelData.GetHeight() * TILE_HEIGHT;

    // 取得地圖本身的中心點座標 (防呆：如果以後你移動了地圖，這段依然會準確)
    float mapCenterX = GetTransform().translation.x;
    float mapCenterY = GetTransform().translation.y;

    // 算出地圖「左上角」在畫面上的真實座標
    // 中心點往左退半個寬度，往上進半個高度 (因為 Y 向上為正)
    float mapTopLeftX = mapCenterX - (mapPixelWidth / 2.0f);
    float mapTopLeftY = mapCenterY + (mapPixelHeight / 2.0f);

    // 計算角色相對於地圖左上角的「相對像素距離」
    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY; // 注意這裡：因為陣列是越往下 Y 越大，所以用 TopLeftY 去減 worldY

    // 假設你的地圖左上角是 (0, 0)，將真實像素除以 Tile Size 就能得到陣列的 Index
    // 例如 X = 100，100 / 32 = 3.125，取整數就是 3，代表在陣列的第 3 行
    int gridX = static_cast<int>(std::floor(localX / TILE_WIDTH));
    int gridY = static_cast<int>(std::floor(localY / TILE_HEIGHT));

    return m_LevelData.GetTile(gridX, gridY);
}

void Map::LoadNewMap(const std::string& imagePath, const std::string& txtPath) {
    // 替換掉舊的圖片
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 重新載入新的 TXT 陣列
    m_LevelData.LoadFromFile(txtPath);

    LOG_INFO("地圖已成功切換至: {}", imagePath);
}