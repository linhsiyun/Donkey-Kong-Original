#include "Map.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp" // 【新增】引入 Logger 以解決 LOG_INFO 錯誤
#include "config.hpp"
#include <cmath>
#include <algorithm>// 【新增】引入 cmath 以使用 std::floor
#include "Setting.hpp"

Map::Map(const std::string& imagePath, const std::string& txtPath) {
    // 設定地圖的視覺 (掛載整張大圖)
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 設定 Z-Index，確保地圖渲染在最底層 (可根據你的系統需求調整)
    SetZIndex(-10.0f);

    // 載入地圖的邏輯陣列
    m_LevelData.LoadFromFile(txtPath);
    AutoScale();

}

// 新增實作座標轉換
TileType Map::GetTileAtPosition(float worldX, float worldY) const {
    // 1. 取得地圖目前的縮放比例
    float scaleX = std::abs(GetTransform().scale.x);
    float scaleY = std::abs(GetTransform().scale.y);

    // 2. 將原始的單格大小乘上縮放倍率，得到「畫面上真實的格子像素大小」
    float actualTileWidth = TILE_WIDTH * scaleX;
    float actualTileHeight = TILE_HEIGHT * scaleY;

    // 3. 取得地圖放大後的整體寬高
    float mapPixelWidth = m_LevelData.GetWidth() * actualTileWidth;
    float mapPixelHeight = m_LevelData.GetHeight() * actualTileHeight;

    float mapCenterX = GetTransform().translation.x;
    float mapCenterY = GetTransform().translation.y;

    float mapTopLeftX = mapCenterX - (mapPixelWidth / 2.0f);
    float mapTopLeftY = mapCenterY + (mapPixelHeight / 2.0f);

    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY;

    // 4. 【關鍵】使用縮放後的實際大小 (actualTileWidth) 來進行除法計算
    int gridX = static_cast<int>(std::floor(localX / actualTileWidth));
    int gridY = static_cast<int>(std::floor(localY / actualTileHeight));

    return m_LevelData.GetTile(gridX, gridY);
}
void Map::LoadNewMap(const std::string& imagePath, const std::string& txtPath) {
    // 替換掉舊的圖片
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 重新載入新的 TXT 陣列
    m_LevelData.LoadFromFile(txtPath);
    AutoScale();

    LOG_INFO("地圖已成功切換至: {}", imagePath);
}

// 自動縮放邏輯
void Map::AutoScale() {
    // 取得圖片原本的原始像素大小
    glm::vec2 imageSize = m_Drawable->GetSize();

    // 分別計算 X 軸與 Y 軸需要放大的倍率
#if VSCODE
    float scaleX = static_cast<float>(PTSD_Config::WINDOW_WIDTH) / imageSize.x;
    float scaleY = static_cast<float>(PTSD_Config::WINDOW_HEIGHT) / imageSize.y;
#else
    float scaleX = static_cast<float>(WINDOW_WIDTH) / imageSize.x;
    float scaleY = static_cast<float>(WINDOW_HEIGHT) / imageSize.y;
#endif    

    // 使用 std::min 會讓整張圖「完整塞進」視窗 (維持比例，但可能會留黑邊)
    // 如果你希望圖片「填滿」整個視窗 (維持比例，但不留黑邊，超出視窗的部分會被裁切掉)，請把 min 改成 max
    float finalScale = std::min(scaleX, scaleY);

    m_Transform.scale = {finalScale, finalScale};
    LOG_INFO("地圖自動縮放比例為: {}", finalScale);
}