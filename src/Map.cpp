#include "Map.hpp"
#include "CoordinateManager.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
#include "config.hpp"
#include "Setting.hpp"
#include <cmath>
#include <algorithm>

Map::Map(const std::string& imagePath, const std::string& txtPath) {
    // 設定地圖的視覺 (掛載整張大圖)
    SetDrawable(std::make_shared<Util::Image>(imagePath));

    // 設定 Z-Index，確保地圖渲染在最底層
    SetZIndex(-10.0f);

    // 載入地圖的邏輯陣列
    m_LevelData.LoadFromFile(txtPath);

    // 初始化時先自動縮放並計算所有維度
    AutoScale();
    UpdateDimensions();
}

void Map::LoadNewMap(const std::string& imagePath, const std::string& txtPath) {
    SetDrawable(std::make_shared<Util::Image>(imagePath));
    m_LevelData.LoadFromFile(txtPath);

    AutoScale();
    UpdateDimensions();
}

void Map::AutoScale() {
    if (m_Drawable == nullptr) return;

    glm::vec2 imageSize = m_Drawable->GetSize();

    // 為了符合 CoordinateManager 定義的 720 邏輯大小
    // 我們將地圖縮放到「高度」正好等於 720 像素（視窗高度）
    // 這樣地圖在 1080x720 的視窗中，上下會剛好貼合，左右則會維持比例置中
    float targetHeight = CoordinateManager::MAP_LOGIC_SIZE;
    float scale = targetHeight / imageSize.y;

    // 採用統一縮放倍率，避免圖片拉伸變形
    m_Transform.scale = {scale, scale};

    // 確保地圖位於視窗中心 (0,0)，這樣 CoordinateManager 的轉換才會精準
    m_Transform.translation = {0.0f, 0.0f};
}

void Map::UpdateDimensions() {
    if (m_Drawable == nullptr) return;

    glm::vec2 imageSize = m_Drawable->GetSize();
    glm::vec2 currentScale = m_Transform.scale;

    // 計算地圖在畫面上真實的像素長寬
    m_MapPixelWidth = imageSize.x * currentScale.x;
    m_MapPixelHeight = imageSize.y * currentScale.y;

    // 雖然改用了 CoordinateManager，我們依然保留這些數值供其他 API 使用
    if (m_LevelData.GetWidth() > 0 && m_LevelData.GetHeight() > 0) {
        m_ActualTileWidth = m_MapPixelWidth / m_LevelData.GetWidth();
        m_ActualTileHeight = m_MapPixelHeight / m_LevelData.GetHeight();
    }

    // 計算地圖左上角的世界座標 (用於邊界檢查 API)
    m_WorldTopLeft.x = m_Transform.translation.x - (m_MapPixelWidth / 2.0f);
    m_WorldTopLeft.y = m_Transform.translation.y + (m_MapPixelHeight / 2.0f);
}

std::pair<int, int> Map::GetTileIndexAtPosition(const glm::vec2& worldPos) const {
    // --- 核心改進：不再自己算 localX，直接使用現有的座標管理系統 ---

    // 1. 將世界座標轉換為 0~720 的邏輯座標
    glm::vec2 logicPos = CoordinateManager::EngineToLogic(worldPos);

    // 2. 根據網格總數，計算每一格在「邏輯空間」佔多少單位
    float logicTileWidth = CoordinateManager::MAP_LOGIC_SIZE / m_LevelData.GetWidth();
    float logicTileHeight = CoordinateManager::MAP_LOGIC_SIZE / m_LevelData.GetHeight();

    // 3. 直接換算網格索引
    int col = static_cast<int>(std::floor(logicPos.x / logicTileWidth));
    int row = static_cast<int>(std::floor(logicPos.y / logicTileHeight));

    return {col, row};
}

TileType Map::GetTileAtPosition(const glm::vec2& worldPos) const {
    auto [col, row] = GetTileIndexAtPosition(worldPos);

    // 邊界防護：如果超出地圖網格範圍，視為空氣(EMPTY)
    if (col < 0 || col >= m_LevelData.GetWidth() ||
        row < 0 || row >= m_LevelData.GetHeight()) {
        return TileType::EMPTY;
    }

    return m_LevelData.GetTile(col, row);
}

glm::vec2 Map::GetGridToWorldPosition(int col, int row) const {
    // 回傳該網格的「中心點」世界座標
    float worldX = m_WorldTopLeft.x + (col + 0.5f) * m_ActualTileWidth;
    float worldY = m_WorldTopLeft.y - (row + 0.5f) * m_ActualTileHeight;
    return {worldX, worldY};
}

void Map::SetTileAtPosition(const glm::vec2& worldPos, TileType type) {
    auto [col, row] = GetTileIndexAtPosition(worldPos);

    if (col >= 0 && col < m_LevelData.GetWidth() &&
        row >= 0 && row < m_LevelData.GetHeight()) {
        // 利用 LevelData 提供的 SetTile 更新陣列
        // 註：這需要確定 LevelData.hpp 裡有 void SetTile(int x, int y, TileType type);
        m_LevelData.SetTile(col, row, type);
    }
}

// ===== 邊界檢查 API 實作 =====

float Map::GetLeftBoundary() const {
    return m_WorldTopLeft.x;
}

float Map::GetRightBoundary() const {
    return m_WorldTopLeft.x + m_MapPixelWidth;
}

float Map::GetTopBoundary() const {
    return m_WorldTopLeft.y;
}

float Map::GetBottomBoundary() const {
    // 引擎的 Y 軸越往下數值越小，所以是左上角 Y 扣掉整張地圖的高
    return m_WorldTopLeft.y - m_MapPixelHeight;
}