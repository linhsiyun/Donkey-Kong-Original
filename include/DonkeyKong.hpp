#ifndef DONKEYKONG_HPP
#define DONKEYKONG_HPP

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Util/Renderer.hpp"
#include "Character.hpp"
#include "AnimatedCharacter.hpp"
#include "Map.hpp"

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

    // 讓 DK 認識地圖，這對未來自動判定站在哪一層平台很有幫助
    void SetMap(std::shared_ptr<Map> map) { m_Map = map; }
    // 步驟三：設定當 DK 轉頭到最後（向右看）的時候，要觸發的回呼函數（例如丟出酒桶）
    void SetBarrelSpawnCallback(const std::function<void()>& callback) {
        m_BarrelSpawnCallback = callback;
    }

    // 設定 Donkey Kong 的行為模式
    enum class Behavior {
        STATIONARY_LOOKING,   // 原地搥胸 + 環顧 (Level 1)
        MOVING_CHEST_BEATING, // 左右移動 + 只搥胸 (Level 2+)
        CLIMBING_AWAY,        // 【新增】過關時爬行離開畫面
        CLIMBING_WITH_PRINCESS, // 【新增】抱著公主爬行離開畫面
        FALLING_STUNNED        // 【新增】掉落到一半受傷/暈眩狀態
    };

    void SetBehavior(Behavior behavior) { m_Behavior = behavior; }
    Behavior GetBehavior() const { return m_Behavior; }

    // 設定當前關卡，用以調整 AI 難度
    void SetLevel(int level) { m_Level = level; }

    // 設定左右移動的 X 軸邊界
    void SetMoveBounds(float minX, float maxX) { m_MinX = minX; m_MaxX = maxX; }

    // 輔助函式：檢查 DK 是否超出邊界
    // 這能解決你說的「位置改了參數沒變」的問題，確保 Update 邏輯會參考這裡
    float GetMinX() const { return m_MinX; }
    float GetMaxX() const { return m_MaxX; }

private:
    // 定義 DK 可能的兩種行為狀態
    enum class State {
        CHEST_BEATING, // 搥胸動畫狀態
        LOOKING        // 環顧(左、前、右)動畫狀態
    };

    State m_State = State::CHEST_BEATING; // 一開始設定為搥胸狀態
    std::shared_ptr<Map> m_Map = nullptr;
    int m_Level = 1;                      // 當前關卡

    // ===== 行為與移動參數 =====
    Behavior m_Behavior = Behavior::STATIONARY_LOOKING;
    float m_MinX = -1000.0f; // 給予預設極大值避免一開始就觸發反轉
    float m_MaxX = 1000.0f;
    float m_MoveSpeed = 50.0f;
    float m_MoveDirection = 1.0f; // 新增：控制移動方向


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
