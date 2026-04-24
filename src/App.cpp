#include "App.hpp"
#include "Map.hpp"
//#include "Util/Image.hpp"
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

void App::Start() {
    LOG_TRACE("Start");

    // 初始化地圖，並加入到 Renderer 渲染清單中
    m_Map = std::make_shared<Map>("../Resources/Images/board-barrels.png", "../Resources/Maps/Map1.txt");
    m_Renderer.AddChild(m_Map);

    //取得地圖的實際縮放後大小
    glm::vec2 mapSize = m_Map->GetScaledSize();
    halfWidth = mapSize.x / 2.0f;
    halfHeight = mapSize.y / 2.0f;
    LOG_INFO("Map halfWidth: {}, Map halfHeight: {}", halfWidth, halfHeight);

    // 初始化 Mario 物件
    m_Mario = std::make_shared<Mario>();
    // 將 Mario 精準放在地圖絕對座標 (100, 100) 的位置
    glm::vec2 marioStartPos = m_Map->GetWorldPosition(100.0f, 630.0f);
    m_Mario->SetPosition(marioStartPos);

    // 把 Mario 裡面所有的圖層一口氣加進 App 的Renderer中
    m_Mario->AddToRenderer(m_Renderer);

    // 初始化火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Fireball->SetPosition({-100.0f, -70.0f}); // 設定火球初始位置
    m_Renderer.AddChild(m_Fireball);

    // 初始化地面上的槌子道具並放在右側
    m_HammerItem = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem->SetPosition({150.0f, -120.5f});
    m_HammerItem->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem);

    // 初始化第二個槌子道具，放在靠近酒桶滾動的路徑上 (測試用)
    m_HammerItem2 = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem2->SetPosition({-halfWidth + 180.0f, halfHeight - 180.0f});
    m_HammerItem2->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem2);

    // 初始化text物件
    m_HUDText = std::make_shared<HUDManager>();
    m_HUDText->Init();
    m_HUDText->AddToRenderer(m_Renderer);

    // 初始化 DonkeyKong 物件
    m_DonkeyKong = std::make_shared<DonkeyKong>();
    m_DonkeyKong->SetPosition({-halfWidth + 180.0f, halfHeight - 100.0f}); // 回調位置，釋放遊戲空間
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

    // 將 Donkey Kong 的邊界資訊傳遞給 Mario
    m_Mario->SetDonkeyKongBounds(m_DonkeyKong->GetPosition(), m_DonkeyKong->GetSize());

    // 設定 App 物件初始狀態為 UPDATE，開始遊戲主迴圈
    m_CurrentState = State::UPDATE;
    LOG_TRACE("UPDATE");
}

// 這是我們將原本寫在 App::Start 裡面的 Lambda ({...}) 抽出來的一般成員函式
// 這樣寫可以讓程式碼比較好讀，不會讓 App::Start 太肥大，同時如果有其他地方需要產酒桶也可以重複呼叫。
void App::SpawnBarrel() {
    LOG_DEBUG("++barrel");

    // 產生一個新的酒桶
    auto newBarrel = std::make_shared<Barrel>();

    // 將酒桶的初始位置設定在 DK 旁邊 (可以做微調)
    glm::vec2 dkPos = m_DonkeyKong->GetPosition();
    newBarrel->SetPosition({dkPos.x + 40.0f, dkPos.y - 20.0f});

    // 可選：設定初始速度、層級
    newBarrel->SetZIndex(40);
    newBarrel->SetScale({m_Mario->marioScale/1.5f, m_Mario->marioScale/1.5f});
    newBarrel->SetDirection(Barrel::Direction::RIGHT);

    // 將酒桶暫存在清單維護並加入畫面繪製的根節點
    m_Barrels.push_back(newBarrel);
    m_Renderer.AddChild(newBarrel);
}

void App::Update() {

    // 1. 取得當前狀態，判定是否處於「可遊玩」狀態
    MarioState marioState = m_Mario->GetState();
    bool isPlaying = (marioState != MarioState::DEAD && marioState != MarioState::WIN);

    /*測試用
    float speed = 5.0f;
    auto transform = m_TestMarker->GetTransform();
    if (Util::Input::IsKeyPressed(Util::Keycode::W)) { transform.translation.y += speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) { transform.translation.y -= speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) { transform.translation.x -= speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) { transform.translation.x += speed; }
    m_TestMarker->m_Transform = transform;
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        float x = transform.translation.x;
        float y = transform.translation.y;
        TileType tile = m_Map->GetTileAtPosition(x, y);
        LOG_INFO("游標座標: ({}, {}), 對應 TXT 代號: {}", x, y, static_cast<int>(tile));
    }
    */
    // 假設按下 Enter 鍵切換到第二張地圖測試
    if (Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
        m_Map->LoadNewMap("../Resources/Images/board-conveyors.png", "../Resources/Maps/Map2.txt");
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
            if (barrel->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
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

            // 取得酒桶腳下的位置 (根據引擎設定，可能需要根據實際原點調整 Y 軸偏移，這裡先抓底部中心再略微往下探測)
            float footY = pos.y - (size.y / 2.0f) - 5.0f;

            // 使用地圖系統查詢酒桶底部踩著什麼格子
            TileType footTile = m_Map->GetTileAtPosition(pos.x, footY);

            // 狀態機邏輯：依照酒桶當前狀態與腳下地形做不同反應
            if (barrel->GetState() == Barrel::State::ROLLING) {
                if (footTile == TileType::EMPTY) {
                    // [狀況 1] 碰到邊緣 (腳底懸空): 切換至從邊緣掉落狀態
                    barrel->SetState(Barrel::State::FALLING_EDGE);
                    LOG_DEBUG("Barrel edge fall at X: {}", pos.x);
                }
#if 0  //sdbg
                else if (footTile == TileType::LADDER) {
                    // [狀況 2] 碰到梯子: 設定一個機率讓它往下掉落 (例: 每次經過梯子中心區域時有 25% 產生掉落)
                    // 為了避免同一根梯子在多個 frame 內連續骰機率，可以加上一個防抖 (例如檢查與梯子中心 X 的對齊程度，此處示範透過亂數快速過濾)
                    // (註: rand() % 100 只有在完全進入梯子的瞬間觸發比較保險，這裡示範最基礎的做法)
                    if (rand() % 100 < 25) {
                        barrel->SetState(Barrel::State::FALLING_LADDER);
                        LOG_DEBUG("Barrel ladder fall at X: {}", pos.x);
                    }
                }
#endif
            }
            else if (barrel->GetState() == Barrel::State::FALLING_EDGE ||
                     barrel->GetState() == Barrel::State::FALLING_LADDER) {
                // [狀況 3] 如果正在掉落，且腳底踩到結實的平地 (FLOOR):
                // 則終止落下，且恢復滾動狀態。
                if (footTile == TileType::FLOOR) {
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

        // 3. 處理 Mario 輸入
        if ((m_Mario->GetState() != MarioState::JUMPING)
            && (m_Mario->GetState() != MarioState::CLIMBING)
            && (m_Mario->GetState() != MarioState::HAMMERING) // 拿槌子時禁止跳躍
            && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
            m_Mario->JumpStart();
        }

        // 2. 根據狀態執行邏輯
        if (m_Mario->IsJumping()) {
            m_Mario->JumpStart();
        }
        // 3. 一般移動邏輯
        else {
            glm::vec2 marioPos = m_Mario->GetPosition();
            // 取得 Mario 的尺寸，用於計算腳底位置
            glm::vec2 marioSize = m_Mario->GetSize();
            MarioState state = m_Mario->GetState();

            // 偵測中心點 (用於判斷是否正在梯子上)
            //TileType tileAtCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y);
            // 偵測腳底下方 (向下偏移約 5~10 像素，確保能穿過地板偵測到下方的梯子)
            TileType tileBelow = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - 33.0f); // tile height=30
            TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) + 1.0f);
            TileType tileFoot2 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - 1.0f);
            //LOG_DEBUG("{}", (int)tileBelow );

            // --- 地面偵測與 Y 座標修正 (Snapping) ---
            // 檢測腳底中心點下方是否有地板 (放寬偵測範圍至 8 像素以增加穩定性)
            float footY = marioPos.y - (marioSize.y / 2.0f);
            bool onFloor = (m_Map->GetTileAtPosition(marioPos.x, footY - 4.0f) == TileType::FLOOR);
#if 1 //sdbg
            if (onFloor) LOG_DEBUG("onFloor");
#endif
            // 如果在地面上且處於走路或靜止狀態，修正 Y 座標以貼合地面
            if (onFloor && state != MarioState::JUMPING && state != MarioState::CLIMBING) {
                // 假設地圖每個格子高度為 8.0 像素 (Donkey Kong 經典規格)
                // 將腳底座標四捨五入到最接近的 8 像素格點，實現「吸附」效果
                //float snappedFootY = std::round(footY / 8.0f) * 8.0f;
                float snappedFootY = std::round(footY / 15.0f) * 15.0f;
                m_Mario->SetPosition({marioPos.x, snappedFootY + (marioSize.y / 2.0f)});

                // 更新局部變數，確保下方的 Walk 邏輯使用的是修正後的位置
                marioPos = m_Mario->GetPosition();
            }

            bool isClimbing = (state == MarioState::CLIMBING || state == MarioState::CLIMB_IDLE);

            // 向上攀爬
            if (Util::Input::IsKeyPressed(Util::Keycode::UP)) {
                if ((tileFoot1 == TileType::LADDER || tileBelow == TileType::LADDER)
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
                // 如果是在平地上，先用透視眼確認下面有梯子，才能開始爬！
                if (m_Mario->GetState() == MarioState::IDLE || m_Mario->GetState() == MarioState::WALKING) {
                    if (m_Mario->CanClimbDown(m_Map)) {
                        m_Mario->Climb(CLIMB_DIR::DOWN);
                    }
                }
                // 如果已經在梯子上了，就直接繼續往下爬
                else if (m_Mario->GetState() == MarioState::CLIMBING || m_Mario->GetState() == MarioState::CLIMB_IDLE) {
                    m_Mario->Climb(CLIMB_DIR::DOWN);
                }
            }
            // 放開上下鍵時，停止攀爬動畫
            else if (state == MarioState::CLIMBING &&
                    (Util::Input::IsKeyUp(Util::Keycode::UP)
                     || Util::Input::IsKeyUp(Util::Keycode::DOWN))) {
                m_Mario->ClimbIdle();
            }

            // 向左移動 (非攀爬狀態，或是雖然在攀爬狀態但腳下已有地板可以離開)
            else if (Util::Input::IsKeyPressed(Util::Keycode::LEFT) &&
                     (!isClimbing)) {
                m_Mario->Walk(MarioDIR::LEFT);
            }
            // 向右移動 (非攀爬狀態，或是雖然在攀爬狀態但腳下已有地板可以離開)
            else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT) &&
                     (!isClimbing)) {
                m_Mario->Walk(MarioDIR::RIGHT);
            }
            // 放開左右鍵時，重置回靜止狀態
            else if ((Util::Input::IsKeyUp(Util::Keycode::LEFT) ||
                      Util::Input::IsKeyUp(Util::Keycode::RIGHT)) &&
                     (!isClimbing)) {
                m_Mario->IDLE();
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
           if (m_Fireball->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                if (m_Mario->GetState() == MarioState::HAMMERING) {
                    LOG_DEBUG("FIREBALL DESTROYED BY HAMMER!");
                    m_Fireball->SetVisible(false);
                m_HUDText->AddScore(800);
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
    // 取得目前的 DeltaTime (時間差)
    float dt = Util::Time::GetDeltaTimeMs();

    // 將時間與地圖指標傳給 Mario 進行物理與動畫更新
    m_Mario->Update(dt, m_Map);
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
