// LevelData.hpp / .cpp：專門負責讀取 TXT 檔案並解析成二維陣列
#ifndef LEVEL_DATA_HPP
#define LEVEL_DATA_HPP

#include <string>
#include <vector>
#include "TileType.hpp"

class LevelData {
public:
    // 讀取 TXT 檔案並解析成二維陣列
    void LoadFromFile(const std::string& filepath);

    // 給未來的碰撞系統查詢用的 API
    TileType GetTile(int x, int y) const;

    // 【新增】修改特定座標的格子類型
    void SetTile(int x, int y, TileType type);

    int GetWidth() const;
    int GetHeight() const;

private:
    std::vector<std::vector<TileType>> m_Grid;
};

#endif // LEVEL_DATA_HPP