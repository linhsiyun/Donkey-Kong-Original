#include "Barrel.hpp"
#include "Util/Time.hpp"      // 用來取得每一幀的時間，進行計時與移動計算
#include "Util/Animation.hpp" // 需要透過它來手動控制圖片的幀數
#include "CoordinateManager.hpp"
#include <cstdlib>

// =============================================
// 建構子：接收圖片路徑並初始化初始狀態
// 初始化酒桶的圖片序列 (Barrel1.png ~ Barrel6.png)
// =============================================
Barrel::Barrel(State state, Direction dir, BarrelType type)
: AnimatedCharacter({}), m_State(state), m_Direction(dir), m_Type(type) {

    // 根據型別載入不同圖片
    if (m_Type == BarrelType::BLUE) {
        m_Drawable = std::make_shared<Util::Animation>(std::vector<std::string>{
            RESOURCE_DIR"/Images/BlueBarrel1.png",
            RESOURCE_DIR"/Images/BlueBarrel2.png",
            RESOURCE_DIR"/Images/BlueBarrel3.png",
            RESOURCE_DIR"/Images/BlueBarrel4.png",
            RESOURCE_DIR"/Images/BlueBarrel5.png",
            RESOURCE_DIR"/Images/BlueBarrel6.png"
        }, false, 150, true, 0);
        SetScale({2.7f, 2.7f});
    } else {
        m_Drawable = std::make_shared<Util::Animation>(std::vector<std::string>{
            RESOURCE_DIR"/Images/Barrel1.png", RESOURCE_DIR"/Images/Barrel2.png",
            RESOURCE_DIR"/Images/Barrel3.png", RESOURCE_DIR"/Images/Barrel4.png",
            RESOURCE_DIR"/Images/Barrel5.png", RESOURCE_DIR"/Images/Barrel6.png"
        }, false, 150, true, 0);
        SetScale({2.0f, 2.0f});
    }
    // 步驟一：停止父類別預設的自動播放，我們要自己根據方向與狀態來控制圖片的切換
    Stop();

    // 【核心修正】初始化時先預設一個極低位置或當前位置
    // 更好的做法是在 App.cpp SetPosition 後再呼叫一個 Init 函式，
    // 或是在 Update 的第一幀同步。
    m_FallStartY = -9999.0f;
    // 初始化開始的圖片
    if (auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable)) {
        // 如果起始狀態是掉落，則設為掉落幀 (索引 4)；否則設為滾動幀 (索引 0)
        if (m_Type == BarrelType::BLUE) {
            m_CurrentFrame = 0; // 藍色木桶只有 2 張圖
        } else {
            m_CurrentFrame = (m_State == State::ROLLING) ? 0 : 4;
        }
        anim->SetCurrentFrame(m_CurrentFrame);
    }
}

// =============================================
// 核心更新方法：處理狀態機動作、位置更新與動畫計時
// =============================================
void Barrel::Update() {
    float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
    float dtSec = dtMs / 1000.0f;
    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);

    glm::vec2 currentPos = GetPosition();
    const auto barrel_half_size = GetSize() / 2.0f; // 取得木桶一半尺寸，用於腳底偵測

if (m_Type == BarrelType::BLUE) {
    if (m_State == State::FALLING_EDGE) {
        // --- 1. 物理運動 (同前) ---
        m_VelocityY -= 800.0f * dtSec;
        currentPos.y += m_VelocityY * dtSec;

        // --- 2. 地板偵測與狀態切換 (同前) ---
        if (m_IgnoreFloorTimer > 0.0f) {
            m_IgnoreFloorTimer -= dtSec;
        } else if (m_Map) {
            TileType footTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y - barrel_half_size.y - 2.0f);
            if (footTile == TileType::FLOOR && m_VelocityY < 0.0f) {
                float bottomEngineY = CoordinateManager::LogicToEngine({0.0f, 680.0f}).y;
                if (currentPos.y < bottomEngineY + 10.0f) {
                    m_State = State::ROLLING;
                    m_Direction = Direction::LEFT;
                    m_VelocityY = 0.0f;
                    currentPos.y = bottomEngineY + barrel_half_size.y;

                    m_CurrentFrame = 2; // 強制切換
                    m_AnimationTimer = 0.0f;
                    anim->SetCurrentFrame(m_CurrentFrame);
                } else {
                    m_VelocityY = 150.0f;
                    m_IgnoreFloorTimer = 0.55f;
                }
            }
        }

        // --- 3. 掉落動畫 (索引 0, 1) ---
        m_AnimationTimer += dtMs;
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer -= 150.0f;
            m_CurrentFrame = (m_CurrentFrame == 0) ? 1 : 0;
            anim->SetCurrentFrame(m_CurrentFrame);
        }
    }
    else if (m_State == State::ROLLING) {
        // --- 4. 水平滾動邏輯 ---
        currentPos.x -= m_MoveSpeed * dtSec;

        // --- 5. 滾動動畫 (索引 2-5) ---
        m_AnimationTimer += dtMs;
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer -= 150.0f;
            m_CurrentFrame++;
            if (m_CurrentFrame > 5 || m_CurrentFrame < 2) {
                m_CurrentFrame = 2;
            }
            anim->SetCurrentFrame(m_CurrentFrame);
        }
    }

    SetPosition(currentPos);
    return; // 藍色木桶邏輯至此結束
}

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

        if (m_Map) {
            // 探測點：腳底微下方 (負責確定當前踩著哪一格)
            glm::vec2 footPos = {currentPos.x, currentPos.y - barrel_half_size.y - 2.0f};
            TileType footTile = m_Map->GetTileAtPosition(footPos);

            // 取得當前腳底踩著的網格索引 (col, row)
            auto [col, row] = m_Map->GetTileIndexAtPosition(footPos);

            // 取得「正下方那一格 (row + 1)」的地形，這就是看穿地板的核心！
            TileType tileBelow = m_Map->GetLevelData().GetTile(col, row + 8);

            // 1. 如果腳下變成空氣，木桶從邊緣落下
            if (footTile == TileType::EMPTY) {
                m_State = State::FALLING_EDGE;
                m_FallStartY = currentPos.y;
                m_LastCheckedLadderCol = -1; // 離開邊緣時重置梯子紀錄

                m_IgnoreFloorTimer = 0.0f;
                anim->SetCurrentFrame(4);
            }
            // 2. 梯子判定：檢查【正下方那一格 (tileBelow)】是不是梯子！
            else if (tileBelow == TileType::LADDER || tileBelow == TileType::BROKEN_LADDER) {

                // 取得正下方梯子格子的中心點 (用來對齊 X 軸)
                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(col, row + 1);

                // 將容錯值設為 12.0f 左右比較平滑，避免滾太快錯過判定
                if (std::abs(currentPos.x - gridCenter.x) < 12.0f && col != m_LastCheckedLadderCol) {
                    m_LastCheckedLadderCol = col;

                    // 以正下方的梯子類型來決定機率
                    int dropChance = (tileBelow == TileType::BROKEN_LADDER) ? 30 : 50;
                    // int dropChance = 100; // 測試用：只要對齊就必掉！

                    if ((std::rand() % 100) < dropChance) {
                        m_State = State::FALLING_EDGE;
                        m_FallStartY = currentPos.y;
                        currentPos.x = gridCenter.x; // 強制對齊梯子中心

                        m_VelocityY = -50.0f;

                        m_IgnoreFloorTimer = 25.0f;
                        m_CurrentFrame = 4;
                        anim->SetCurrentFrame(m_CurrentFrame);
                    }
                }
            }
            // 3. 在一般地板 (或梯子底部) 上滾動時，重置梯子標記
            else {
                m_LastCheckedLadderCol = -1;
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
        float frameDrop = m_FallSpeed * dtSec;
        currentPos.y -= m_FallSpeed * dtSec;

        // 動畫更新 (4與5輪播)
        m_AnimationTimer += dtMs;
        if (m_AnimationTimer >= 150.0f) {
            m_AnimationTimer -= 150.0f;
            m_CurrentFrame = (m_CurrentFrame == 4) ? 5 : 4;
            anim->SetCurrentFrame(m_CurrentFrame);
        }

        if (m_IgnoreFloorTimer > 0.0f) {
            // 如果還在穿透距離內，持續扣除掉落距離，且「不偵測地板」
            m_IgnoreFloorTimer -= frameDrop;
        }

        // 【新增：地形偵測】如果碰到地板，著陸並反彈！
        else if (m_Map) {
            TileType footTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y - barrel_half_size.y - 2.0f);

            if (footTile == TileType::FLOOR) {
                m_State = State::ROLLING;
                // 【邏輯加強】檢查是否真的「大幅度掉落」
                // 只有當 fallDistance 存在（不為初始值）且超過一層樓高時才反轉
                if (m_FallStartY != -9999.0f) {
                    float fallDistance = m_FallStartY - currentPos.y;
                    float tileHeight = m_Map->GetTileHeight();

                    // 只有掉落高度超過 2.5 倍格子，且這不是剛生成的「第一摔」
                    if (fallDistance > tileHeight * 2.5f) {
                        m_Direction = (m_Direction == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
                    }
                    m_FallStartY = -9999.0f;
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