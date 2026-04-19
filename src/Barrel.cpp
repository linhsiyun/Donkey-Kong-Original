#include "Barrel.hpp"
#include "Util/Time.hpp"      // 用來取得每一幀的時間，進行計時與移動計算
#include "Util/Animation.hpp" // 需要透過它來手動控制圖片的幀數

// =============================================
// 建構子：接收圖片路徑並初始化初始狀態
// 初始化酒桶的圖片序列 (Barrel1.png ~ Barrel6.png)
// =============================================
Barrel::Barrel(State state, Direction dir)
    : AnimatedCharacter({
          RESOURCE_DIR"/Images/Barrel1.png",
          RESOURCE_DIR"/Images/Barrel2.png",
          RESOURCE_DIR"/Images/Barrel3.png",
          RESOURCE_DIR"/Images/Barrel4.png",
          RESOURCE_DIR"/Images/Barrel5.png",
          RESOURCE_DIR"/Images/Barrel6.png"
      }), m_State(state), m_Direction(dir) {
    // 步驟一：停止父類別預設的自動播放，我們要自己根據方向與狀態來控制圖片的切換
    Stop();

    // 初始化開始的圖片
    if (auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable)) {
        // 如果起始狀態是掉落，則設為掉落幀 (索引 4)；否則設為滾動幀 (索引 0)
        m_CurrentFrame = (m_State == State::ROLLING) ? 0 : 4;
        anim->SetCurrentFrame(m_CurrentFrame);
    }
}

// =============================================
// 核心更新方法：處理狀態機動作、位置更新與動畫計時
// =============================================
void Barrel::Update() {
    float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()); // 取得經過毫秒數
    float dtSec = dt / 1000.0f; // 換算為秒，用來計算像素移動距離

    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (!anim) return; // 保護機制

    // 更新圖片切換計時器
    m_AnimationTimer += dt;

    // 取得目前的位置，用來套用新的偏移量
    glm::vec2 currentPos = GetPosition();

    // 依據目前狀態更新動畫與位置
    if (m_State == State::ROLLING) {

        // --- 狀態 1：水平滾動 ---

        // 1. 位置更新
        if (m_Direction == Direction::RIGHT) {
            currentPos.x += m_MoveSpeed * dtSec;
        } else {
            currentPos.x -= m_MoveSpeed * dtSec;
        }

        // 2. 動畫更新 (每 100 毫秒切換一張圖)
        if (m_AnimationTimer >= 100.0f) {
            m_AnimationTimer = 0.0f;

            if (m_Direction == Direction::RIGHT) {
                // 向右：0 -> 1 -> 2 -> 3 -> 0 ...
                m_CurrentFrame = (m_CurrentFrame + 1) % 4;
            } else {
                // 向左：3 -> 2 -> 1 -> 0 -> 3 ...
                if (m_CurrentFrame <= 0 || m_CurrentFrame > 3) {
                    m_CurrentFrame = 3; // 修正如果原本在非0~3的範圍
                } else {
                    m_CurrentFrame--;
                }
            }
            anim->SetCurrentFrame(m_CurrentFrame);
        }

    } else if (m_State == State::FALLING_EDGE || m_State == State::FALLING_LADDER) {

        // --- 狀態 2：往下掉落 (邊緣落下或爬梯子掉落) ---

        // 1. 位置更新 (往下移動，Y 軸可能依據引擎座標系為減小或增加，這裡假設 Y 往下變小)
        if (m_Direction == Direction::RIGHT) {
            currentPos.x += m_MoveSpeed * dtSec;
        } else {
            currentPos.x -= m_MoveSpeed * dtSec;
        }
        currentPos.y -= m_FallSpeed * dtSec;

        // 2. 動畫更新 (掉落中的兩張圖片：索引 4 與 5)
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer = 0.0f;

            // 在 4 (Barrel5.png) 與 5 (Barrel6.png) 之間切換
            m_CurrentFrame = (m_CurrentFrame == 4) ? 5 : 4;
            anim->SetCurrentFrame(m_CurrentFrame);
        }
    }

    // 將計算後的新位置更新回角色上
    SetPosition(currentPos);

    // TODO: 外部應在 App.cpp(Main Loop) 或 Map 處理中：
    //  - 偵測是否抵達梯子或邊緣，並呼叫 SetState(State::FALLING_XXX)
    //  - 偵測掉落到底部平台，並切換回 State::ROLLING，再改變方向(SetDirection)
    //  - 偵測與 Mario 的碰撞或是否離開畫面以銷毀物件
}

bool Barrel::IfCollides(const glm::vec2& otherPos, const glm::vec2& otherSize) const {
    const float collisionFactor = 0.7f; // 調整此值以縮小碰撞範圍，例如 0.7 代表 70% 的原始尺寸
    const auto self_half_size = (GetSize() * collisionFactor) / 2.0f;
    const auto other_half_size = otherSize / 2.0f;
    const auto& self_pos = GetPosition();

    return std::abs(self_pos.x - otherPos.x) < (self_half_size.x + other_half_size.x) &&
           std::abs(self_pos.y - otherPos.y) < (self_half_size.y + other_half_size.y);
}
