// Map.hpp
#ifndef MAP_HPP
#define MAP_HPP

#include "Util/GameObject.hpp"
#include "LevelData.hpp"
#include "CoordinateManager.hpp" // 引入邏輯座標管理
#include <string>
#include <utility>

class Map : public Util::GameObject {
public:
    // 建構子同時接收「圖片路徑」與「TXT路徑」
    Map(const std::string& imagePath, const std::string& txtPath);

    // --- 核心邏輯資料 ---
    const LevelData& GetLevelData() const { return m_LevelData; }

    // === 為了相容 App.cpp 而補回的 API ===
    float GetMapWidth() const { return m_MapPixelWidth; }
    float GetMapHeight() const { return m_MapPixelHeight; }
    float GetTileWidth() const { return m_ActualTileWidth; }
    float GetTileHeight() const { return m_ActualTileHeight; }
    glm::vec2 GetScale() const { return m_Transform.scale; }
    // 補回：相容舊的 GetTileIndexAtPosition 參數
    std::pair<int, int> GetTileIndexAtPosition(float worldX, float worldY) const {
        return GetTileIndexAtPosition(glm::vec2(worldX, worldY));
    }

    // 補回：相容舊的 SetTileAtPosition 參數
    void SetTileAtPosition(float worldX, float worldY, TileType type) {
        SetTileAtPosition(glm::vec2(worldX, worldY), type);
    }

    // 相容舊的名稱：直接呼叫我們剛寫好的 GetGridToWorldPosition
    glm::vec2 GetTileWorldPosition(int gridX, int gridY) const {
        return GetGridToWorldPosition(gridX, gridY);
    }

    // 相容舊的參數格式：將兩個 float 包裝成 glm::vec2 再呼叫
    TileType GetTileAtPosition(float worldX, float worldY) const {
        return GetTileAtPosition(glm::vec2(worldX, worldY));
    }

    // --- 座標轉換 API (取代原本破碎的 localX 計算) ---

    /**
     * @brief 取得指定世界座標下的格子類型
     */
    TileType GetTileAtPosition(const glm::vec2& worldPos) const;

    /**
     * @brief 取得指定世界座標對應的網格索引 (Column, Row)
     */
    std::pair<int, int> GetTileIndexAtPosition(const glm::vec2& worldPos) const;

    /**
     * @brief 將網格索引轉換為世界座標 (回傳格子的中心點)
     */
    glm::vec2 GetGridToWorldPosition(int col, int row) const;

    // --- 邊界檢查 API (供 Mario 等物件使用，取代硬編碼的螢幕寬高) ---
    float GetLeftBoundary() const;
    float GetRightBoundary() const;
    float GetTopBoundary() const;
    float GetBottomBoundary() const;

    // --- 地圖操作 ---
    void SetTileAtPosition(const glm::vec2& worldPos, TileType type);
    void LoadNewMap(const std::string& imagePath, const std::string& txtPath);

    // --- 生命週期管理 ---
    void UpdateDimensions(); // 當視窗縮放或地圖改變時呼叫

private:
    void AutoScale();

    LevelData m_LevelData;

    // 快取地圖的實體屬性，避免重複計算
    float m_MapPixelWidth = 0.0f;
    float m_MapPixelHeight = 0.0f;
    float m_ActualTileWidth = 0.0f;
    float m_ActualTileHeight = 0.0f;

    // 地圖左上角的世界座標基準點
    glm::vec2 m_WorldTopLeft = {0.0f, 0.0f};
};

#endif // MAP_HPP
