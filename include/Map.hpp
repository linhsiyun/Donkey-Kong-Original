// Map.hpp / .cpp：繼承自你的 Util::GameObject，將圖片與邏輯（LevelData）封裝在一起

#ifndef MAP_HPP
#define MAP_HPP

#include "Util/GameObject.hpp"
#include "LevelData.hpp"
#include <string>

class Map : public Util::GameObject {
public:
    // 建構子同時接收「圖片路徑」與「TXT路徑」
    Map(const std::string& imagePath, const std::string& txtPath);

    // --- 核心邏輯資料 ---
    const LevelData& GetLevelData() const { return m_LevelData; }

    // === 為了相容 App.cpp 而補回的 API ===
    float GetMapWidth() const { return mapPixelWidth; }
    float GetMapHeight() const { return mapPixelHeight; }
    float GetTileWidth() const { return actualTileWidth; }
    float GetTileHeight() const { return actualTileHeight; }
    const glm::vec2& GetScale() const { return m_Transform.scale; }

    // 相容舊的名稱：直接呼叫我們剛寫好的 GetGridToWorldPosition
    glm::vec2 GetTileWorldPosition(int gridX, int gridY) const;

    /**
     * @brief 取得指定世界座標下的格子類型
     */
    TileType GetTileAtPosition(float worldX, float worldY) const;

    /**
     * @brief 取得指定世界座標對應的網格索引 (Column, Row)
     */
    std::pair<int, int> GetTileIndexAtPosition(float worldX, float worldY) const;

    // --- 地圖操作 ---
    void SetTileAtPosition(float worldX, float worldY, TileType type);
    void LoadNewMap(const std::string& imagePath, const std::string& txtPath);

    // --- 生命週期管理 ---
    void UpdateDimensions(); // 當視窗縮放或地圖改變時呼叫

private:
    void AutoScale();

    LevelData m_LevelData;

    float mapPixelWidth = 0.0f;
    float mapPixelHeight = 0.0f;
    float actualTileWidth = 0.0f;
    float actualTileHeight = 0.0f;

    float mapTopLeftX;
    float mapTopLeftY;
};

#endif // MAP_HPP
