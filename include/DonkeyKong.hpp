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

class DonkeyKong : public AnimatedCharacter {
public:
    // ===== 步驟一：定義大金剛的各種行為模式 =====
    // 這裡的列舉必須包含你 .cpp 裡寫到的所有 Behavior
    enum class Behavior {
        STATIONARY_LOOKING,     // 原地搥胸 + 環顧 (Level 1)
        MOVING_CHEST_BEATING,   // 左右移動 + 只搥胸 (Level 2+)
        CLIMBING_AWAY,          // 過關時爬行離開畫面
        CLIMBING_WITH_PRINCESS, // 抱著公主爬行離開畫面
        FALLING_STUNNED,        // 掉落到一半受傷/暈眩狀態
        OPENING_CLIMBING,       // 開場動畫：往上爬
        OPENING_WAIT            // 開場動畫：爬完等待
    };

    DonkeyKong();

    void Update();

    // 讓 DK 認識地圖
    void SetMap(std::shared_ptr<Map> map) { m_Map = map; }

    // 設定當 DK 向右看時，要觸發的回呼函數（丟出木桶）
    void SetBarrelSpawnCallback(const std::function<void()>& callback) {
        m_BarrelSpawnCallback = callback;
    }

    // ===== 狀態控制 API =====
    void SetBehavior(Behavior behavior) { m_Behavior = behavior; }
    Behavior GetBehavior() const { return m_Behavior; }
    void SetLevel(int level) { m_Level = level; }

    // ===== 移動邊界控制 API =====
    void SetMoveBounds(float minX, float maxX) { m_MinX = minX; m_MaxX = maxX; }
    float GetMinX() const { return m_MinX; }
    float GetMaxX() const { return m_MaxX; }

    // 回復到常規遊戲狀態的 API (對應你 .cpp 裡的實作)
    void ResetToGame();
    std::shared_ptr<Core::Drawable> GetDrawable() const { return m_Drawable; }
    void SetLocked(bool locked) { m_IsLocked = locked; }

private:
    // 定義常規遊戲中 (STATIONARY_LOOKING) 的兩種狀態
    enum class State {
        CHEST_BEATING, // 搥胸動畫狀態
        LOOKING        // 環顧(左、前、右)動畫狀態
    };

    State m_State = State::CHEST_BEATING; // 預設為搥胸狀態
    std::shared_ptr<Map> m_Map = nullptr;
    int m_Level = 1;

    // ===== 行為與移動參數 =====
    Behavior m_Behavior = Behavior::STATIONARY_LOOKING;
    float m_MinX = -1000.0f; // 給予預設極大值避免一開始就觸發反轉
    float m_MaxX = 1000.0f;
    float m_MoveSpeed = 50.0f;

    // ===== 計時器區域 (毫秒為單位) =====
    float m_ChestTimer = 0.0f;     // 追蹤搥胸/爬行每次圖片切換的間隔時間
    float m_StateTimer = 0.0f;     // 追蹤目前這輪搥胸已經過了多久
    float m_LookTimer = 0.0f;      // 追蹤看左右前三張圖之間的切換間隔時間
    float m_NextLookTime = 0.0f;   // 儲存隨機取得的「這一次搥胸所需要持續的時間」

    // ===== 動畫索引追蹤 =====
    int m_CurrentChestFrame = 0;   // 目前使用的圖片索引
    int m_LookIndex = 0;           // 目前看方向的進度 (0:看左, 1:看前, 2:看右)

    // ===== 事件回呼 =====
    std::function<void()> m_BarrelSpawnCallback;

    // 輔助函式：產生隨機搥胸時間
    float GetRandomChestBeatingDuration();
    bool m_IsLocked = false;
};

#endif // DONKEYKONG_HPP