#ifndef BARREL_HPP
#define BARREL_HPP

#include <memory>
#include <vector>
#include <string>
#include "Util/Renderer.hpp"
#include "AnimatedCharacter.hpp"

/*
    酒桶的圖片：
        Barrel.png ~ Barrel4.png: 酒桶滾動的四張動畫圖
        Barrel5.png, Barrel6.png: 酒桶落下的兩張動畫圖（例如從平台邊緣掉下去的情況）

    酒桶的行為模式：
     1. 酒桶一開始會從 DK 的位置出現，並且水平向左或向右滾動（由 DK 的朝向決定）。
     2. 如果酒桶滾到梯子旁邊，會有機率選擇沿著梯子落下(切換到下落的動畫)，直到下面的平台上繼續滾動。
     3. 如果酒桶滾到平台邊緣，會切換到落下動畫，並且開始垂直向下移動，直到離開畫面或碰到 Mario。
        - 酒桶在滾動時會切換 Barrel.png ~ Barrel4.png 形成滾動動畫。
        - 向右滾動時，圖片會是 Barrel.png 到 Barrel4.png 輪播；向左滾動時，圖片會是 Barrel4.png 到 Barrel.png 輪播。
        - 酒桶在落下時會切換到 Barrel5.png、Barrel6.png 形成落下的動畫。
     4. 酒桶從上層平台落下到下一層，先切換到落下動畫，然後持續向下移動，直到落到下一個平台上，再切換回滾動動畫並改變水平移動方向。
     5. 酒桶會持續滾動或者落下，直到離開畫面(酒桶消失)或碰到 Mario (Mario 死亡，遊戲結束)。
 */
class Barrel : public AnimatedCharacter {
public:
    // 定義酒桶可能的移動方向
    enum class Direction {
        LEFT,
        RIGHT
    };

    // 定義酒桶目前的動作狀態
    enum class State {
        ROLLING,       // 在平台上水平滾動
        FALLING_LADDER, // 沿著梯子垂直落下
        FALLING_EDGE    // 從平台邊緣垂直落下
    };

    // 步驟一：定義建構子，接收多張圖的路徑 (Barrel.png 到 Barrel6.png)
    explicit Barrel(const std::vector<std::string>& AnimationPaths);

    // 步驟二：更新酒桶狀態 (處理位置移動與動畫幀的切換)
    void Update();

    // 設定與取得酒桶的移動方向
    void SetDirection(Direction dir) { m_Direction = dir; }
    [[nodiscard]] Direction GetDirection() const { return m_Direction; }

    // 設定與取得酒桶的目前狀態
    void SetState(State state) { m_State = state; }
    [[nodiscard]] State GetState() const { return m_State; }

    // 設定酒桶的移動速度
    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }
    void SetFallSpeed(float speed) { m_FallSpeed = speed; }

private:
    State m_State = State::ROLLING;          // 預設為滾動狀態
    Direction m_Direction = Direction::RIGHT; // 預設向右滾動

    // 速度設定
    float m_MoveSpeed = 150.0f; // 水平滾動速度 (pixels/sec)
    float m_FallSpeed = 200.0f; // 垂直掉落速度 (pixels/sec)

    // ===== 動畫與計時器區域 =====
    float m_AnimationTimer = 0.0f; // 控制圖片切換的計時器
    int m_CurrentFrame = 0;        // 紀錄目前的圖片索引 (0~3: 滾動, 4~5: 落下)
};
#endif // BARREL_HPP