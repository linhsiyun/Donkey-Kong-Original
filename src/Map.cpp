#include "Map.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp" // 【新增】引入 Logger 以解決 LOG_INFO 錯誤
#include "config.hpp"
#include <cmath>
#include <algorithm>// 【新增】引入 cmath 以使用 std::floor

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
    // 1. 取得目前的縮放比例 (假設是等比例縮放)
    float scale = m_Transform.scale.x;

    // 2. 計算「縮放後」的地圖實際總寬高 (像素)
    float mapPixelWidth = m_LevelData.GetWidth() * TILE_WIDTH * scale;
    float mapPixelHeight = m_LevelData.GetHeight() * TILE_HEIGHT * scale;

    float mapCenterX = GetTransform().translation.x;
    float mapCenterY = GetTransform().translation.y;

    // 3. 算出地圖左上角的真實座標
    float mapTopLeftX = mapCenterX - (mapPixelWidth / 2.0f);
    float mapTopLeftY = mapCenterY + (mapPixelHeight / 2.0f);

    // 4. 計算滑鼠相對於地圖左上角的距離
    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY;

    // 5. 關鍵：除以「縮放後」的單格大小
    int gridX = static_cast<int>(std::floor(localX / (TILE_WIDTH * scale)));
    int gridY = static_cast<int>(std::floor(localY / (TILE_HEIGHT * scale)));

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
    float scaleX = static_cast<float>(WINDOW_WIDTH) / imageSize.x;
    float scaleY = static_cast<float>(WINDOW_HEIGHT) / imageSize.y;

    // 使用 std::min 會讓整張圖「完整塞進」視窗 (維持比例，但可能會留黑邊)
    // 如果你希望圖片「填滿」整個視窗 (維持比例，但不留黑邊，超出視窗的部分會被裁切掉)，請把 min 改成 max
    float finalScale = std::min(scaleX, scaleY);

    m_Transform.scale = {finalScale, finalScale};
    LOG_INFO("地圖自動縮放比例為: {}", finalScale);
}