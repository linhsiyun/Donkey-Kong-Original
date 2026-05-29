// 純粹定義地圖格子的列舉（Enum）
#ifndef TILE_TYPE_HPP
#define TILE_TYPE_HPP

enum class TileType {
    EMPTY = 0,
    FLOOR = 1,
    LADDER = 2,
    BROKEN_LADDER = 3,
    RIVET = 4, // 100m 插銷
    CONVEYOR1 = 5, // 對應 y1 的長傳送帶
    CONVEYOR2 = 6, // 對應 y2 左邊的傳送帶
    CONVEYOR3 = 7, // 對應 y2 右邊的傳送帶
    MOVING_LADDER_LEFT = 8,   // 左側梯會伸縮的部分
    MOVING_LADDER_RIGHT = 9,  // 右側梯會伸縮的部分
    OVER_LIMIT = 99 // 超出地圖邊界

};

#endif // TILE_TYPE_HPP
