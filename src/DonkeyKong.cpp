#include "DonkeyKong.hpp"
#include "Util/Time.hpp"      // 用來取得每一幀的時間，進行計時
#include "Util/Animation.hpp" // 需要透過它來手動控制圖片的幀數
#include "config.hpp"         // 為了使用 RESOURCE_DIR
#include <cstdlib>            // 用於 rand()
#include <algorithm>          // 用於 std::max

// 定義常規搥胸模式的循環序列
static const int chestSequence[] = {0, 0, 0, 0, 2, 1, 2, 0};

// =============================================
// 建構子：接收圖片路徑並初始化初始狀態與計時
// =============================================
DonkeyKong::DonkeyKong()
    : AnimatedCharacter({
          RESOURCE_DIR"/Images/DK0.png",
          RESOURCE_DIR"/Images/DK1.png",
          RESOURCE_DIR"/Images/DK2.png",
          RESOURCE_DIR"/Images/DK3.png",
          RESOURCE_DIR"/Images/DK4.png",
          RESOURCE_DIR"/Images/DK5.png",
          RESOURCE_DIR"/Images/Donkey_climb1.png",    // Index 6
          RESOURCE_DIR"/Images/Donkey_climb2.png",    // Index 7
          RESOURCE_DIR"/Images/Donkey_Princess1.png", // Index 8
          RESOURCE_DIR"/Images/Donkey_Princess2.png", // Index 9
          RESOURCE_DIR"/Images/Donkey_fall1.png",     // Index 10
          RESOURCE_DIR"/Images/Donkey_fall2.png",     // Index 11
          RESOURCE_DIR"/Images/push_off.png",         // Index 12
          RESOURCE_DIR"/Images/DKGrin.png"            // Index 13
      }) {
    // 停止父類別預設的自動播放，我們將自己根據時間與狀態控制圖片切換
    Stop();

    // 初始化第一次進入「環顧狀態」需要隨機搥胸多久
    m_NextLookTime = GetRandomChestBeatingDuration();

    // 初始化第一幀為序列的第一個動作
    m_CurrentChestFrame = 0;
    if (auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable)) {
        anim->SetCurrentFrame(chestSequence[0]);
    }
}

// =============================================
// 輔助方法：隨機產生「這次需要搥胸多久才切換」
// =============================================
float DonkeyKong::GetRandomChestBeatingDuration() {
    // 根據等級動態調整搥胸時間 (難度越高，轉頭速度越快)
    // Level 1: 1000ms + (0~4000ms) = 1000~5000ms
    // Level 5: 1000ms + (0~500ms) = 1000~1500ms
    int range = (m_Level < 5) ? 4000 : 500;
    return 1000.0f + static_cast<float>(std::rand() % (range + 1));
}

// =============================================
// API：開場結束後，將大金剛狀態重置回遊戲模式
// =============================================
void DonkeyKong::ResetToGame() {
    m_Behavior = Behavior::STATIONARY_LOOKING;
    m_State = State::CHEST_BEATING;

    m_ChestTimer = 0.0f;
    m_StateTimer = 0.0f;
    m_LookTimer = 0.0f;
    m_LookIndex = 0;
    m_CurrentChestFrame = 0;

    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (anim) {
        anim->SetCurrentFrame(0);
    }

    // 設定大金剛常規遊戲下的專屬正確比例
    // SetScale({2.5f, 2.5f});
}

// =============================================
// 核心更新方法：處理狀態機與所有行動邏輯
// =============================================
void DonkeyKong::Update() {
    float dt = static_cast<float>(Util::Time::GetDeltaTimeMs());
    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (!anim) return; // 保護機制

    if (m_IsLocked) {
        return;
    }
    // ---------------------------------------------------------
    // 1. 開場動畫等待中 (由 OpeningScene 操控)
    // ---------------------------------------------------------
    if (m_Behavior == Behavior::OPENING_WAIT) {
        return;
    }

    // ---------------------------------------------------------
    // 2. 開場動畫爬行中 (只處理圖片切換，座標由 OpeningScene 計算)
    // ---------------------------------------------------------
    if (m_Behavior == Behavior::OPENING_CLIMBING) {
        m_ChestTimer += dt;
        int startFrame = 8; // Donkey_Princess1(8) 與 Donkey_Princess2(9) 交替播放

        if (m_CurrentChestFrame < startFrame || m_CurrentChestFrame >= startFrame + 2) {
            m_CurrentChestFrame = startFrame;
            anim->SetCurrentFrame(startFrame);
        }

        if (m_ChestTimer >= 200.0f) { // 每 200ms 切換一次爬行動作
            m_ChestTimer -= 200.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == startFrame) ? startFrame + 1 : startFrame;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }

        return; // 提早結束，避免執行下方常規邏輯
    }

    // ---------------------------------------------------------
    // 3. 過關爬行離開畫面 (自行計算 Y 軸位移)
    // ---------------------------------------------------------
    if (m_Behavior == Behavior::CLIMBING_AWAY || m_Behavior == Behavior::CLIMBING_WITH_PRINCESS) {
        glm::vec2 pos = GetPosition();
        pos.y += 10.0f * (dt / 1500.0f); // 向上爬行速度
        SetPosition(pos);

        m_ChestTimer += dt;
        int startFrame = (m_Behavior == Behavior::CLIMBING_AWAY) ? 6 : 8;

        if (m_CurrentChestFrame < startFrame || m_CurrentChestFrame >= startFrame + 2) {
            m_CurrentChestFrame = startFrame;
            anim->SetCurrentFrame(startFrame);
        }

        if (m_ChestTimer >= 400.0f) { // 每 400ms 切換爬行動作
            m_ChestTimer -= 400.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == startFrame) ? startFrame + 1 : startFrame;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }
        return;
    }

    // ---------------------------------------------------------
    // 4. 受傷/暈眩邏輯 (停在原地撥放 fall1/fall2 切換)
    // ---------------------------------------------------------
    if (m_Behavior == Behavior::FALLING_STUNNED) {
        m_ChestTimer += dt;
        if (m_CurrentChestFrame < 10 || m_CurrentChestFrame > 11) {
            m_CurrentChestFrame = 10;
            anim->SetCurrentFrame(10);
        }

        if (m_ChestTimer >= 200.0f) {
            m_ChestTimer -= 200.0f;
            m_CurrentChestFrame = (m_CurrentChestFrame == 10) ? 11 : 10;
            anim->SetCurrentFrame(m_CurrentChestFrame);
        }
        return;
    }

    // ---------------------------------------------------------
    // 5. 高難度：移動並搥胸模式 (Level 2+)
    // ---------------------------------------------------------
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

        m_ChestTimer += dt;
        float chestTimeLimit = (m_Level < 5) ? 450.0f : 250.0f;
        if (m_ChestTimer >= chestTimeLimit) {
            m_ChestTimer -= chestTimeLimit;
            m_CurrentChestFrame = (m_CurrentChestFrame + 1) % 8;
            anim->SetCurrentFrame(chestSequence[m_CurrentChestFrame]);
        }
        return;
    }

    // ---------------------------------------------------------
    // 6. 經典模式：原地搥胸 + 環顧丟木桶 (Level 1)
    // ---------------------------------------------------------
    if (m_Behavior == Behavior::STATIONARY_LOOKING) {
        if (m_State == State::CHEST_BEATING) {
            m_ChestTimer += dt;
            m_StateTimer += dt;

            // 1. 每隔 250ms 在 DK1.png(1) 和 DK2.png(2) 之間切換
            if (m_ChestTimer >= 250.0f) {
                m_ChestTimer -= 250.0f;
                m_CurrentChestFrame = (m_CurrentChestFrame == 1) ? 2 : 1;
                anim->SetCurrentFrame(m_CurrentChestFrame);
            }

            // 2. 檢查總共搥胸時間是否到達指定的隨機上限
            if (m_StateTimer >= m_NextLookTime) {
                m_State = State::LOOKING;
                m_StateTimer = 0.0f;
                m_LookTimer = 0.0f;
                m_LookIndex = 0; // 0: 看左
                anim->SetCurrentFrame(3);
            }
        }
        else if (m_State == State::LOOKING) {
            m_LookTimer += dt;

            // 每個環顧方向維持 250ms
            if (m_LookTimer >= 250.0f) {
                m_LookTimer -= 250.0f;
                m_LookIndex++;

                if (m_LookIndex == 1) {
                    anim->SetCurrentFrame(4); // 向前看
                }
                else if (m_LookIndex == 2) {
                    anim->SetCurrentFrame(5); // 向右看
                    // 觸發產出木桶
                    if (m_BarrelSpawnCallback) {
                         m_BarrelSpawnCallback();
                    }
                }
                else if (m_LookIndex >= 3) {
                    // 看完三個方向，切換回搥胸狀態
                    m_State = State::CHEST_BEATING;
                    m_StateTimer = 0.0f;
                    m_ChestTimer = 0.0f;

                    m_CurrentChestFrame = 1;
                    anim->SetCurrentFrame(m_CurrentChestFrame);

                    // 決定下一次要搥胸多久
                    m_NextLookTime = GetRandomChestBeatingDuration();
                }
            }
        }
    }
}