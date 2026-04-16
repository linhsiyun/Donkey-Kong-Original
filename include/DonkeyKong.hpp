#ifndef DONKEYKONG_HPP
#define DONKEYKONG_HPP

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Util/Renderer.hpp"
#include "Character.hpp"
#include "AnimatedCharacter.hpp"

/*
  DK有5張圖：
   1. DK1.png: 搥胸1
   2. DK2.png: 搥胸2
   3. DK3.png: 向左看
   4. DK4.png: 向前看
   5. DK5.png: 向右看

  DK的行為模式：
   1. 預設狀態是搥胸，每隔250ms在 DK1.png 和 DK2.png 之間切換，形成搥胸動畫。
   2. 每搥胸 1~2 sec後，切換到 DK3.png、DK4.png、DK5.png，分別表示向左看、向前看、向右看 (一樣間隔250ms)。
   3. 向右看時，觸發一個事件，例如產生一個 Barrel 從右邊滾出來。
   4. 每次看完三個方向後，回到搥胸狀態，重複上述行為。
 */

class DonkeyKong : public AnimatedCharacter {
public:
    // 步驟一：定義建構子，初始化內部的動畫圖層
    DonkeyKong();

    // 步驟二：更新 DK 狀態 (根據時間改變動畫與觸發事件)
    void Update();

    // 步驟三：設定當 DK 轉頭到最後（向右看）的時候，要觸發的回呼函數（例如丟出酒桶）
    void SetBarrelSpawnCallback(const std::function<void()>& callback) {
        m_BarrelSpawnCallback = callback;
    }

private:
    // 定義 DK 可能的兩種行為狀態
    enum class State {
        CHEST_BEATING, // 搥胸動畫狀態
        LOOKING        // 環顧(左、前、右)動畫狀態
    };

    State m_State = State::CHEST_BEATING; // 一開始設定為搥胸狀態

    // ===== 計時器區域 (毫秒為單位) =====
    float m_ChestTimer = 0.0f;     // 追蹤搥胸每次圖片切換的間隔時間
    float m_StateTimer = 0.0f;     // 追蹤目前這輪搥胸已經過了多久
    float m_LookTimer = 0.0f;      // 追蹤看左右前三張圖之間的切換間隔時間

    // 儲存隨機取得的「這一次搥胸所需要持續的時間」
    float m_NextLookTime = 0.0f;

    // ===== 動畫索引追蹤 =====
    int m_CurrentChestFrame = 0;   // 目前是在搥胸的第一張 (0) 還是第二張 (1)
    int m_LookIndex = 0;           // 目前看方向的進度 (0:看左, 1:看前, 2:看右)

    // ===== 事件回呼 =====
    std::function<void()> m_BarrelSpawnCallback; // 紀錄外部設定的回呼函數(用來產桶子)

    // 產生一個介於 1000 到 2000 的隨機數值(1~2秒)，回傳毫秒
    float GetRandomChestBeatingDuration();
};

#endif // DONKEYKONG_HPP