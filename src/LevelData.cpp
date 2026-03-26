#include "LevelData.hpp"
#include "Util/LoadTextFile.hpp"
#include "Util/Logger.hpp"
#include <sstream>

void LevelData::LoadFromFile(const std::string& filepath) {
    m_Grid.clear();

    // 呼叫現有的 Util 函式讀取整個檔案
    std::string content = Util::LoadTextFile(filepath);
    if (content.empty()) {
        LOG_ERROR("Level data file is empty or missing: {}", filepath);
        return;
    }

    std::stringstream ss(content);
    std::string line;

    // 逐行解析
    while (std::getline(ss, line)) {
        std::vector<TileType> row;
        for (char c : line) {
            // 只處理數字，忽略換行符號(\r, \n)或其他雜訊
            if (c >= '0' && c <= '9') {
                int typeValue = c - '0'; // 將字元轉換為整數
                row.push_back(static_cast<TileType>(typeValue));
            }
        }
        if (!row.empty()) {
            m_Grid.push_back(row);
        }
    }
    LOG_INFO("Loaded map data: {}x{} grid", GetWidth(), GetHeight());
}

TileType LevelData::GetTile(int x, int y) const {
    // 邊界檢查，避免未來的碰撞系統存取越界
    if (y < 0 || y >= m_Grid.size() || x < 0 || x >= m_Grid[y].size()) {
        return TileType::EMPTY;
    }
    return m_Grid[y][x];
}

int LevelData::GetWidth() const {
    return m_Grid.empty() ? 0 : m_Grid[0].size();
}

int LevelData::GetHeight() const {
    return m_Grid.size();
}