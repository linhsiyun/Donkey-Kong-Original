#include "Fiamma.hpp"
#include "CoordinateManager.hpp"
#include "Util/Time.hpp"
#include <cstdlib>

Fiamma::Fiamma() : AnimatedCharacter({
    RESOURCE_DIR"/Images/fiamma1.png",
    RESOURCE_DIR"/Images/fiamma2.png"
}) {
    // 設定初始屬性
    SetZIndex(45);           // 稍微位於 Mario (50) 後方
    SetScale({2.2f, 2.2f});  // 縮放比例

    // 設定動畫參數
    SetLooping(true);
    SetInterval(200);        // 每 200ms 切換一次圖片
    Play();
    m_State = State::FALLING;
}

void Fiamma::Update() {
    float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
    float dtSec = dtMs / 1000.0f;
    auto anim = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);

    glm::vec2 currentPos = GetPosition();
    glm::vec2 size = GetSize();
    float footY = currentPos.y - (size.y / 2.0f);
    float scaleRatio = CoordinateManager::GetScaleRatio();

    if (m_State == State::FALLING) {
        // --- 掉落邏輯 ---
        currentPos.y -= 150.0f * dtSec;
        footY = currentPos.y - (size.y / 2.0f);

        if (m_Map) {
            TileType footTile = m_Map->GetTileAtPosition(currentPos.x, footY - (2.0f * scaleRatio));
            if (footTile == TileType::FLOOR || footTile == TileType::LADDER || footTile == TileType::BROKEN_LADDER) {
                m_State = State::WALKING;
                auto [col, row] = m_Map->GetTileIndexAtPosition(currentPos.x, footY - (2.0f * scaleRatio));
                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(col, row);
                currentPos.y = gridCenter.y + (m_Map->GetTileHeight() / 2.0f) + (size.y / 2.0f);
            }
        }
    }
    else if (m_State == State::WALKING) {
        // --- 走路邏輯 ---
        m_RandomTurnTimer -= dtSec;
        if (m_RandomTurnTimer <= 0.0f) {
            m_Direction = (std::rand() % 2 == 0) ? Direction::LEFT : Direction::RIGHT;
            m_RandomTurnTimer = 1.0f + (std::rand() % 200) / 100.0f;
        }

        if (m_Direction == Direction::RIGHT) {
            currentPos.x += m_MoveSpeed * dtSec;
            SetScale({std::abs(GetScale().x), GetScale().y});
        } else {
            currentPos.x -= m_MoveSpeed * dtSec;
            SetScale({-std::abs(GetScale().x), GetScale().y});
        }

        if (m_Map) {
            float checkY = footY - (2.0f * scaleRatio);
            auto [col, baseRow] = m_Map->GetTileIndexAtPosition(currentPos.x, checkY);

            auto [centerCol, centerRow] = m_Map->GetTileIndexAtPosition(currentPos.x, currentPos.y);
            TileType centerTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y);

            // A. 【修復判定 Bug】：只在離開梯子的網格時，才允許重新骰機率
            if (centerCol != m_LastCheckedLadderCol && centerTile != TileType::LADDER && centerTile != TileType::BROKEN_LADDER) {
                m_LastCheckedLadderCol = -1;
            }

            // B. 遇到梯子依機率爬行
            if (centerTile == TileType::LADDER || centerTile == TileType::BROKEN_LADDER) {
                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(centerCol, centerRow);

                // 靠近梯子中心，且還沒對這個梯子骰過機率
                if (std::abs(currentPos.x - gridCenter.x) < (8.0f * scaleRatio) && centerCol != m_LastCheckedLadderCol) {

                    m_LastCheckedLadderCol = centerCol; // 標記已判定，避免重複觸發

                    int roll = std::rand() % 100;
                    bool shouldClimb = false;

                    // 依照梯子種類給予不同機率
                    if (centerTile == TileType::BROKEN_LADDER) {
                        if (roll < 30) shouldClimb = true; // 30% 機率爬破損梯子
                    } else if (centerTile == TileType::LADDER) {
                        if (roll < 50) shouldClimb = true; // 50% 機率爬正常梯子
                    }

                    if (shouldClimb) {
                        m_State = State::CLIMBING;
                        currentPos.x = gridCenter.x;
                        m_RandomTurnTimer = static_cast<float>(baseRow);
                    }
                }
            }

            // C. 地板吸附與邊界防呆
            if (m_State == State::WALKING) {
                int floorRow = -1;

                // 【修復厚地板與斜坡】：智慧型掃描
                // 優先檢查上方(baseRow - 1)，但只接受 FLOOR，避免被半空的梯子吸上去
                TileType upTile = (baseRow - 1 >= 0) ? m_Map->GetLevelData().GetTile(col, baseRow - 1) : TileType::EMPTY;

                if (upTile == TileType::FLOOR) {
                    floorRow = baseRow - 1; // 完美爬上坡 / 浮出地板
                } else {
                    TileType currentTile = (baseRow >= 0 && baseRow < m_Map->GetLevelData().GetHeight()) ? m_Map->GetLevelData().GetTile(col, baseRow) : TileType::EMPTY;
                    if (currentTile == TileType::FLOOR || currentTile == TileType::LADDER || currentTile == TileType::BROKEN_LADDER) {
                        floorRow = baseRow; // 平地行走
                    } else {
                        TileType downTile = (baseRow + 1 < m_Map->GetLevelData().GetHeight()) ? m_Map->GetLevelData().GetTile(col, baseRow + 1) : TileType::EMPTY;
                        if (downTile == TileType::FLOOR || downTile == TileType::LADDER || downTile == TileType::BROKEN_LADDER) {
                            floorRow = baseRow + 1; // 走下坡
                        } else {
                            TileType downDownTile = (baseRow + 2 < m_Map->GetLevelData().GetHeight()) ? m_Map->GetLevelData().GetTile(col, baseRow + 2) : TileType::EMPTY;
                            if (downDownTile == TileType::FLOOR || downDownTile == TileType::LADDER || downDownTile == TileType::BROKEN_LADDER) {
                                floorRow = baseRow + 2; // 陡降坡
                            }
                        }
                    }
                }

                if (floorRow != -1) {
                    glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(col, floorRow);
                    currentPos.y = gridCenter.y + (m_Map->GetTileHeight() / 2.0f) + (size.y / 2.0f);
                } else {
                    // 走到懸崖邊緣防呆轉向
                    m_Direction = (m_Direction == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
                    currentPos.x += (m_Direction == Direction::RIGHT) ? (5.0f * scaleRatio) : -(5.0f * scaleRatio);
                    m_RandomTurnTimer = 2.0f;
                }
            }
        }
    }
    else if (m_State == State::CLIMBING) {
        // --- 爬梯邏輯 ---
        currentPos.y += m_ClimbSpeed * dtSec;

        if (m_Map) {
            footY = currentPos.y - (size.y / 2.0f);
            auto [footCol, footRow] = m_Map->GetTileIndexAtPosition(currentPos.x, footY - (2.0f * scaleRatio));

            TileType footTile = TileType::EMPTY;
            if (footCol >= 0 && footCol < m_Map->GetLevelData().GetWidth() && footRow >= 0 && footRow < m_Map->GetLevelData().GetHeight()) {
                footTile = m_Map->GetLevelData().GetTile(footCol, footRow);
            }

            // unused, auto [centerCol, centerRow] = m_Map->GetTileIndexAtPosition(currentPos.x, currentPos.y);
            TileType centerTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y);

            bool isPlatform = false;
            if (footTile == TileType::FLOOR || footTile == TileType::LADDER || footTile == TileType::BROKEN_LADDER) {
                TileType leftTile = (footCol - 1 >= 0) ? m_Map->GetLevelData().GetTile(footCol - 1, footRow) : TileType::EMPTY;
                TileType rightTile = (footCol + 1 < m_Map->GetLevelData().GetWidth()) ? m_Map->GetLevelData().GetTile(footCol + 1, footRow) : TileType::EMPTY;

                if (leftTile == TileType::FLOOR || rightTile == TileType::FLOOR) {
                    isPlatform = true;
                }
            }

            // 【關鍵 2】：計算「爬了幾個網格」(Row 的數值越往上越小)
            int startRow = static_cast<int>(m_RandomTurnTimer);
            // 由於一層樓的間距約 8 個 Row，我們要求它至少往上爬 5 個 Row 才算真正離開底層
            bool hasLeftBottomFloor = (startRow - footRow) >= 5;

            // 必須離開底層 + 碰到真正的平台 + 身體已經探出梯子頂部 才能停下
            if (isPlatform && centerTile != TileType::LADDER && centerTile != TileType::BROKEN_LADDER && hasLeftBottomFloor) {
                m_State = State::WALKING;
                m_LastCheckedLadderCol = -1;
                m_Direction = (std::rand() % 2 == 0) ? Direction::LEFT : Direction::RIGHT;
                m_RandomTurnTimer = 1.0f + (std::rand() % 200) / 100.0f;

                int targetRow = footRow;
                while (targetRow - 1 >= 0 && m_Map->GetLevelData().GetTile(footCol, targetRow - 1) == TileType::FLOOR) {
                    targetRow--;
                }

                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(footCol, footRow);
                currentPos.y = gridCenter.y + (m_Map->GetTileHeight() / 2.0f) + (size.y / 2.0f);
            }
        }
    }

    SetPosition(currentPos);
}