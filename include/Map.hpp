//這個 HPP 的用途：將 TXT 檔轉換成實體的 GameObject

#ifndef MAP_HPP
#define MAP_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

class Map {
public:
    Map() = default;
    ~Map() = default;

    // 加上 [[nodiscard]]，強制呼叫者必須接收這包地圖物件
    [[nodiscard]] std::vector<std::shared_ptr<Util::GameObject>> LoadMap(const std::string& filepath);

private:
    std::vector<std::shared_ptr<Util::GameObject>> m_Tiles;

    const int TILE_SIZE = 30;
    const int MAP_COLS = 37;
    const int MAP_ROWS = 24;
};

#endif // MAP_HPP