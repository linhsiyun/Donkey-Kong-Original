#ifndef ELEVATOR_HPP
#define ELEVATOR_HPP
#include "Character.hpp"
#include <glm/vec2.hpp>

/*
  Elevator Stage： 畫面中央有兩條垂直電梯，一條向上移動，一條向下移動。
 */
class Elevator : public Character {
public:
    enum class Direction { UP, DOWN };

    Elevator(Direction dir, float minY, float maxY, float speed);
    void Update();

    Direction GetDirection() const { return m_Direction; }
    float GetSpeed() const { return m_Speed; }

private:
    Direction m_Direction;
    float m_Speed;        // 電梯移動速度
    float m_MinY;         // 最低點界線
    float m_MaxY;         // 最高點界線
};

#endif // ELEVATOR_HPP