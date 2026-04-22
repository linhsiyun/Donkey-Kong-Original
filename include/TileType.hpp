// 純粹定義地圖格子的列舉（Enum）
#ifndef TILE_TYPE_HPP
#define TILE_TYPE_HPP

enum class TileType {
    EMPTY = 0,
    FLOOR = 1,
    LADDER = 2,
    RIVET = 3,  // 100m 插銷
    WALL = 5,
};

#endif // TILE_TYPE_HPP
