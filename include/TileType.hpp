// 純粹定義地圖格子的列舉（Enum）
#ifndef TILE_TYPE_HPP
#define TILE_TYPE_HPP

enum class TileType {
    EMPTY = 0,
    FLOOR = 1,
    LADDER = 2,
    RIVET = 3,  // 100m 插銷
    CONVEYOR1 = 5, // 對應 y1 的長傳送帶
    CONVEYOR2 = 6, // 對應 y2 左邊的傳送帶
    CONVEYOR3 = 7, // 對應 y2 右邊的傳送帶
    OVER_LIMIT = 99 // 超出地圖邊界
};

#endif // TILE_TYPE_HPP
