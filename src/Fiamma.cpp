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

        if (m_Map) {
            float checkY = footY - (2.0f * scaleRatio);
            auto [col, baseRow] = m_Map->GetTileIndexAtPosition(currentPos.x, checkY);
            TileType centerTile = m_Map->GetTileAtPosition(currentPos.x, currentPos.y);

            bool isOnLadderX = false; // [修正] 必須先宣告變數

            // B. 遇到梯子機率向上爬
            if (centerTile == TileType::LADDER || centerTile == TileType::BROKEN_LADDER) {
                auto [centerCol, centerRow] = m_Map->GetTileIndexAtPosition(currentPos.x, currentPos.y);
                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(centerCol, centerRow);

                if (std::abs(currentPos.x - gridCenter.x) < (8.0f * scaleRatio)) {
                    isOnLadderX = true; // [修正] 標記正在梯子範圍內
                    if (centerCol != m_LastCheckedLadderCol) {
                        m_LastCheckedLadderCol = centerCol;
                        if ((std::rand() % 100) < 25) { 
                            m_State = State::CLIMBING;
                            m_ClimbDir = VerticalDirection::UP;
                            currentPos.x = gridCenter.x;
                            m_RandomTurnTimer = static_cast<float>(baseRow);
                        }
                    }
                }
            }
            // C. 碰到地板下方有梯子，機率向下爬
            else {
                TileType tileBelow = m_Map->GetLevelData().GetTile(col, baseRow + 10);
         
                if (tileBelow == TileType::LADDER || tileBelow == TileType::BROKEN_LADDER) {
                    auto [centerCol, centerRow] = m_Map->GetTileIndexAtPosition(currentPos.x, currentPos.y);
                    glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(centerCol, centerRow);

                    if (std::abs(currentPos.x - gridCenter.x) < (8.0f * scaleRatio)) {
                        isOnLadderX = true; // [修正] 標記正在梯子範圍內
                        if (centerCol != m_LastCheckedLadderCol) {
                            m_LastCheckedLadderCol = centerCol; 
                            if ((std::rand() % 100) < 25) { 
                                m_State = State::CLIMBING;
                                m_ClimbDir = VerticalDirection::DOWN;
                                currentPos.x = gridCenter.x;
                                m_RandomTurnTimer = static_cast<float>(baseRow);
                            }
                        }
                    }
                }
            }

            // 如果完全不在梯子 X 範圍內，重置標記，允許火球之後爬下一根梯子
            if (!isOnLadderX) {
                m_LastCheckedLadderCol = -1;
            }

            if (m_State == State::WALKING) {
                // 只有確定還在 WALKING 狀態，才套用水平位移
                if (m_Direction == Direction::RIGHT) {
                    currentPos.x += m_MoveSpeed * dtSec;
                    SetScale({std::abs(GetScale().x), GetScale().y});
                } else {
                    currentPos.x -= m_MoveSpeed * dtSec;
                    SetScale({-std::abs(GetScale().x), GetScale().y});
                }

                int floorRow = -1;
                // 修正：只檢查當前網格(baseRow)與正下方一格(baseRow+1)，避免吸附到上方的梯子或平台
                for (int r = baseRow; r <= baseRow + 1; ++r) {
                    if (r >= 0 && r < m_Map->GetLevelData().GetHeight()) {
                        TileType type = m_Map->GetLevelData().GetTile(col, r);
                        if (type == TileType::FLOOR || type == TileType::LADDER || type == TileType::BROKEN_LADDER) {
                            floorRow = r;
                            break;
                        }
                    }
                }

                if (floorRow != -1) {
                    // 取得地板的精準座標
                    glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(col, floorRow);
                    float targetY = gridCenter.y + (m_Map->GetTileHeight() / 2.0f) + (size.y / 2.0f);

                    // 只有當位移偏差大於一定程度時才進行 Y 軸修正，減少微小抖動（蛇行）
                    if (std::abs(currentPos.y - targetY) > 1.0f) {
                        currentPos.y = targetY;
                        LOG_DEBUG("w y snap: {}", currentPos.y);
                    }
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

        // 【修正 1】強制鎖定 X 座標，防止爬行時蛇行
        if (m_Map && m_LastCheckedLadderCol != -1) {
            // 使用起始時記憶的 Col 來取得精準中心
            glm::vec2 ladderCenter = m_Map->GetGridToWorldPosition(m_LastCheckedLadderCol, 0);
            currentPos.x = ladderCenter.x;
        }

        if (m_ClimbDir == VerticalDirection::UP) {
            currentPos.y += m_ClimbSpeed * dtSec;
        } else {
            currentPos.y -= m_ClimbSpeed * dtSec;
        }

        if (m_Map) {
            footY = currentPos.y - (size.y / 2.0f);
            auto [footCol, footRow] = m_Map->GetTileIndexAtPosition(currentPos.x, footY - (2.0f * scaleRatio));

            auto& levelData = m_Map->GetLevelData();
            auto getTile = [&](int c, int r) {
                if (c < 0 || c >= levelData.GetWidth() || r < 0 || r >= levelData.GetHeight()) return TileType::EMPTY;
                return levelData.GetTile(c, r);
            };

            TileType footTile = getTile(footCol, footRow);
            //unused, TileType tileBelow = getTile(footCol, footRow + 1);

            // 判定是否為平台表面 (檢查左右是否有地板)
            auto checkIsPlatform = [&](int c, int r) {
                TileType current = getTile(c, r);
                if (current != TileType::FLOOR && current != TileType::LADDER) return false;
                return (getTile(c - 1, r) == TileType::FLOOR || getTile(c + 1, r) == TileType::FLOOR);
            };

            int startRow = static_cast<int>(m_RandomTurnTimer);
            bool hasMovedEnough = std::abs(startRow - footRow) >= 10; 
            bool shouldStopClimbing = false;
            int snapRow = footRow;

            if (m_ClimbDir == VerticalDirection::UP) {
                // 向上爬：必須「腳底脫離固體地板」且「下方那一格是平台表面」才停止
                // 這能確保 Fiamma 爬過 8 tiles 厚的地板，站到最頂層
                if (hasMovedEnough && footTile != TileType::FLOOR && footTile != TileType::LADDER && checkIsPlatform(footCol, footRow + 1)) {
                    shouldStopClimbing = true;
                    snapRow = footRow + 1; // 站在下方那一格的地板上
                }
            } else {
                // 向下爬：腳底踩到地板平台即停止
                if (hasMovedEnough && checkIsPlatform(footCol, footRow)) {
                    shouldStopClimbing = true;
                    snapRow = footRow; // 腳就在這一格
                }
            }

            if (shouldStopClimbing) {
                m_State = State::WALKING;
                m_LastCheckedLadderCol = footCol;
                m_Direction = (std::rand() % 2 == 0) ? Direction::LEFT : Direction::RIGHT;
                m_RandomTurnTimer = 1.0f + (std::rand() % 200) / 100.0f;
                
                // 根據 snapRow 精準對齊地板表面
                glm::vec2 gridCenter = m_Map->GetGridToWorldPosition(footCol, snapRow);
                currentPos.y = gridCenter.y + (m_Map->GetTileHeight() / 2.0f) + (size.y / 2.0f);
            }
        }
    }

    SetPosition(currentPos);
}