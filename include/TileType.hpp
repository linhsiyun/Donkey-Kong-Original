// 純粹定義地圖格子的列舉（Enum）
#ifndef TILE_TYPE_HPP
#define TILE_TYPE_HPP

enum class TileType {
    EMPTY = 0,
    FLOOR = 1,
    LADDER = 2,
    BROKEN_LADDER = 3,
    RIVET = 4,

};

#endif // TILE_TYPE_HPP