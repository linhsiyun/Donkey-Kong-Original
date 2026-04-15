#include "App.hpp"
#include "Map.hpp"
//#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Time.hpp"
#include "Util/Logger.hpp"
#include "config.hpp"

// 地面上的槌子道具物件
static std::shared_ptr<Character> m_HammerItem;

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

    // 初始化 Mario 物件，並設定初始位置
    m_Mario = std::make_shared<Mario>();
    m_Mario->SetPosition({-halfWidth + 100.0f, -halfHeight + 100.0f});

    // 把 Mario 裡面所有的圖層一口氣加進 App 的Renderer中
    m_Mario->AddToRenderer(m_Renderer);

    // 初始化火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Fireball->SetPosition({0.0f, -70.0f}); // 設定火球初始位置
    m_Renderer.AddChild(m_Fireball);

    // 初始化地面上的槌子道具並放在右側
    m_HammerItem = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem->SetPosition({150.0f, -50.5f});
    m_HammerItem->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem);

    // 初始化text物件
    m_HUDText = std::make_shared<HUDManager>();
    m_HUDText->Init();
    m_HUDText->AddToRenderer(m_Renderer);

    // 初始化 DonkeyKong 物件並設定動畫圖片
    std::vector<std::string> dkImages = {
        RESOURCE_DIR"/Images/DK1.png",
        RESOURCE_DIR"/Images/DK2.png",
        RESOURCE_DIR"/Images/DK3.png",
        RESOURCE_DIR"/Images/DK4.png",
        RESOURCE_DIR"/Images/DK5.png"
    };
    m_DonkeyKong = std::make_shared<DonkeyKong>(dkImages);
    m_DonkeyKong->SetPosition({-halfWidth + 100.0f, halfHeight - 150.0f}); // 設定 Donkey Kong 到畫面上方
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

    // 設定 App 物件初始狀態為 UPDATE，開始遊戲主迴圈
    m_CurrentState = State::UPDATE;
    LOG_TRACE("UPDATE");
}

#if 1 //TODO
// 這是我們將原本寫在 App::Start 裡面的 Lambda ({...}) 抽出來的一般成員函式
// 這樣寫可以讓程式碼比較好讀，不會讓 App::Start 太肥大，同時如果有其他地方需要產酒桶也可以重複呼叫。
void App::SpawnBarrel() {
    LOG_DEBUG("++barrel");

    // 準備酒桶的動畫素材
    std::vector<std::string> barrelImages = {
        RESOURCE_DIR"/Images/Barrel1.png",
        RESOURCE_DIR"/Images/Barrel2.png",
        RESOURCE_DIR"/Images/Barrel3.png",
        RESOURCE_DIR"/Images/Barrel4.png",
        RESOURCE_DIR"/Images/Barrel5.png",
        RESOURCE_DIR"/Images/Barrel6.png"
    };

    // 產生一個新的酒桶
    auto newBarrel = std::make_shared<Barrel>(barrelImages);

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
#endif
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
    // 假設按下 Enter 鍵切換到第二關
    if (Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
        m_Map->LoadNewMap("../Resources/Images/dk.png", "level2.txt");
    }
    // 告訴渲染器，把所有 AddChild 進來的物件畫到畫面上 (包含你的地圖)
    if (isPlaying) {

        // 2. 更新 DonkeyKong (若停止更新，其產酒桶的回呼就不會觸發)
        if (m_DonkeyKong) {
            m_DonkeyKong->Update();
        }
#if 1 // TODO
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
                    continue;                       // 跳過後續處理，直接檢查下一個酒桶
                } else {
                    m_Mario->Dead();                // Mario 死亡
                }
            }

            // 取得酒桶座標
            glm::vec2 pos = barrel->GetPosition();

            // 判定是否超出邊界（增加 50.0f 的緩衝，讓酒桶完全消失在畫面外再刪除）
            if (std::abs(pos.x) > halfWidth + 50.0f || std::abs(pos.y) > halfHeight + 50.0f) {
                LOG_DEBUG("--barrel");
                m_Renderer.RemoveChild(barrel); // 1. 從渲染器移除
                it = m_Barrels.erase(it);       // 2. 從 vector 移除，並取得指向下一個元素的迭代器
            } else {
                ++it; // 沒被刪除時才手動增加迭代器
            }
        }
#endif
        // 3. 處理 Mario 輸入
        if ((m_Mario->GetState() != MarioState::JUMPING)
            && (m_Mario->GetState() != MarioState::CLIMBING)
            && (m_Mario->GetState() != MarioState::HAMMERING) // 拿槌子時禁止跳躍
            && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
            m_Mario->JumpStart();
        }

        // 2. 根據狀態執行邏輯
        if (m_Mario->IsJumping()) {
            m_Mario->Jump();
        }
        // 3. 一般移動邏輯
        else {
            // 向上攀爬
            if (Util::Input::IsKeyPressed(Util::Keycode::UP)) {
                // TODO: 檢查梯子
                //if (m_Mario->GetState() != MarioState::HAMMERING )
                {
                    m_Mario->Climb(CLIMB_DIR::UP);

                    glm::vec2 pos = m_Mario->GetPosition();
                    if (pos.y >= (halfHeight - 100.0f)) {
                        m_Mario->Win();
                    }
                }
            }
            // 向下攀爬
            else if (Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
                // TODO: 檢查梯子
                //if (m_Mario->GetState() != MarioState::HAMMERING )
                {
                    m_Mario->Climb(CLIMB_DIR::DOWN);
                }
            }
            // 放開上下鍵時，停止攀爬動畫
            else if (m_Mario->GetState() == MarioState::CLIMBING &&
                    (Util::Input::IsKeyUp(Util::Keycode::UP)
                     || Util::Input::IsKeyUp(Util::Keycode::DOWN))) {
                m_Mario->ClimbIdle();
            }

            // 向左移動
            else if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
                m_Mario->Walk(MarioDIR::LEFT);
            }
            // 向右移動
            else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
                m_Mario->Walk(MarioDIR::RIGHT);
            }
            // 放開左右鍵時，重置回靜止狀態
            else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) ||
                     Util::Input::IsKeyUp(Util::Keycode::RIGHT)) {
                m_Mario->IDLE();
            }
        }

        // 4. 更新火球移動邏輯
        if (m_Fireball->GetVisibility()) {
            m_Fireball->Update();
        }

        // 5. 碰撞偵測：Mario 與火球
        if (m_Fireball->GetVisibility()) {
           if (m_Fireball->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                if (m_Mario->GetState() == MarioState::HAMMERING) {
                    LOG_DEBUG("FIREBALL DESTROYED BY HAMMER!");
                    m_Fireball->SetVisible(false);
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
