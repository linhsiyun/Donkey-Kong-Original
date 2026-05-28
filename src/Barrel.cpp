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

    // 【核心修正】初始化時先預設一個極低位置或當前位置
    // 更好的做法是在 App.cpp SetPosition 後再呼叫一個 Init 函式，
    // 或是在 Update 的第一幀同步。
    m_FallStartY = -9999.0f;
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

    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
    if (!anim) return;

    // 取得時間差 (毫秒與秒)
    float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
    float dtSec = dtMs / 1000.0f;

    glm::vec2 currentPos = GetPosition();
    const auto barrel_half_size = GetSize() / 2.0f; // 取得木桶一半尺寸，用於腳底偵測

    if (m_State == State::ROLLING) {
        // --- 狀態 1：水平滾動 ---
        if (m_Direction == Direction::RIGHT) {
            currentPos.x += m_MoveSpeed * dtSec;
        } else {
            currentPos.x -= m_MoveSpeed * dtSec;
        }

        // 動畫更新 (0~3)
        m_AnimationTimer += dtMs;
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer -= 150.0f;
            if (m_Direction == Direction::RIGHT) {
                m_CurrentFrame = (m_CurrentFrame == 3) ? 0 : m_CurrentFrame + 1;
            } else {
                m_CurrentFrame = (m_CurrentFrame == 0) ? 3 : m_CurrentFrame - 1;
            }
            anim->SetCurrentFrame(m_CurrentFrame);
        }

        // 【新增：地形偵測】如果離開了地板，開始掉落！
        if (m_Map) {
            // 探測腳底往下一點點的地形 (給定 2.0f 容錯值)
            TileType footTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y - barrel_half_size.y - 2.0f);

            // 如果腳下變成空氣，木桶從邊緣落下
            if (footTile == TileType::EMPTY) {
                m_State = State::FALLING_EDGE;
                // 記錄開始掉落時的高度
                m_FallStartY = currentPos.y;
                m_CurrentFrame = 4; // 切換至落下圖
                anim->SetCurrentFrame(m_CurrentFrame);
            }
        }

    } else if (m_State == State::FALLING_EDGE || m_State == State::FALLING_LADDER) {
        // --- 狀態 2：往下掉落 ---
        // 為了看起來自然，落下時保留一點點 X 軸的慣性微幅移動
        if (m_Direction == Direction::RIGHT) {
            currentPos.x += (m_MoveSpeed * 0.1f) * dtSec;
        } else {
            currentPos.x -= (m_MoveSpeed * 0.1f) * dtSec;
        }
        currentPos.y -= m_FallSpeed * dtSec;

        // 動畫更新 (4與5輪播)
        m_AnimationTimer += dtMs;
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer -= 150.0f;
            m_CurrentFrame = (m_CurrentFrame == 4) ? 5 : 4;
            anim->SetCurrentFrame(m_CurrentFrame);
        }

        // 【新增：地形偵測】如果碰到地板，著陸並反彈！
        if (m_Map) {
            TileType footTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y - barrel_half_size.y - 2.0f);

            if (footTile == TileType::FLOOR) {
                m_State = State::ROLLING;
                // 【邏輯加強】檢查是否真的「大幅度掉落」
                // 只有當 fallDistance 存在（不為初始值）且超過一層樓高時才反轉
                if (m_FallStartY != -9999.0f) {
                    float fallDistance = m_FallStartY - currentPos.y;
                    float tileHeight = m_Map->GetTileHeight();

                    // 只有掉落高度超過 1.5 倍格子，且這不是剛生成的「第一摔」
                    if (fallDistance > tileHeight * 2.5f) {
                        m_Direction = (m_Direction == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
                    }
                }

                m_CurrentFrame = 0;
                anim->SetCurrentFrame(m_CurrentFrame);
            }
        }
    }

    // 【新增：回收機制】如果木桶掉出螢幕最下方，直接隱藏它
    if (m_Map && currentPos.y < m_Map->GetBottomBoundary() - barrel_half_size.y) {
        SetVisible(false); // 這樣 App 就可以在後台把隱藏的木桶清除掉
    }

    // 寫入最終計算的座標
    SetPosition(currentPos);
}

bool Barrel::IfCollides(const glm::vec2& otherPos, const glm::vec2& otherSize) const {
    const float collisionFactor = 0.7f; // 調整此值以縮小碰撞範圍，例如 0.7 代表 70% 的原始尺寸
    const auto self_half_size = (GetSize() * collisionFactor) / 2.0f;
    const auto other_half_size = otherSize / 2.0f;
    const auto& self_pos = GetPosition();

    return std::abs(self_pos.x - otherPos.x) < (self_half_size.x + other_half_size.x) &&
           std::abs(self_pos.y - otherPos.y) < (self_half_size.y + other_half_size.y);
}
