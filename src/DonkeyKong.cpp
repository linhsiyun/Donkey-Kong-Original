#include "DonkeyKong.hpp"
#include "Util/Time.hpp"      // 用來取得每一幀的時間，進行計時
#include "Util/Animation.hpp" // 需要透過它來手動控制圖片的幀數
#include <cstdlib>            // 用於 rand() 隨機函式的實作
#include <algorithm>          // 用於 std::max

// 定義搥胸循環序列
static const int chestSequence[] = {0, 0, 0, 0, 2, 1, 2, 0};

// =============================================
// 建構子：接收圖片路徑並初始化初始狀態與計時
// 初始化內部的圖片序列 (DK1.png ~ DK5.png)
// =============================================
DonkeyKong::DonkeyKong()
    : AnimatedCharacter({
          RESOURCE_DIR"/Images/DK0.png",
          RESOURCE_DIR"/Images/DK1.png",
          RESOURCE_DIR"/Images/DK2.png",
          RESOURCE_DIR"/Images/DK3.png",
          RESOURCE_DIR"/Images/DK4.png",
          RESOURCE_DIR"/Images/DK5.png",
          RESOURCE_DIR"/Images/Donkey_climb1.png", // Index 6
          RESOURCE_DIR"/Images/Donkey_climb2.png", // Index 7
          RESOURCE_DIR"/Images/Donkey_Princess1.png", // Index 8
          RESOURCE_DIR"/Images/Donkey_Princess2.png",  // Index 9
          RESOURCE_DIR"/Images/push_off.png",
          RESOURCE_DIR"/Images/DKGrin.png"
          RESOURCE_DIR"/Images/Donkey_Princess2.png", // Index 9
          RESOURCE_DIR"/Images/Donkey_fall1.png",     // Index 10
          RESOURCE_DIR"/Images/Donkey_fall2.png"      // Index 11
      }) {
    // 步驟一：停止父類別預設的自動播放，我們將自己根據時間控制圖片切換
    Stop();

    // 步驟二：初始化，第一次進入「環顧狀態 (看左看右看前)」需要隨機搥胸多久
    m_NextLookTime = GetRandomChestBeatingDuration();

    // 初始化第一幀為序列的第一個動作
    m_CurrentChestFrame = 0;
    if (auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable)) {
        anim->SetCurrentFrame(chestSequence[0]);
    }
}

// =============================================
// 輔助方法：隨機產生「這次需要搥胸多久才切換」
// 回傳 1000 到 5000 的隨機毫秒 (1~5 秒)
// =============================================
float DonkeyKong::GetRandomChestBeatingDuration() {
    // 根據等級動態調整搥胸時間 (難度越高，轉頭速度越快)
    // Level 1: 1000ms + (0~4000ms) = 1000~5000ms
    // Level 5: 1000ms + (0~500ms) = 1000~1500ms
    // 使用線性遞減公式，並確保最小值不低於 1000ms 的隨機區間
    int range = (m_Level < 5) ? 4000 : 500;
    return 1000.0f + static_cast<float>(std::rand() % (range + 1));
}


void DonkeyKong::ResetToGame() {
    // 1. 強制將行為與狀態切回常規遊戲的搥胸模式
    m_Behavior = Behavior::STATIONARY_LOOKING;
    m_State = State::CHEST_BEATING;

    // 2. 將所有內部的狀態與動畫計時器歸零重置
    m_ChestTimer = 0.0f;
    m_StateTimer = 0.0f;
    m_LookTimer = 0.0f;
    m_LookIndex = 0;
    m_CurrentChestFrame = 0;

    // 3. 關鍵修復：立刻將當前幀切換到第 0 幀（DK0.png），消除 250ms 的等待閃爍
    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (anim) {
        anim->SetCurrentFrame(0);
    }

    // 4. 關鍵修復：設定大金剛常規遊戲下的專屬正確比例（不要套用 marioScale）
    // 註：此處數值（例如 2.5f）可根據你畫面上大金剛與紅色鷹架的契合度自由微調
    SetScale({2.5f, 2.5f});
}


// =============================================
// 核心更新方法：處理狀態機與動畫計時
// =============================================
void DonkeyKong::Update() {
    // 取得自上一幀以來的經過時間 (Delta Time) ，作為計時依據 (單位使用毫秒)
    float dt = static_cast<float>(Util::Time::GetDeltaTimeMs());

    // 藉由向下轉型，取得底層控制圖片序列的 Animation 物件，以便直接調整顯示的幀(索引)
    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (!anim) return; // 保護機制，如果沒抓到就不執行，避免程式崩潰


    if (m_Behavior == Behavior::OPENING_WAIT) {
        return;
    }

    if (m_Behavior == Behavior::OPENING_CLIMBING) {
        m_ChestTimer += dt;
        int startFrame = 8; // Donkey_Princess1(8) 與 Donkey_Princess2(9) 交替播放

        if (m_CurrentChestFrame < startFrame || m_CurrentChestFrame >= startFrame + 2) {
            m_CurrentChestFrame = startFrame;
            anim->SetCurrentFrame(startFrame);
        }

        if (m_ChestTimer >= 400.0f) { // 每 200ms 切換一次爬行動作
            m_ChestTimer = 0.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == startFrame) ? startFrame + 1 : startFrame;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }
        return; // 提早結束，不執行後續常規遊戲的搥胸或環顧邏輯
    }

    // --- 【新增】移動邏輯: 爬行離開畫面 ---
    if (m_Behavior == Behavior::CLIMBING_AWAY || m_Behavior == Behavior::CLIMBING_WITH_PRINCESS) {
        glm::vec2 pos = GetPosition();
        pos.y += 10.0f * (dt / 1500.0f); // 向上爬行速度 (已放慢)
        SetPosition(pos);

        m_ChestTimer += dt;
        int startFrame = (m_Behavior == Behavior::CLIMBING_AWAY) ? 6 : 8;

        // 初始化動畫幀
        if (m_CurrentChestFrame < startFrame || m_CurrentChestFrame >= startFrame + 2) {
            m_CurrentChestFrame = startFrame;
            anim->SetCurrentFrame(startFrame);
        }

        if (m_ChestTimer >= 400.0f) { // 每 200ms 切換爬行動作
            m_ChestTimer = 0.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == startFrame) ? startFrame + 1 : startFrame;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }
        return;
    }

    // --- 【新增】受傷/暈眩邏輯: 停在原地撥放 fall1/fall2 切換 ---
    if (m_Behavior == Behavior::FALLING_STUNNED) {
        m_ChestTimer += dt;
        // 初始化動畫幀範圍，確保從 fall1 (10) 開始
        if (m_CurrentChestFrame < 10 || m_CurrentChestFrame > 11) {
            m_CurrentChestFrame = 10;
            anim->SetCurrentFrame(10);
        }

        if (m_ChestTimer >= 200.0f) { // 每 200ms 切換受傷動作
            m_ChestTimer = 0.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == 10) ? 11 : 10;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }
        return;
    }

    // --- 移動邏輯: 移動搥胸模式 ---
    if (m_Behavior == Behavior::MOVING_CHEST_BEATING) {
        glm::vec2 pos = GetPosition();
        pos.x += m_MoveSpeed * (dt / 1000.0f);

        // 邊界檢查與反轉方向
        if (pos.x > m_MaxX) {
            pos.x = m_MaxX;
            m_MoveSpeed *= -1.0f;
        } else if (pos.x < m_MinX) {
            pos.x = m_MinX;
            m_MoveSpeed *= -1.0f;
        }
        SetPosition(pos);

        m_ChestTimer += dt; // 用來切換搥胸圖片的計時器

        // 每隔 500ms 依照指定序列 {0, 0, 0, 0, 2, 1, 2, 0} 切換圖片
        float ChestTimeLimit = (m_Level < 5) ? 450.0f : 250;
        if (m_ChestTimer >= ChestTimeLimit) {
            m_ChestTimer = 0.0f; // 重置內部的小計時器

            // 在 0~7 之間循環索引
            m_CurrentChestFrame = (m_CurrentChestFrame + 1) % 8;
            anim->SetCurrentFrame(chestSequence[m_CurrentChestFrame]);
        }
        return;
    }


    // --- 移動邏輯: Behavior::STATIONARY_LOOKING
    // --- 根據目前的狀態進入不同邏輯 ---
    if (m_State == State::CHEST_BEATING) {

        // --- 狀態 1：搥胸 ---
        // 累加速度到兩個計時器
        m_ChestTimer += dt; // 用來切換搥胸圖片的計時器
        m_StateTimer += dt; // 統計已停留在這個狀態多久的計時器

        // 1. 每隔 250ms 在 DK1.png(1) 和 DK2.png(2) 之間切換圖片，形成搥胸動畫
        if (m_ChestTimer >= 250.0f) {
            m_ChestTimer = 0.0f; // 重置內部的小計時器
            // 將畫面的圖片切成另一張（0 就換 1，1 就換 0）
            m_CurrentChestFrame = (m_CurrentChestFrame == 1) ? 2 : 1;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }

        // 2. 檢查總共搥胸時間是否到達指定的隨機上限 (1~2秒)
        if (m_StateTimer >= m_NextLookTime) {
            // 時間到了，轉換到環顧(左右前)狀態
            m_State = State::LOOKING;

            // 重置狀態的主要計時器
            m_StateTimer = 0.0f;

            // 初始化環顧動畫需要的計時器與索引進度
            m_LookTimer = 0.0f;
            m_LookIndex = 0; // 0: 代表第一張環顧圖(DK3.png 向左看)

            // 設定顯示圖為向左看 (因為 1, 2 是搥胸圖，所以 3 是左邊)
            anim->SetCurrentFrame(3);
        }
    }
    else if (m_State == State::LOOKING) {

        // --- 狀態 2：環顧(向左、向前、向右看) ---
        m_LookTimer += dt; // 環顧之間的間隔計時

        // 每個環顧方向也維持定量的時間間隔 (250ms) 才往下切換
        if (m_LookTimer >= 250.0f) {
            // 扣除掉用完的時間，保留非常細微的誤差給下一個週期
            m_LookTimer -= 250.0f;

            // 前進到下一個看的方向
            m_LookIndex++;

            if (m_LookIndex == 1) {
                // 進入第二階段：向前看 (DK4.png - 索引4)
                anim->SetCurrentFrame(4);
            }
            else if (m_LookIndex == 2) {
                // 進入第三階段：向右看 (DK5.png - 索引5)
                anim->SetCurrentFrame(5);

                // --- 核心互動：當向右看時，觸發產出下一個障礙物 (酒桶) ---
                if (m_BarrelSpawnCallback) {
                     m_BarrelSpawnCallback();
                }
            }
            else if (m_LookIndex >= 3) {
                // 已經看完三個方向了，要把狀態切換回原本的搥胸狀態
                m_State = State::CHEST_BEATING;

                // 重置計時器
                m_StateTimer = 0.0f;
                m_ChestTimer = 0.0f;

                // 將圖片索引與畫面上呈現的圖歸零，變回預設的第一張搥胸圖 (DK1.png)
                m_CurrentChestFrame = 0;
                anim->SetCurrentFrame(1);

                // 為了讓動作看起來自然，再次決定下一次要搥胸多少時間才會進入環顧
                m_NextLookTime = GetRandomChestBeatingDuration();
            }
        }
    }
}
