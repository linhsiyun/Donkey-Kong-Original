#include "DonkeyKong.hpp"
#include "Util/Time.hpp"      // 用來取得每一幀的時間，進行計時
#include "Util/Animation.hpp" // 需要透過它來手動控制圖片的幀數
#include <cstdlib>            // 用於 rand() 隨機函式的實作

// =============================================
// 建構子：接收圖片路徑並初始化初始狀態與計時
// =============================================
DonkeyKong::DonkeyKong(const std::vector<std::string>& AnimationPaths)
    : AnimatedCharacter(AnimationPaths) {

    // 步驟一：停止父類別預設的自動播放，我們將自己根據時間控制圖片切換
    Stop();

    // 步驟二：初始化，第一次進入「環顧狀態 (看左看右看前)」需要隨機搥胸多久
    m_NextLookTime = GetRandomChestBeatingDuration();
}

// =============================================
// 輔助方法：隨機產生「這次需要搥胸多久才切換」
// 回傳 1000 到 2000 的隨機毫秒 (1~2 秒)
// =============================================
float DonkeyKong::GetRandomChestBeatingDuration() {
    return 1000.0f + static_cast<float>(rand() % 1001);
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

    // --- 根據目前的狀態進入不同邏輯 ---
    if (m_State == State::CHEST_BEATING) {

        // --- 狀態 1：搥胸 ---
        // 累加速度到兩個計時器
        m_ChestTimer += dt; // 用來切換搥胸圖片的計時器
        m_StateTimer += dt; // 統計已停留在這個狀態多久的計時器

        // 1. 每隔 250ms 在 DK1.png(0) 和 DK2.png(1) 之間切換圖片，形成搥胸動畫
        if (m_ChestTimer >= 250.0f) {
            m_ChestTimer = 0.0f; // 重置內部的小計時器
            // 將畫面的圖片切成另一張（0 就換 1，1 就換 0）
            m_CurrentChestFrame = (m_CurrentChestFrame == 0) ? 1 : 0;
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

            // 設定顯示圖為向左看 (因為 0, 1 是搥胸圖，所以 2 是左邊)
            anim->SetCurrentFrame(2);
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
                // 進入第二階段：向前看 (DK4.png - 索引3)
                anim->SetCurrentFrame(3);
            }
            else if (m_LookIndex == 2) {
                // 進入第三階段：向右看 (DK5.png - 索引4)
                anim->SetCurrentFrame(4);

                // --- 核心互動：當向右看時，觸發產出下一個障礙物 (酒桶) ---
#if 1 // TODO
                if (m_BarrelSpawnCallback) {
                     m_BarrelSpawnCallback();
                }
#endif
            }
            else if (m_LookIndex >= 3) {
                // 已經看完三個方向了，要把狀態切換回原本的搥胸狀態
                m_State = State::CHEST_BEATING;

                // 重置計時器
                m_StateTimer = 0.0f;
                m_ChestTimer = 0.0f;

                // 將圖片索引與畫面上呈現的圖歸零，變回預設的第一張搥胸圖 (DK1.png)
                m_CurrentChestFrame = 0;
                anim->SetCurrentFrame(0);

                // 為了讓動作看起來自然，再次決定下一次要搥胸多少時間才會進入環顧
                m_NextLookTime = GetRandomChestBeatingDuration();
            }
        }
    }
}