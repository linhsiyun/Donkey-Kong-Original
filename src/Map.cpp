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
    UpdateDimensions();
}

// 新增實作座標轉換
TileType Map::GetTileAtPosition(float worldX, float worldY) const {
    // 預防讀取空的地圖資料而導致除以零
    if (m_LevelData.GetWidth() == 0 || m_LevelData.GetHeight() == 0) {
        return TileType::EMPTY;
    }

    // 計算角色的座標相對於地圖左上角的相對距離
    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY;

    // 4. 使用最新算出的動態網格大小來換算角色目前踩在第幾格（Index）
    int gridX = static_cast<int>(std::floor(localX / actualTileWidth));
    int gridY = static_cast<int>(std::floor(localY / actualTileHeight));

    return m_LevelData.GetTile(gridX, gridY);
}

void Map::SetTileAtPosition(float worldX, float worldY, TileType type) {
    if (m_LevelData.GetWidth() == 0 || m_LevelData.GetHeight() == 0) return;

    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY;
    int gridX = static_cast<int>(std::floor(localX / actualTileWidth));
    int gridY = static_cast<int>(std::floor(localY / actualTileHeight));
    
    m_LevelData.SetTile(gridX, gridY, type);
}

glm::vec2 Map::GetTileWorldPosition(int gridX, int gridY) const {
    // 計算該格子中心點的世界座標
    float localX = (static_cast<float>(gridX) + 0.5f) * actualTileWidth;
    float localY = (static_cast<float>(gridY) + 0.5f) * actualTileHeight;

    return {mapTopLeftX + localX, mapTopLeftY - localY};
}

std::pair<int, int> Map::GetTileIndexAtPosition(float worldX, float worldY) const {
    if (m_LevelData.GetWidth() == 0 || m_LevelData.GetHeight() == 0) return {-1, -1};

    float localX = worldX - mapTopLeftX;
    float localY = mapTopLeftY - worldY;
    
    int gridX = static_cast<int>(std::floor(localX / actualTileWidth));
    int gridY = static_cast<int>(std::floor(localY / actualTileHeight));
    
    return {gridX, gridY};
}

void Map::LoadNewMap(const std::string& imagePath, const std::string& txtPath) {
    // 替換掉舊的圖片
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 重新載入新的 TXT 陣列
    m_LevelData.LoadFromFile(txtPath);
    AutoScale();
    UpdateDimensions();

    LOG_INFO("map switches to: {}, {}", imagePath, txtPath);
}

void Map::UpdateDimensions() {
    // 1. 取得地圖目前的縮放比例與原始圖片大小
    float scaleX = std::abs(GetTransform().scale.x);
    float scaleY = std::abs(GetTransform().scale.y);
    glm::vec2 imageSize = m_Drawable->GetSize();

    // 2. 將原始圖片大小乘上縮放倍率，得到「整張地圖在畫面上真實的像素長寬」
    mapPixelWidth = imageSize.x * scaleX;
    mapPixelHeight = imageSize.y * scaleY;
    LOG_INFO("  mapPixel: ({},{})", mapPixelWidth, mapPixelHeight);

    // 3. 【動態連動】計算單一格子的實際寬高
    //    公式：地圖實際顯示寬度 / TXT 網格的行數 = 單一格子的動態寬度
    actualTileWidth = mapPixelWidth / m_LevelData.GetWidth();
    actualTileHeight = mapPixelHeight / m_LevelData.GetHeight();
    LOG_INFO("actualTile: ({},{})", actualTileWidth, actualTileHeight);

    float mapCenterX = GetTransform().translation.x;
    float mapCenterY = GetTransform().translation.y;

    // 定位出地圖圖片的左上角座標
    mapTopLeftX = mapCenterX - (mapPixelWidth / 2.0f);
    mapTopLeftY = mapCenterY + (mapPixelHeight / 2.0f);
    LOG_INFO(" mapTopLeft: ({},{})", mapTopLeftX, mapTopLeftY);
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
    LOG_INFO("map AutoScale: {}", finalScale);
}
