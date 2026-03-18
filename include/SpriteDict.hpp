//這個 HPP 的用途：數字代號與圖片路徑的轉換

#ifndef SPRITEDICT_HPP
#define SPRITEDICT_HPP

#include "pch.hpp"

// 地圖專用的代碼
enum TileType {
    TILE_EMPTY = 0,
    TILE_FLOOR = 1,
    TILE_SCALE = 2,
    TILE_BARREL = 3,
    TILE_SPAWN_DK = 4,
    TILE_SPAWN_PRINCESS = 5
};

class SpriteDict {
public:
    static std::string GetTilePath(int tileType) {
        switch (tileType) {
            case TILE_FLOOR: return "../Resources/Images/Floor.png"; // 請換成你的圖片路徑
            case TILE_SCALE: return "../Resources/Images/Scale.png";
            case TILE_BARREL: return "../Resources/Images/Barrel0.png";
            default: return "";
        }
    }
};

#endif // SPRITEDICT_HPP