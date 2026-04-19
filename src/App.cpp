#include "App.hpp"
#include "Map.hpp"
//#include "Util/Image.hpp"
#include "Mario.hpp"
#include "TileType.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Time.hpp"
#include "Util/Logger.hpp"
#include "config.hpp"
#include "Setting.hpp"
#include <cstdlib> // 加入 rand()

// 地面上的槌子道具物件
static std::shared_ptr<Character> m_HammerItem;
static std::shared_ptr<Character> m_HammerItem2;

void App::LoadLevel(int level) {
    m_CurrentLevel = level;

    // 1. 根據傳入的關卡編號，載入對應的地圖圖片與純文字檔 (MapX.txt)
    // 同時把所有物件 (Mario, 火球, 道具, 大金剛) 移動到該關卡適合的座標
    if (m_CurrentLevel == 1) {
        m_Map->LoadNewMap("../Resources/Images/board-barrels.png", "../Resources/Maps/Map1.txt");

        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;

        // 設定螢幕邊界，讓 Mario 的 Update 邏輯可以進行限制
        m_Mario->SetScreenBounds(halfWidth, halfHeight);

        // 第一關的各角色與道具初始位置
        m_Mario->SetPosition({-halfWidth + 50.0f, -halfHeight + 45.0f});
        m_Fireball->SetPosition({-100.0f, -70.0f});
        if (m_HammerItem) m_HammerItem->SetPosition({150.0f, -135.0f});
        if (m_HammerItem2) m_HammerItem2->SetPosition({-halfWidth + 80.0f, halfHeight - 180.0f});
        m_DonkeyKong->SetPosition({-halfWidth + 80.0f, halfHeight - 100.0f});

    } else if (m_CurrentLevel == 2) {
        m_Map->LoadNewMap("../Resources/Images/board-conveyors.png", "../Resources/Maps/Map2.txt");

        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;

        // 設定螢幕邊界，讓 Mario 的 Update 邏輯可以進行限制
        m_Mario->SetScreenBounds(halfWidth, halfHeight);

        // 第二關的各角色與道具初始位置 (目前暫時設定與第一關相同，之後你可以自由調整這組座標)
        m_Mario->SetPosition({-halfWidth + 150.0f, -halfHeight + 50.0f});
        m_Fireball->SetPosition({-100.0f, -70.0f});
        if (m_HammerItem) m_HammerItem->SetPosition({150.0f, -120.5f});
        if (m_HammerItem2) m_HammerItem2->SetPosition({-halfWidth + 180.0f, halfHeight - 180.0f});
        m_DonkeyKong->SetPosition({-halfWidth + 180.0f, halfHeight - 100.0f});
    } else if (m_CurrentLevel == 3) {
        m_Map->LoadNewMap("../Resources/Images/board-elevators.png", "../Resources/Maps/Map3.txt");

        //TODO
    } else if (m_CurrentLevel == 4) {
        m_Map->LoadNewMap("../Resources/Images/board-rivets.png", "../Resources/Maps/Map4.txt");

        //TODO
    }
    LOG_INFO("map halfWidth: {}, halfHeight: {}", halfWidth, halfHeight);

    // 2. 清除畫面上現有的所有酒桶
    for (auto& barrel : m_Barrels) {
        m_Renderer.RemoveChild(barrel);
    }
    m_Barrels.clear();

    // 3. 重置 Mario 以及其它遊戲角色的狀態
    m_Mario->IDLE(); // 強制解除勝利狀態，回到預設的閒置
    m_Mario->SetDonkeyKongBounds(m_DonkeyKong->GetPosition(), m_DonkeyKong->GetSize());

    // 4. 重置火球與道具狀態 (設定為可見)
    m_Fireball->SetVisible(true);

    if (m_HammerItem) m_HammerItem->SetVisible(true);
    if (m_HammerItem2) m_HammerItem2->SetVisible(true);

    // 5. 重置 HUD 資訊
    if (m_HUDText) {
        m_HUDText->Init();                  // 重置分數為 0 且 Bonus 為 5000
        m_HUDText->SetLevel(m_CurrentLevel); // 更新畫面上的 L=XX 文字
    }
}

void App::Start() {
    LOG_TRACE("Start");

    // 初始化地圖，並加入到 Renderer 渲染清單中
    m_Map = std::make_shared<Map>("../Resources/Images/board-barrels.png", "../Resources/Maps/Map1.txt");
    m_Renderer.AddChild(m_Map);

    // 透過 PTSD_Config 取得視窗大小
// #if VSCODE
//     halfWidth = static_cast<float>(PTSD_Config::WINDOW_WIDTH) / 2.0f;
//     halfHeight = static_cast<float>(PTSD_Config::WINDOW_HEIGHT) / 2.0f;
// #else
//     halfWidth = static_cast<float>(WINDOW_WIDTH) / 2.0f;
//     halfHeight = static_cast<float>(WINDOW_HEIGHT) / 2.0f;
// #endif
//     LOG_INFO("halfWidth: {}, halfHeight: {}", halfWidth, halfHeight);


    // 初始化 Mario 物件
    m_Mario = std::make_shared<Mario>();

    // 把 Mario 裡面所有的圖層一口氣加進 App 的Renderer中
    m_Mario->AddToRenderer(m_Renderer);

    // 初始化火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Renderer.AddChild(m_Fireball);

    // 初始化地面上的槌子道具並放在右側
    m_HammerItem = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem);

    // 初始化第二個槌子道具，放在靠近酒桶滾動的路徑上 (測試用)
    m_HammerItem2 = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem2->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem2);

    // 初始化text物件
    m_HUDText = std::make_shared<HUDManager>();
    m_HUDText->Init();
    m_HUDText->AddToRenderer(m_Renderer);

    // 初始化 DonkeyKong 物件
    m_DonkeyKong = std::make_shared<DonkeyKong>();
    m_DonkeyKong->SetZIndex(50); // 可選：調整圖層順序
    m_DonkeyKong->SetScale({m_Mario->marioScale/1.5f, m_Mario->marioScale/1.5f});
#if 1 //TODO
    // 設定產出木桶的回呼行為 (callback function)
    // [this] 捕捉 this 指標，代表在此 Lambda 裡面可以呼叫及使用 App 的成員函式與變數 (如 this->SpawnBarrel)
    m_DonkeyKong->SetBarrelSpawnCallback([this]() {
        this->SpawnBarrel();
    });
#endif
    m_Renderer.AddChild(m_DonkeyKong);

    // 載入當前關卡 (這會負責載入地圖、設定角色的初始位置與重置狀態，也處理 DonkeyKong 給 Mario 的邊界傳遞)
    LoadLevel(m_CurrentLevel);

    // 設定 App 物件初始狀態為 UPDATE，開始遊戲主迴圈
    m_CurrentState = State::UPDATE;
    LOG_TRACE("UPDATE");
}

// 這是我們將原本寫在 App::Start 裡面的 Lambda ({...}) 抽出來的一般成員函式
// 這樣寫可以讓程式碼比較好讀，不會讓 App::Start 太肥大，同時如果有其他地方需要產酒桶也可以重複呼叫。
void App::SpawnBarrel() {
    LOG_DEBUG("++barrel");

    // 產生一個新的酒桶
    auto newBarrel = std::make_shared<Barrel>(Barrel::State::ROLLING, Barrel::Direction::RIGHT);

    // 1. 必須先設定縮放，後續呼叫 GetSize() 才能取得縮放後的正確尺寸
    newBarrel->SetScale({m_Mario->marioScale / 1.5f, m_Mario->marioScale / 1.5f});
    newBarrel->SetZIndex(40);

    // 2. 取得大金剛與酒桶的相關資訊
    glm::vec2 dkPos = m_DonkeyKong->GetPosition();
    glm::vec2 dkSize = m_DonkeyKong->GetSize();
    glm::vec2 barrelSize = newBarrel->GetSize();

    // 3. 計算「腳邊」座標
    // X 軸：放在大金剛中心點右側，加上兩者寬度的一半再加上一點間隙 (5.0f)
    float spawnX = dkPos.x + (dkSize.x / 2.0f) + 5.0f;
    // Y 軸：大金剛的腳底 (dkPos.y - dkSize.y/2) 加上酒桶的一半高度，使其底部齊平
    float spawnY = (dkPos.y - (dkSize.y / 2.0f)) + (barrelSize.y / 2.0f);

    newBarrel->SetPosition({spawnX, spawnY});

    // 將酒桶暫存在清單維護並加入畫面繪製的根節點
    m_Barrels.push_back(newBarrel);
    m_Renderer.AddChild(newBarrel);
}

void App::Update() {

#if 1  //sdbg: 按下 N 鍵切換到下一關測試, 按下 R 鍵 reset
    if (Util::Input::IsKeyDown(Util::Keycode::N)) {
        if (m_CurrentLevel != 4) {
            m_CurrentLevel++;
        } else {
            m_CurrentLevel = 1;
        }
        LoadLevel(m_CurrentLevel);
    }
    else if (Util::Input::IsKeyDown(Util::Keycode::R)) {
        LoadLevel(m_CurrentLevel);
    }
#endif

    // 1. 取得當前狀態，判定是否處於「可遊玩」狀態
    MarioState marioState = m_Mario->GetState();
    bool isPlaying = (marioState != MarioState::DEAD && marioState != MarioState::WIN);

    // 當目前已經通關，並且處於勝利狀態時，我們讓玩家按下按鈕後可以自動進到下一關
    if (marioState == MarioState::WIN) {
        // 等待玩家按下任意前進按鈕 (例如跳躍鍵 SPACE 或 RETURN 鍵)
        if (Util::Input::IsKeyDown(Util::Keycode::SPACE) || Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
            if (m_CurrentLevel != 4) {
                m_CurrentLevel++;
                LoadLevel(m_CurrentLevel);
            } else {
                // TODO
            }
        }
    }

    // 告訴渲染器，把所有 AddChild 進來的物件畫到畫面上 (包含你的地圖)
    if (isPlaying) {
#if 1 //sdbg
        // 2. 更新 DonkeyKong (若停止更新，其產酒桶的回呼就不會觸發)
        if (m_DonkeyKong) {
            m_DonkeyKong->Update();
        }
#endif
        // 更新畫面上的所有酒桶
        for (auto it = m_Barrels.begin(); it != m_Barrels.end(); ) {
            auto& barrel = *it;
            barrel->Update();

            // 8. 碰撞偵測：Mario 與酒桶
            glm::vec2 marioSize = m_Mario->GetSize();
            // 如果正在揮槌子，擴大碰撞盒以便更容易擊碎障礙物
            if (marioState == MarioState::HAMMERING) marioSize *= 1.8f;

            if (barrel->IfCollides(m_Mario->GetPosition(), marioSize)) {
                if (marioState == MarioState::HAMMERING) {
                    LOG_DEBUG("BARREL DESTROYED BY HAMMER!");
                    m_Renderer.RemoveChild(barrel); // 從渲染器移除
                    it = m_Barrels.erase(it);       // 從清單移除
                    m_HUDText->AddScore(500);
                    continue;                       // 跳過後續處理，直接檢查下一個酒桶
                } else {
                    m_Mario->Dead();                // Mario 死亡
                }
            }

            // 取得酒桶座標與尺寸
            glm::vec2 pos = barrel->GetPosition();
            glm::vec2 size = barrel->GetSize();

            // === 新增：斜坡/地表偵測邏輯 (修正酒桶在傾斜地板上誤判為 FALLING_EDGE 的問題) ===
            float barrelFootY = pos.y - (size.y / 2.0f); // 酒桶當前腳底 Y 座標
            float targetFootY = barrelFootY;
            bool foundSurface = false;
            const float tileH = m_Map->GetTileHeight();
            const float searchRange = tileH * 1.5f; // 搜尋範圍設定為 1.5 格高，確保跨越傾斜落差

            // [情況 A：向上修正 (處理上坡，防止酒桶陷入地板)]
            if (m_Map->GetTileAtPosition(pos.x, barrelFootY) == TileType::FLOOR) {
                for (float dy = 1.0f; dy <= searchRange; dy += 1.0f) {
                    TileType tile = m_Map->GetTileAtPosition(pos.x, barrelFootY + dy + 1.0f);
                    if (tile == TileType::EMPTY || tile == TileType::LADDER) {
                        targetFootY = barrelFootY + dy;
                        foundSurface = true;
                        break;
                    }
                }
            }
            // [情況 B：向下修正 (處理下坡，防止酒桶因坡度誤判為落下)]
            else {
                for (float dy = 1.0f; dy <= searchRange; dy += 1.0f) {
                    if (m_Map->GetTileAtPosition(pos.x, barrelFootY - dy) == TileType::FLOOR) {
                        targetFootY = barrelFootY - dy + 1.0f; // 吸附至地板表面上方 1 像素
                        foundSurface = true;
                        break;
                    }
                }
            }

            // 取得酒桶下方格子類型 (用於判斷是否經過梯子)
            TileType footTile = m_Map->GetTileAtPosition(pos.x, targetFootY - 2.0f);

            // 狀態機邏輯：依照酒桶當前狀態與腳下地形做不同反應
            if (barrel->GetState() == Barrel::State::ROLLING) {
                if (!foundSurface) {
                    // [狀況 1] 真的沒地板了 (在搜尋範圍內無 FLOOR): 切換至從邊緣掉落狀態
                    barrel->SetState(Barrel::State::FALLING_EDGE);
                    LOG_DEBUG("Barrel edge fall at X: {}", pos.x);
                } else {
                    // 若偵測到地板，則更新酒桶 Y 座標以實現吸附效果，使其沿斜坡平滑移動
                    barrel->SetPosition({pos.x, targetFootY + (size.y / 2.0f)});
                    pos = barrel->GetPosition(); // 同步位置資訊

                    if (footTile == TileType::LADDER) {
                        // [狀況 2] 碰到梯子: 設定隨機機率決定是否沿梯子落下
                        if (rand() % 100 < 25) {
                            barrel->SetState(Barrel::State::FALLING_LADDER);
                            LOG_DEBUG("Barrel ladder fall at X: {}", pos.x);
                        }
                    }
                }
            }
            else if (barrel->GetState() == Barrel::State::FALLING_EDGE ||
                     barrel->GetState() == Barrel::State::FALLING_LADDER) {
                // [狀況 3] 如果正在掉落，且偵測到可著陸的地表 (foundSurface):
                // 則終止落下，且恢復滾動狀態。
                if (foundSurface) {
                    barrel->SetState(Barrel::State::ROLLING);
                    // 根據遊戲邏輯，碰到下一層平台時需反轉移動方向
                    if (barrel->GetDirection() == Barrel::Direction::RIGHT) {
                        barrel->SetDirection(Barrel::Direction::LEFT);
                    } else {
                        barrel->SetDirection(Barrel::Direction::RIGHT);
                    }
                    LOG_DEBUG("Barrel hit floor, continue rolling.");
                }
            }

            // 判定是否超出邊界（增加 50.0f 的緩衝，讓酒桶完全消失在畫面外再刪除）
            if (std::abs(pos.x) > halfWidth + 50.0f || std::abs(pos.y) > halfHeight + 50.0f) {
                LOG_DEBUG("--barrel");
                m_Renderer.RemoveChild(barrel); // 1. 從渲染器移除
                it = m_Barrels.erase(it);       // 2. 從 vector 移除，並取得指向下一個元素的迭代器
            } else {
                ++it; // 沒被刪除時才手動增加迭代器
            }
        }

        // 在處理 Mario 的移動邏輯之前，先更新 Donkey Kong 的邊界資訊給 Mario
        m_Mario->SetDonkeyKongBounds(m_DonkeyKong->GetPosition(), m_DonkeyKong->GetSize());

        // 3. 處理 Mario 輸入  // TODO: clime_idle 也不能 jump...
        if ((m_Mario->GetState() != MarioState::JUMPING)
            && (m_Mario->GetState() != MarioState::CLIMBING)
            && (m_Mario->GetState() != MarioState::HAMMERING) // 拿槌子時禁止跳躍
            && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
            m_Mario->JumpStart();
        }

        // 1. 先執行移動邏輯 (讓座標更新到這一幀的目標位置)
        if (m_Mario->IsJumping()) {
            m_Mario->Jump();
        }
        else if (m_Mario->GetState() == MarioState::FALLING) {
             // 處理 FALLING 狀態下的位移（這部分原本散落在 Update 各處，建議統一先移動）
        }

        // 2. 移動後，再取得最新位置進行地表偵測
        glm::vec2 marioPos = m_Mario->GetPosition();
        glm::vec2 marioSize = m_Mario->GetSize();
        float footY = marioPos.y - (marioSize.y / 2.0f);
        float targetFootY = footY;
        bool foundSurface = false;
        const float tileH = m_Map->GetTileHeight();

        // 跳躍中搜尋範圍可以稍微加大，避免高速下落穿透
        const float searchRange = m_Mario->IsJumping() ? tileH * 2.0f : tileH * 1.5f;

        if (m_Map->GetTileAtPosition(marioPos.x, footY) == TileType::FLOOR) {
            for (float dy = 1.0f; dy <= searchRange; dy += 1.0f) {
                TileType tile = m_Map->GetTileAtPosition(marioPos.x, footY + dy + 1.0f);
                if (tile == TileType::EMPTY || tile == TileType::LADDER) {
                    targetFootY = footY + dy;
                    foundSurface = true;
                    break;
                }
            }
        } else {
            for (float dy = 1.0f; dy <= searchRange; dy += 1.0f) {
                if (m_Map->GetTileAtPosition(marioPos.x, footY - dy) == TileType::FLOOR) {
                    targetFootY = footY - dy + 1.0f;
                    foundSurface = true;
                    break;
                }
            }
        }

        // 3. 根據偵測結果判定狀態切換
        if (m_Mario->IsJumping()) {
            // [跳躍落地判定]
            // 這裡除了計時器，也可以加入 Y 軸趨勢判斷
            if (foundSurface && m_Mario->GetJumpTimer() > 17.5f) {
                // 偵錯日誌：可以觀察落地時的 Y 座標落差
                // LOG_DEBUG("Jump Landing Attempt: footY={}, targetFootY={}", footY, targetFootY);
                m_Mario->Land(targetFootY);
            }
            else if (m_Mario->GetJumpTimer() >= 35.0f) {
                m_Mario->Fall();
            }
        }
        else {
            TileType tileBelow = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - 33.0f);
            TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) + 3.0f);
            TileType tileFoot2 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - 1.0f);

            // [落地檢查]
            // 如果在墜落狀態中偵測到表面，則恢復為靜止狀態。
            if (foundSurface && m_Mario->GetState() == MarioState::FALLING) {
                m_Mario->IDLE();
            }

            MarioState state = m_Mario->GetState();
            bool isClimbing = (state == MarioState::CLIMBING || state == MarioState::CLIMB_IDLE);

            // [物理吸附與墜落轉換]
            // 只有在非跳躍、非攀爬的狀態下才進行地面貼合修正
            if (state != MarioState::JUMPING && !isClimbing) {
                if (foundSurface) {
                    // 將 Mario 的位置修正到地板表面高度
                    m_Mario->SetPosition({marioPos.x, targetFootY + (marioSize.y / 2.0f)});
                    marioPos = m_Mario->GetPosition();
                }
                else {
                    // 如果下方完全沒有地板，則進入墜落狀態
                    m_Mario->Fall();
                    state = m_Mario->GetState();
                }
            }


            // 向上攀爬
            if (Util::Input::IsKeyPressed(Util::Keycode::UP)) {
                if ((tileFoot1/*tileAtCenter*/ == TileType::LADDER ||
                    (tileBelow == TileType::LADDER && !foundSurface))
                 && m_Mario->GetState() != MarioState::HAMMERING) {
                    m_Mario->Climb(CLIMB_DIR::UP);

                    glm::vec2 pos = m_Mario->GetPosition();
                    if (pos.y >= (halfHeight - 50.0f)) {
                        m_Mario->Win();
                    }
                }
            }
            // 向下攀爬
            else if (Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
                if ((tileFoot2 == TileType::LADDER || tileBelow == TileType::LADDER)
                 &&  m_Mario->GetState() != MarioState::HAMMERING) {
                    m_Mario->Climb(CLIMB_DIR::DOWN);
                }
            }
            // 放開上下鍵時，停止攀爬動畫
            else if (state == MarioState::CLIMBING &&
                    (Util::Input::IsKeyUp(Util::Keycode::UP)
                     || Util::Input::IsKeyUp(Util::Keycode::DOWN))) {
                m_Mario->ClimbIdle();
            }

            // 水平移動處理：只有在非攀爬（或在梯子底部/頂端）且非墜落狀態下才允許
            else if ((!isClimbing || foundSurface) && state != MarioState::FALLING) {
                if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
                m_Mario->Walk(MarioDIR::LEFT);
            }
                else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
                m_Mario->Walk(MarioDIR::RIGHT);
            }
                // 若目前正在走路但沒有按住方向鍵，且放開了按鍵，則進入 IDLE
                else if (state == MarioState::WALKING) {
                    if (Util::Input::IsKeyUp(Util::Keycode::LEFT) ||
                        Util::Input::IsKeyUp(Util::Keycode::RIGHT)) {
                        m_Mario->IDLE();
                    }
                }
            }
        }
#if 1 //sdbg
        // 4. 更新火球移動邏輯 (如果火球可見)
        if (m_Fireball->GetVisibility()) {
            m_Fireball->Update();
        }
#endif
        // 5. 碰撞偵測：Mario 與火球
        if (m_Fireball->GetVisibility()) {
            glm::vec2 marioSize = m_Mario->GetSize();
            if (m_Mario->GetState() == MarioState::HAMMERING) marioSize *= 1.8f;

           if (m_Fireball->IfCollides(m_Mario->GetPosition(), marioSize)) {
                if (m_Mario->GetState() == MarioState::HAMMERING) {
                    m_HUDText->AddScore(800);
                    m_Fireball->SetVisible(false); // 擊碎火球
                } else {
                    m_Mario->Dead();
                }
            }
        }

        // 6. 道具偵測：撿起槌子
        if (m_HammerItem && m_HammerItem->GetVisibility() && m_Mario->GetState() == MarioState::JUMPING) {
            if (m_HammerItem->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                m_Mario->WaitForHammer();
                m_HammerItem->SetVisible(false);
            }
        }
        if (m_HammerItem2 && m_HammerItem2->GetVisibility() && m_Mario->GetState() == MarioState::JUMPING) {
            if (m_HammerItem2->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                m_Mario->WaitForHammer();
                m_HammerItem2->SetVisible(false);
            }
        }

        // 7. 更新 HUD (包含 Bonus 時間倒數)
        m_HUDText->Update(Util::Time::GetDeltaTimeMs());
    }

    // 即使遊戲結束，Mario 的動畫更新 (例如 Win 動畫) 與 Renderer 仍需持續運行
    m_Mario->Update();
    m_Renderer.Update();


    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
