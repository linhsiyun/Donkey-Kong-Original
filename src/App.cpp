#include "App.hpp"
#include "Map.hpp"
//#include "Util/Image.hpp"
#include "Mario.hpp"
#include "TileType.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Time.hpp"
#include "Util/Logger.hpp"
#include "Util/Animation.hpp"
#include "config.hpp"
#include "Setting.hpp"
#include <cstdlib> // 加入 rand()
#include "ConveyorSystem.hpp"
#include "CementSpawner.hpp"

// 地面上的槌子道具物件
static std::shared_ptr<Character> m_HammerItem;
static std::shared_ptr<Character> m_HammerItem2;
static std::shared_ptr<Character> m_StaticBarrels;

// 傳送帶關卡物件
static std::shared_ptr<ConveyorSystem> m_ConveyorSystem;
static std::vector<std::shared_ptr<CementSpawner>> m_CementSpawners;

/**
 * @brief 生成並初始化一個新的酒桶物件。
 *
 * 此函式通常由 Donkey Kong 的動畫回呼觸發。執行流程包含：
 * 1. 實例化 Barrel 物件並設定初始縮放與 Z 軸層級。
 * 2. 計算生成座標，確保酒桶在大金剛右側腳邊出現，且底部與地表齊平。
 * 3. 將新酒桶加入 `m_Barrels` 容器以供邏輯更新，並註冊至 `m_Renderer` 進行繪製。
 */
void App::SpawnBarrel() {
    LOG_DEBUG("++barrel");

    // 建構 (construct) 一個新的酒桶
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

/**
 * @brief 更新所有酒桶的邏輯更新器。
 *
 * 此函式負責處理場景中所有酒桶的生命週期，包含：
 * 1. 執行位移與動畫更新。
 * 2. 處理與 Mario 的碰撞偵測（包含搥擊判定）。
 * 3. 執行地表物理偵測，確保酒桶貼合斜坡或進入墜落狀態。
 * 4. 處理酒桶的狀態機轉換與邊界清理。
 * @param marioState 傳入 Mario 當前的狀態，用以調整碰撞判定範圍與結果。
 */
void App::UpdateBarrels(MarioState marioState) {
    // 使用迭代器遍歷酒桶清單，以便在迴圈中安全地刪除物件
    for (auto it = m_Barrels.begin(); it != m_Barrels.end(); ) {
        auto& barrel = *it;
        barrel->Update(); // 執行酒桶內部的位移與動畫幀更新

        // --- 1. 碰撞偵測：Mario 與酒桶 ---
        glm::vec2 marioSize = m_Mario->GetSize();
        // 如果 Mario 正在揮舞槌子，擴大他的有效判定範圍
        if (marioState == MarioState::HAMMERING) marioSize *= 1.8f;

        if (barrel->IfCollides(m_Mario->GetPosition(), marioSize)) {
            if (marioState == MarioState::HAMMERING) {
                // 搥擊成功：觸發特效、加分，並移除酒桶
                TriggerSmash(barrel->GetPosition(), 500);
                m_Renderer.RemoveChild(barrel);
                it = m_Barrels.erase(it); // erase 會回傳下一個有效的迭代器
                continue; // 跳過後續邏輯，直接處理下一個酒桶
            } else {
                // 碰撞失敗：Mario 死亡
                m_Mario->Dead();
            }
        }

        // --- 2. 地表偵測與物理吸附邏輯 ---
        glm::vec2 pos = barrel->GetPosition();
        glm::vec2 size = barrel->GetSize();

        float barrelFootY = pos.y - (size.y / 2.0f); // 取得酒桶底部 Y 座標
        float targetFootY = barrelFootY;
        bool foundSurface = false;
        const float tileH = m_Map->GetTileHeight();
        const float searchRange = tileH * 1.5f; // 搜尋範圍設為 1.5 格，足以涵蓋斜坡落差

        // [地表掃描]：判斷酒桶目前是站在地板上，還是懸空
        // 採用你發現的邏輯：往下深探 1 像素，避免邊界判定誤差
        TileType currentBarrelTile = m_Map->GetTileAtPosition(pos.x, barrelFootY - 1.0f);

        if (currentBarrelTile == TileType::FLOOR || currentBarrelTile == TileType::RIVET) {
            // 向上修正：如果酒桶稍微陷入地板，將其抬起至表面
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tile = m_Map->GetTileAtPosition(pos.x, barrelFootY + dy + 1.0f);
                if (tile == TileType::EMPTY || tile == TileType::LADDER) {
                    targetFootY = barrelFootY + dy;
                    foundSurface = true;
                    break;
                }
            }
        } else {
            // 向下修正：處理下坡情況，讓酒桶「貼著」斜坡向下滾動
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tileBelow = m_Map->GetTileAtPosition(pos.x, barrelFootY - dy);
                if (tileBelow == TileType::FLOOR || tileBelow == TileType::RIVET) {
                    targetFootY = barrelFootY - dy + 1.0f; // 修正到地板上方
                    foundSurface = true;
                    break;
                }
            }
        }

        // 偵測酒桶正下方是否有梯子 (稍微往下探測 2 像素)
        TileType footTile = m_Map->GetTileAtPosition(pos.x, targetFootY - 2.0f);

        // --- 3. 狀態機切換邏輯 ---
        if (barrel->GetState() == Barrel::State::ROLLING) {
            if (!foundSurface) {
                // 狀況 A：前方沒路了，進入邊緣墜落狀態
                barrel->SetState(Barrel::State::FALLING_EDGE);
            } else {
                // 狀況 B：正常滾動，套用物理吸附修正座標
                barrel->SetPosition({pos.x, targetFootY + (size.y / 2.0f)});
                pos = barrel->GetPosition();
                // 如果經過梯子，有機率 (25%) 決定往下掉
                if (footTile == TileType::LADDER && rand() % 100 < 25) {
                    barrel->SetState(Barrel::State::FALLING_LADDER);
                }
            }
        }
        else if (barrel->GetState() == Barrel::State::FALLING_EDGE ||
                 barrel->GetState() == Barrel::State::FALLING_LADDER) {
            if (foundSurface) {
                // 狀況 C：掉落中碰到地板，恢復滾動狀態並反轉方向
                barrel->SetState(Barrel::State::ROLLING);
                barrel->SetDirection(barrel->GetDirection() == Barrel::Direction::RIGHT ?
                                     Barrel::Direction::LEFT : Barrel::Direction::RIGHT);
            }
        }

        // --- 4. 邊界檢查：超出畫面則銷毀 ---
        if (std::abs(pos.x) > halfWidth + 50.0f || std::abs(pos.y) > halfHeight + 50.0f) {
            m_Renderer.RemoveChild(barrel);
            it = m_Barrels.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief 更新水泥塊邏輯更新器。
 */
void App::UpdateCementPans() {
    for (auto it = m_CementPans.begin(); it != m_CementPans.end(); ) {
        auto& pan = *it;
        pan->Update(m_Map, m_ConveyorSystem); // 執行物理與位移

        // 碰撞偵測 (Mario)
        if (pan->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
            m_Mario->Dead();
        }

        // 邊界檢查：掉出畫面外則銷毀
        glm::vec2 pos = pan->GetPosition();
        // 允許水泥塊移動到 halfWidth 之外一段距離（如 50px），確保圖片完全消失後再移除
        if (pan->ShouldRemove() || std::abs(pos.x) > halfWidth + 50.0f) {
            m_Renderer.RemoveChild(pan);
            it = m_CementPans.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief 觸發搥擊特效與加分邏輯。
 *
 * 當 Mario 使用槌子擊碎障礙物（如木桶或火球）時呼叫。
 * 此函式會更新分數、在指定位置播放爆炸特效動畫，並暫時凍結遊戲邏輯以增強擊打感（Hitstop）。
 *
 * @param position 特效產生的世界座標
 * @param score 該次擊碎獲得的分數獎勵
 */
void App::TriggerSmash(glm::vec2 position, int score) {
    m_HUDText->AddScore(score);
    m_SmashEffect->SetPosition(position);
    m_SmashEffect->SetVisible(true);
    m_SmashEffect->Stop(); // 重置到第一幀
    m_SmashEffect->Play();
    m_FreezeTimer = 1500.0f; // 調整為凍結 1.5 秒
    LOG_DEBUG("SMASH TRIGGERED at ({}, {})", position.x, position.y);
}

/**
 * @brief 載入並初始化指定關卡。
 *
 * 執行流程包含：
 * 1. 全域清理：移除上一關殘留的電梯、酒桶、插銷與視覺特效。
 * 2. 狀態重置：還原 Mario、Donkey Kong 與 HUD 的初始狀態。
 * 3. 資源載入：根據 Stage 編號切換地圖圖片與邏輯陣列。
 * 4. 配置佈局：設定角色初始座標、電梯生成路徑與插銷位置。
 * @param level 當前的關卡進度總數。
 */
void App::LoadLevel(int level) {

    m_CurrentLevel = level;
    m_CurrentStage = level % 4;
    if (m_CurrentStage == 0)
        m_CurrentStage = 4;

    // 每次載入關卡前先清除舊酒桶(Stage 1)，確保場景完全重置
    for (auto& barrel : m_Barrels) m_Renderer.RemoveChild(barrel);
    m_Barrels.clear();

    // 只有在第一關時顯示固定木桶堆，其餘關卡隱藏。將此邏輯移至開頭以統一管理物件狀態。
    if (m_StaticBarrels) m_StaticBarrels->SetVisible(m_CurrentStage == 1);

    // 每次載入關卡前先清除舊電梯(Stage 3)，確保畫面上不會殘留電梯踏板
    for (auto& el : m_Elevators) m_Renderer.RemoveChild(el);
    m_Elevators.clear();

    // 每次載入關卡前先清除舊插銷(Stage 4)，確保畫面上不會殘留
    for (auto& pair : m_RivetVisuals) m_Renderer.RemoveChild(pair.second);
    m_RivetVisuals.clear();

    // 清除舊水泥塊
    for (auto& pan : m_CementPans) m_Renderer.RemoveChild(pan);
    m_CementPans.clear();

    // 清除舊傳送帶 Spawners
    for (auto& s : m_CementSpawners) m_Renderer.RemoveChild(s);
    m_CementSpawners.clear();

    // 重置通關特效與大金剛狀態 (確保從 Stage 4 勝利後切換或重玩時狀態正確)
    if (m_BlackCover) m_BlackCover->SetVisible(false);
    if (m_LeftMask) m_LeftMask->SetVisible(false);
    if (m_RightMask) m_RightMask->SetVisible(false);

    if (m_DonkeyKong) {
        // 還原 DK 的縮放比例（恢復正向並重設尺寸）
        m_DonkeyKong->SetScale({m_Mario->marioScale / 1.5f, m_Mario->marioScale / 1.5f});
    }

    // 重置 Stage 4 特有的邏輯標記 (移至開頭以確保不論從哪一關離開，狀態都乾淨)
    m_ActiveRivetPos = {0.0f, 0.0f};
    m_HasActiveRivet = false;
    m_RivetCount = 0;
    m_DKFallTimer = 0.0f;

    // 重置 Mario 以及其它遊戲角色的狀態與可見性 (移至開頭以統一管理)
    m_Mario->SetState(MarioState::IDLE);
#if 1 //sdbg
    m_Fireball->SetVisible(true);
#else
    m_Fireball->SetVisible(false);
#endif
    if (m_HammerItem) m_HammerItem->SetVisible(true);
    if (m_HammerItem2) m_HammerItem2->SetVisible(true);

    // 重置 HUD 資訊
    if (m_HUDText) {
        m_HUDText->Init();                  // 重置分數為 0 且 Bonus 為 5000
        m_HUDText->SetLevel(m_CurrentLevel); // 更新畫面上的 L=XX 文字
    }

    // 根據傳入的關卡編號，載入對應的地圖圖片與純文字檔 (MapX.txt)
    // 同時把所有物件 (Mario, 火球, 道具, 大金剛) 移動到該關卡適合的座標
    if (m_CurrentStage == 1) {
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
        if (m_StaticBarrels) {
            // 將木桶堆放在 Donkey Kong 左側的平台上
            m_StaticBarrels->SetPosition({-halfWidth + 35.0f, halfHeight - 110.0f});
        }

        m_DonkeyKong->SetPosition({-halfWidth + 120.0f, halfHeight - 100.0f});
        m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::STATIONARY_LOOKING);
        if (m_Princess) {
            m_Princess->SetPosition({0.0f, halfHeight - 35});
        }

    } else if (m_CurrentStage == 2) {
        m_Map->LoadNewMap("../Resources/Images/board-conveyors.png", "../Resources/Maps/Map2.txt");

        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;

        // 設定螢幕邊界，讓 Mario 的 Update 邏輯可以進行限制
        m_Mario->SetScreenBounds(halfWidth, halfHeight);

        // 設定左右遮罩，讓水泥塊能漸漸出現/消失
        float maskWidth = 100.0f; // 遮罩寬度，需大到足以蓋住生成點
        m_LeftMask->SetVisible(true);
        m_LeftMask->SetPosition({-halfWidth - (maskWidth / 2.0f), -100.0f});
        //m_LeftMask->SetScale({maskWidth / 10.0f, 100.0f}); // 假設 black.png 原始尺寸很小，將其拉長放大

        m_RightMask->SetVisible(true);
        m_RightMask->SetPosition({halfWidth + (maskWidth / 2.0f), -100.0f});
        //m_RightMask->SetScale({maskWidth / 10.0f, 100.0f});

        // 初始化傳送帶邏輯系統
        m_ConveyorSystem = std::make_shared<ConveyorSystem>();

        // 建立四個 Cement Spawners
        // 定義 y1, y2 高度 (需根據地圖實際像素調整，這裡先用示意值)
        float y1 = -160.0f;
        float y2 = 55.0f;

        auto s1 = std::make_shared<CementSpawner>(TileType::CONVEYOR1, CementSpawner::Side::LEFT);
        s1->SetPosition({-350.0f, y1});
        auto s2 = std::make_shared<CementSpawner>(TileType::CONVEYOR1, CementSpawner::Side::RIGHT);
        s2->SetPosition({350.0f, y1});
        auto s3 = std::make_shared<CementSpawner>(TileType::CONVEYOR2, CementSpawner::Side::LEFT);
        s3->SetPosition({-350.0f, y2});
        auto s4 = std::make_shared<CementSpawner>(TileType::CONVEYOR3, CementSpawner::Side::RIGHT);
        s4->SetPosition({350.0f, y2});

        m_CementSpawners = {s1, s2, s3, s4};
        for(auto& s : m_CementSpawners) m_Renderer.AddChild(s);

        // 第二關的各角色與道具初始位置 (目前暫時設定與第一關相同，之後你可以自由調整這組座標)
        m_Mario->SetPosition({-halfWidth + 50.0f, -halfHeight + 45.0f});
        m_Fireball->SetPosition({-100.0f, -70.0f});
        if (m_HammerItem) m_HammerItem->SetPosition({150.0f, -120.5f});
        if (m_HammerItem2) m_HammerItem2->SetPosition({-halfWidth + 180.0f, halfHeight - 180.0f});

        // 設定第二關 DK 會左右移動且只會搥胸
        m_DonkeyKong->SetPosition({-halfWidth + 110.0f, halfHeight - 110.0f});
        m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::MOVING_CHEST_BEATING);
        m_DonkeyKong->SetMoveBounds(-halfWidth + 110.0f, halfWidth - 110.0f);
        if (m_Princess) {
            m_Princess->SetPosition({0.0f, halfHeight - 35});
        }

    } else if (m_CurrentStage == 3) {
        m_Map->LoadNewMap("../Resources/Images/board-elevators.png", "../Resources/Maps/Map3.txt");
        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;
        m_Mario->SetScreenBounds(halfWidth, halfHeight);
        m_Mario->SetPosition({-halfWidth + 50.0f, -halfHeight + 45.0f});

        // 設定第三關 DK 只會搥胸
        m_DonkeyKong->SetPosition({-halfWidth + 120.0f, halfHeight - 110.0f});
        m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::MOVING_CHEST_BEATING);
        m_DonkeyKong->SetMoveBounds(-halfWidth + 120.0f, -halfWidth + 120.0f);
        if (m_Princess) {
            m_Princess->SetPosition({0.0f, halfHeight - 35});
        }

        // 取得地圖目前的縮放比例
        glm::vec2 mapScale = m_Map->GetScale();

        // 假設上下邊界是根據半高設定
        float elevatorTopY = 130.0f;
        float elevatorBotY = -halfHeight + 30.0f;
        float spacing = 150.0f * mapScale.y; // 垂直間距也要隨著縮放調整

        // 左側電梯 (上升)：建立多個踏板以填滿上下邊界，形成連續循環的移動效果
        for (float y = elevatorBotY; y <= elevatorTopY; y += spacing) {
            // 建立單個電梯踏板：設定方向與邊界，同步地圖縮放以維持比例，並加入追蹤容器與渲染器
            auto el = std::make_shared<Elevator>(Elevator::Direction::UP, elevatorBotY, elevatorTopY, 1.0f);  // 讓速度隨關卡等級增加: 1.0f + m_CurrentLevel * 0.1f
            el->SetScale(mapScale); // 同步地圖縮放比例
            el->SetPosition({-145.0f * mapScale.x, y}); // X 座標需乘以地圖縮放，確保在不同解析度下位置正確
            m_Elevators.push_back(el);
            m_Renderer.AddChild(el);
        }

        // 右側電梯 (下降)：同樣根據邊界與間距建立踏板，方向設定為向下
        for (float y = elevatorTopY; y >= elevatorBotY; y -= spacing) {
            auto el = std::make_shared<Elevator>(Elevator::Direction::DOWN, elevatorBotY, elevatorTopY, 1.0f);
            el->SetScale(mapScale); // 同步地圖縮放比例
            el->SetPosition({-15.0f * mapScale.x, y}); // X 座標需乘以地圖縮放
            m_Elevators.push_back(el);
            m_Renderer.AddChild(el);
        }
    } else if (m_CurrentStage == 4) {
        m_Map->LoadNewMap("../Resources/Images/board-rivets.png", "../Resources/Maps/Map4.txt");
        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;
        m_Mario->SetScreenBounds(halfWidth, halfHeight);

        // 第四關的各角色與道具初始位置
        m_Mario->SetPosition({-halfWidth + 50.0f, -halfHeight + 45.0f});
        m_Fireball->SetPosition({-100.0f, -70.0f});
        if (m_HammerItem) m_HammerItem->SetPosition({-280.0f, -110.0f});
        if (m_HammerItem2) m_HammerItem2->SetPosition({0.0f, halfHeight - 180.0f});

        // 設定第四關 DK 只會搥胸
        m_DonkeyKong->SetPosition({0.0f, halfHeight - 110.0f});
        m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::MOVING_CHEST_BEATING);
        m_DonkeyKong->SetMoveBounds(0.0f, 0.0f);
        if (m_Princess) {
            m_Princess->SetPosition({0.0f, halfHeight - 35});
        }

        const auto& data = m_Map->GetLevelData();
        for (int y = 0; y < data.GetHeight(); ++y) {
            for (int x = 0; x < data.GetWidth(); ++x) {
                if (data.GetTile(x, y) == TileType::RIVET) {
                    auto rivet = std::make_shared<Character>(RESOURCE_DIR"/Images/rivet.png");
                    // 根據 Tile 大小調整縮放，這裡假設使用與 Mario 類似的縮放倍率
                    rivet->SetScale({m_Mario->marioScale, m_Mario->marioScale});
                    rivet->SetZIndex(-5); // 放在地圖上方，角色下方
                    rivet->SetPosition(m_Map->GetTileWorldPosition(x, y+3));
                    LOG_DEBUG("({},{})", x, y);
                    m_RivetVisuals[{x, y}] = rivet;
                    m_Renderer.AddChild(rivet);
                    m_RivetCount++;
                }
            }
        }
    }
    LOG_INFO("map halfWidth: {}, halfHeight: {}", halfWidth, halfHeight);

    // 5. 同步物理資訊：更新 Donkey Kong 的新位置給 Mario 作為移動邊界
    m_Mario->SetDonkeyKongBounds(m_DonkeyKong->GetPosition(), m_DonkeyKong->GetSize());
}

/**
 * @brief 應用程式啟動初始化。
 *
 * 負責建立遊戲生命週期內共用的核心物件（Map, Mario, HUD, DonkeyKong），
 * 配置全域特效與道具，並在最後呼叫 LoadLevel 進入第一關。
 * 此函式僅在程式開啟時執行一次。
 */
void App::Start() {
    LOG_TRACE("Start");

    // 建構 (construct) 地圖，並加入到 Renderer 渲染清單中
    m_Map = std::make_shared<Map>("../Resources/Images/board-barrels.png", "../Resources/Maps/Map1.txt");
    m_Renderer.AddChild(m_Map);

    // 建構 Mario 物件，把 Mario 裡面所有的圖層一口氣加進 App 的Renderer中
    m_Mario = std::make_shared<Mario>();
    m_Mario->AddToRenderer(m_Renderer);

    // 建構火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Renderer.AddChild(m_Fireball);

    // 建構地面上的槌子道具並放在右側
    m_HammerItem = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem);

    // 建構第二個槌子道具，放在靠近酒桶滾動的路徑上 (測試用)
    m_HammerItem2 = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem2->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem2);

    // 建構 Stage 1 背景中的固定酒桶堆 (位於 Donkey Kong 旁邊)
    m_StaticBarrels = std::make_shared<Character>(RESOURCE_DIR"/Images/barrel00.png");
    m_StaticBarrels->SetScale({m_Mario->marioScale / 1.0f, m_Mario->marioScale / 1.0f});
    m_StaticBarrels->SetZIndex(40); // 確保在角色層級之後
    m_Renderer.AddChild(m_StaticBarrels);

    // 建構text物件
    m_HUDText = std::make_shared<HUDManager>();
    m_HUDText->Init();
    m_HUDText->AddToRenderer(m_Renderer);

    // 建構 DonkeyKong 物件
    m_DonkeyKong = std::make_shared<DonkeyKong>();
    m_DonkeyKong->SetZIndex(50); // 可選：調整圖層順序
    m_DonkeyKong->SetScale({m_Mario->marioScale/1.5f, m_Mario->marioScale/1.5f});
    m_Renderer.AddChild(m_DonkeyKong);

    // 設定產出木桶的回呼行為 (callback function)
    // [this] 捕捉 this 指標，代表在此 Lambda 裡面可以呼叫及使用 App 的成員函式與變數 (如 this->SpawnBarrel)
    m_DonkeyKong->SetBarrelSpawnCallback([this]() {
        this->SpawnBarrel();
    });

    // 初始化搥擊特效動畫 (1-2-3-1-2-3-pop)
    m_SmashEffect = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
        RESOURCE_DIR"/Images/bubble1.png", RESOURCE_DIR"/Images/bubble2.png",
        RESOURCE_DIR"/Images/bubble3.png", RESOURCE_DIR"/Images/bubble1.png",
        RESOURCE_DIR"/Images/bubble2.png", RESOURCE_DIR"/Images/bubble3.png",
        RESOURCE_DIR"/Images/bubble_pop.png"
    });
    m_SmashEffect->SetZIndex(70); // 高於角色
    m_SmashEffect->SetScale({3.0f, 3.0f});
    m_SmashEffect->SetVisible(false);
    m_SmashEffect->SetLooping(false);
    m_SmashEffect->SetInterval(150); // 加快速度，每幀 150ms (總長約 1050ms)
    m_Renderer.AddChild(m_SmashEffect);

    // 初始化黑色遮蓋方塊 (用於 Stage 4 通關)
    // 假設你有一個小小的黑色圖片 black.png，我們將其放大以遮住中間梯子區域
    m_BlackCover = std::make_shared<Character>(RESOURCE_DIR"/Images/black.png");
    m_BlackCover->SetZIndex(40); // 放在地圖之上，角色之下
    m_BlackCover->SetScale({2.0f, 2.0f}); // 放大以遮蓋中間結構
    m_BlackCover->SetVisible(false);
    m_Renderer.AddChild(m_BlackCover);

    // 初始化公主精靈 (Princess + Princess2/Princess3)
    m_Princess = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
        RESOURCE_DIR"/Images/Princess.png",
        RESOURCE_DIR"/Images/Princess2.png",
        RESOURCE_DIR"/Images/Princess3.png"
    });
    m_Princess->SetZIndex(60); // 放在大金剛下方但角色上方
    // 先以 Mario 的縮放當作基準，之後依據各幀尺寸調整縮放以保持視覺位置不動
    m_Princess->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Princess->SetVisible(true);
    m_Princess->Stop();
    m_Princess->SetLooping(false);

    m_Renderer.AddChild(m_Princess);
    m_Princess->Stop();
    // m_PrincessRefSize = m_Princess->GetSize();
    // if (m_PrincessRefSize.x > 0 && m_PrincessRefSize.y > 0) {
    //     m_Princess->SetPivot(m_PrincessRefSize / 2.0f);
    // }

    // 初始化左右遮罩 (使用與 BlackCover 相同的黑色圖片)
    m_LeftMask = std::make_shared<Character>(RESOURCE_DIR"/Images/black2.png");
    m_LeftMask->SetZIndex(60); // ZIndex 必須高於 CementPan (45) 與 Mario (50)
    m_LeftMask->SetVisible(false);
    m_Renderer.AddChild(m_LeftMask);

    m_RightMask = std::make_shared<Character>(RESOURCE_DIR"/Images/black2.png");
    m_RightMask->SetZIndex(60);
    m_RightMask->SetVisible(false);
    m_Renderer.AddChild(m_RightMask);

    // 載入當前關卡 (這會負責載入地圖、設定角色的初始位置與重置狀態，也處理 DonkeyKong 給 Mario 的邊界傳遞)
    LoadLevel(m_CurrentLevel);

    // 設定 App 物件初始狀態為 UPDATE，開始遊戲主迴圈
    m_CurrentState = State::UPDATE;
    LOG_TRACE("UPDATE");
}

/**
 * @brief 遊戲主迴圈。
 *
 * 每幀執行以下邏輯：
 * 1. 關卡切換測試與勝利過場動畫處理（如 Stage 4 的地基崩塌）。
 * 2. 遊戲暫停/凍結邏輯判斷（Hitstop 特效）。
 * 3. 物件邏輯更新：Donkey Kong 行為、電梯位移、木桶物理。
 * 4. Mario 核心物理：輸入處理、跳躍/攀爬判定、地表偵測、地圖互動（如拔插銷）。
 * 5. 碰撞偵測：火球與槌子道具判定。
 * 6. 渲染排序與介面更新。
 */
void App::Update() {

#if 1  //sdbg: 按下 N 鍵切換到下一關測試, 按下 R 鍵 reset
    if (Util::Input::IsKeyDown(Util::Keycode::N)) {
        m_CurrentLevel++;
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

        // --- Stage 4 特有的通關動畫：中間垮掉，DK 掉下去 ---
        if (m_CurrentStage == 4) {
            m_BlackCover->SetVisible(true);
            m_BlackCover->SetPosition({0.0f, -45.0f}); // 根據地圖位置調整，遮住中間結構

            // 讓 Donkey Kong 持續下墜
            glm::vec2 dkPos = m_DonkeyKong->GetPosition();
            if (dkPos.y > -halfHeight + 80.0f) { // 掉到螢幕底部
                dkPos.y -= 3.0f; // 下墜速度
                m_DonkeyKong->SetPosition(dkPos);

                // 每 200ms 反轉一次 Y 軸縮放，達到緩慢旋轉效果
                m_DKFallTimer += static_cast<float>(Util::Time::GetDeltaTimeMs());
                if (m_DKFallTimer >= 200.0f) {
                    m_DKFallTimer = 0.0f;
                    m_DonkeyKong->SetScale({m_DonkeyKong->GetScale().x, -m_DonkeyKong->GetScale().y});
                }

                m_DonkeyKong->Update();
            }
            //m_DonkeyKong->Update(); // 如果要 DK 掉落時仍有搥胸動作
        }

        // 等待玩家按下任意前進按鈕 (例如跳躍鍵 SPACE 或 RETURN 鍵)
        if (Util::Input::IsKeyDown(Util::Keycode::SPACE) || Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
            //if (m_CurrentLevel != 4)
            {
                m_CurrentLevel++;
                LoadLevel(m_CurrentLevel);
            }
        }
    }

    // 告訴渲染器，把所有 AddChild 進來的物件畫到畫面上 (包含你的地圖)
    if (isPlaying) {
        // 處理畫面凍結邏輯
        if (m_FreezeTimer > 0.0f) {
            m_FreezeTimer -= static_cast<float>(Util::Time::GetDeltaTimeMs());
            if (m_FreezeTimer <= 0.0f) {
                m_SmashEffect->SetVisible(false);
            }
            // 凍結期間只更新渲染器（讓特效動畫跑），不執行後續遊戲邏輯
            m_Renderer.Update();
            return;
        }

        // 更新傳送帶系統與 Spawner
        if (m_CurrentStage == 2 && m_ConveyorSystem) {
            float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
            m_ConveyorSystem->Update(dtMs);

            // 檢查 Spawners 是否要生成新的水泥塊
            for (auto& spawner : m_CementSpawners) {
                int dir = m_ConveyorSystem->GetDirection(spawner->GetTargetBelt());
                if (spawner->ShouldSpawn(dtMs, dir)) {
                    // 生成時傳入該 Spawner 對應的傳送帶類型
                    auto newPan = std::make_shared<CementPan>(spawner->GetTargetBelt());
                    newPan->SetPosition(spawner->GetPosition());
                    m_CementPans.push_back(newPan);
                    m_Renderer.AddChild(newPan);
                }
            }

            UpdateCementPans();
        }

#if 1 //sdbg
        // 2. 更新 DonkeyKong (若停止更新，其產酒桶的回呼就不會觸發)
        if (m_DonkeyKong) {
            m_DonkeyKong->Update();
        }
#endif

        // 更新公主計時與幀切換 (每 5 秒播放 Princess2/Princess3 來回切換兩次)
        if (m_Princess) {
            float dt = static_cast<float>(Util::Time::GetDeltaTimeMs());
            if (m_PrincessAnimating) {
                m_PrincessAnimTimerMs += dt;
                if (m_PrincessAnimTimerMs >= 250.0f) {
                    m_PrincessAnimTimerMs -= 250.0f;
                    m_PrincessToggleCount++;
                    // 切換 Princess2(索引1) / Princess3(索引2)
                    int frame = (m_PrincessToggleCount % 2 == 1) ? 2 : 1; // 第一個計時到會變成 Princess3
                    m_Princess->SetCurrentFrame(frame);
                    // 調整縮放使得不同尺寸幀看起來大小一致，不改變世界座標
                    if (m_PrincessRefSize.x > 0 && m_PrincessRefSize.y > 0) {
                        glm::vec2 newSize = m_Princess->GetSize();
                        glm::vec2 scaleFactor = {m_Mario->marioScale * (m_PrincessRefSize.x / newSize.x),
                                                m_Mario->marioScale * (m_PrincessRefSize.y / newSize.y)};
                        m_Princess->SetScale(scaleFactor);
                        m_Princess->SetPivot(m_PrincessRefSize / 2.0f);
                    }
                    // 若已切換四次 (總共顯示 Princess2/Princess3 兩來回)，結束動畫
                    if (m_PrincessToggleCount >= 4) {
                        m_PrincessAnimating = false;
                        m_PrincessToggleCount = 0;
                        m_Princess->SetCurrentFrame(0); // 回到 Princess 圖
                        if (m_PrincessRefSize.x > 0 && m_PrincessRefSize.y > 0) {
                            m_Princess->SetScale({m_Mario->marioScale, m_Mario->marioScale});
                            m_Princess->SetPivot(m_PrincessRefSize / 2.0f);
                        }
                        m_PrincessTimerMs = 5000.0f;    // 重置 5 秒計時
                    }
                }
            } else {
                m_PrincessTimerMs -= static_cast<float>(Util::Time::GetDeltaTimeMs());
                if (m_PrincessTimerMs <= 0.0f) {
                    // 立即顯示 Princess2 作為第一張，並開始內部計時
                    m_PrincessAnimating = true;
                    m_PrincessAnimTimerMs = 0.0f;
                    m_PrincessToggleCount = 0;
                    m_Princess->SetCurrentFrame(1);
                    if (m_PrincessRefSize.x > 0 && m_PrincessRefSize.y > 0) {
                        glm::vec2 newSize = m_Princess->GetSize();
                        glm::vec2 scaleFactor = {m_Mario->marioScale * (m_PrincessRefSize.x / newSize.x),
                                                m_Mario->marioScale * (m_PrincessRefSize.y / newSize.y)};
                        m_Princess->SetScale(scaleFactor);
                        m_Princess->SetPivot(m_PrincessRefSize / 2.0f);
                    }
                }
            }
        }

        // 更新電梯位移邏輯
        for (auto& elevator : m_Elevators) {
            elevator->Update();
        }

        // 更新木桶邏輯
        UpdateBarrels(marioState);

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
        glm::vec2 currentGroundTilePos = {0.0f, 0.0f}; // 【新增】儲存當前踩踏地板的中心座標
        const float tileH = m_Map->GetTileHeight();

        // 跳躍中搜尋範圍可以稍微加大，避免高速下落穿透
        const float searchRange = m_Mario->IsJumping() ? tileH * 2.0f : tileH * 1.5f;

        // 採用你發現的邏輯：往下深探 1 像素，確保穩定偵測到當前踩踏的地板
        TileType currentMarioTile = m_Map->GetTileAtPosition(marioPos.x, footY - 1.0f);

        // [修正] 地面偵測邏輯：加入對傳送帶 Tile 的判定
        if (currentMarioTile == TileType::FLOOR ||
            currentMarioTile == TileType::RIVET ||
            currentMarioTile == TileType::CONVEYOR1 ||
            currentMarioTile == TileType::CONVEYOR2 ||
            currentMarioTile == TileType::CONVEYOR3) {
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tile = m_Map->GetTileAtPosition(marioPos.x, footY + dy + 1.0f);
                if (tile == TileType::EMPTY || tile == TileType::LADDER) {

                    targetFootY = footY + dy;
                    foundSurface = true;
                    break;
                }
            }
        } else {
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tileBelow = m_Map->GetTileAtPosition(marioPos.x, footY - dy);
                if (tileBelow == TileType::FLOOR ||
                    tileBelow == TileType::RIVET ||
                    tileBelow == TileType::CONVEYOR1 ||
                    tileBelow == TileType::CONVEYOR2 ||
                    tileBelow == TileType::CONVEYOR3) {
                    targetFootY = footY - dy + 1.0f;
                    foundSurface = true;
                    currentMarioTile = tileBelow; // 重要：更新目前偵測到的 Tile 類型，以便後續處理傳送帶速度
                    break;
                }
            }
        }

        // --- [新增] 動態電梯平台碰撞偵測 ---
        // 如果在靜態地圖上沒找到表面，且正處於電梯關卡，則檢查 Mario 是否站在電梯上
        // 如果 Mario 踩在上升電梯上，我們必須主動把 el->GetSpeed() 對應的位移量加回 m_Mario->SetPosition。
        //     否則，Mario 會停在原地，而電梯會直接「穿過」他的身體往上升，導致他瞬間變回懸空狀態。
        // marioFootY >= elTopY - 10.0f 提供了一個緩衝區，確保 Mario 在下墜過程中只要接近電梯頂部，
        //     就能被正確「吸附」上去，這能提供更流暢的操作感。
        if (!foundSurface && m_CurrentStage == 3) {
            for (auto& el : m_Elevators) {
                // 使用 AABB 碰撞初步判斷 Mario 是否觸碰到電梯踏板
                if (el->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                    float marioFootY = marioPos.y - (marioSize.y / 2.0f);
                    float elTopY = el->GetPosition().y + (el->GetSize().y / 2.0f);

                    // 判定 Mario 必須在踏板上方（允許 10 像素的吸附落差）
                    if (marioFootY >= elTopY - 10.0f) {
                        foundSurface = true;
                        targetFootY = elTopY;

                        // 同步位移：Mario 必須加上電梯這一幀的移動距離，才不會從移動平台滑落
                        float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
                        float moveDist = el->GetSpeed() * dt * 60.0f;
                        if (el->GetDirection() == Elevator::Direction::UP) m_Mario->SetPosition(m_Mario->GetPosition() + glm::vec2(0, moveDist));
                        else m_Mario->SetPosition(m_Mario->GetPosition() - glm::vec2(0, moveDist));
                        break; // 只要站上一個電梯就不需檢查其他電梯
                    }
                }
            }
        }

        // 如果有找到地表，則記下該格子的網格索引
        if (foundSurface) {
            // 取得該格子在世界座標中的中心點 (x, y)
            auto [gx, gy] = m_Map->GetTileIndexAtPosition(marioPos.x, targetFootY - 1.0f);
            currentGroundTilePos = m_Map->GetTileWorldPosition(gx, gy);
        }

        // --- 處理 Rivet (Stage 4 特有邏輯：走完才移除) ---
        if (m_CurrentStage == 4) {
            if (m_Mario->IsJumping()) {
                // 如果跳起來，就清除踩踏紀錄（跳躍不能拔插銷）
                m_HasActiveRivet = false;
            } else if (foundSurface) {
                // 直接拿剛剛記下的座標去查詢 TileType
                TileType currentTile = m_Map->GetTileAtPosition(currentGroundTilePos.x, currentGroundTilePos.y);

                if (currentTile == TileType::RIVET) {
                    // 只要踩在插銷上，就持續更新紀錄這個插銷的中心座標
                    m_ActiveRivetPos = currentGroundTilePos;
                    m_HasActiveRivet = true;
                }
                else if (currentTile == TileType::FLOOR && m_HasActiveRivet) {
                    // 關鍵邏輯：目前踩的是普通地板，且「上一幀」還有踩在插銷上
                    auto [rx, ry] = m_Map->GetTileIndexAtPosition(m_ActiveRivetPos.x, m_ActiveRivetPos.y);

                    if (m_RivetVisuals.count({rx, ry})) {
                        // 1. 邏輯移除：將該網格設為 EMPTY
                        m_Map->SetTileAtPosition(m_ActiveRivetPos.x, m_ActiveRivetPos.y, TileType::EMPTY);

                        // 2. 視覺移除：移除 Character 物件
                        m_Renderer.RemoveChild(m_RivetVisuals[{rx, ry}]);
                        m_RivetVisuals.erase({rx, ry});
                        m_RivetCount--;
                        m_HUDText->AddScore(100);
                        LOG_INFO("Rivet removed at ({},{}), remaining: {}", rx, ry, m_RivetCount);

                        if (m_RivetCount <= 0) {
                            m_Mario->Win();
                            // 關鍵：觸發勝利後立刻更新渲染並結束 App::Update，防止下方邏輯修改狀態
                            goto END_OF_LOGIC;
                        }
                    }
                    // 拔掉後清除紀錄
                    m_HasActiveRivet = false;
                }
                else if (currentTile != TileType::RIVET) {
                    // 踩到其他東西（例如梯子或空地），重置紀錄
                    m_HasActiveRivet = false;
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
            // 取得 Mario 中心點的格子類型，用於輔助判定穿越厚地板的爬行
            //TileType tileAtCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y);

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

                    // --- [新增] 傳送帶推力邏輯 ---
                    if (m_CurrentStage == 2 && m_ConveyorSystem) {
                        // 根據當前踩踏的 Tile 取得速度 (px/s)
                        float beltVelocity = m_ConveyorSystem->GetVelocity(currentMarioTile);
                        if (std::abs(beltVelocity) > 0.0f) {
                            float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
                            glm::vec2 conveyorPushPos = m_Mario->GetPosition();
                            conveyorPushPos.x += beltVelocity * dt; // 套用水平位移
                            m_Mario->SetPosition(conveyorPushPos);
                        }
                    }

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
                // 關鍵修正：加入 tileAtCenter 判定
                // 當 Mario 的腳正在穿越 22 像素厚的地板時，腳底判定點會偵測不到樓梯
                // 但此時 Mario 的中心點 (Waist) 已經接觸到地板上方的樓梯格，確保他能持續向上爬直到登頂
                if ((tileFoot1 == TileType::LADDER || tileFoot2 == TileType::LADDER || // tileAtCenter == TileType::LADDER)
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

        // 4. 更新火球移動邏輯 (如果火球可見)
        if (m_Fireball->GetVisibility()) {
            m_Fireball->Update();
        }

        // 5. 碰撞偵測：Mario 與火球
        if (m_Fireball->GetVisibility()) {
            glm::vec2 marioSize = m_Mario->GetSize();
            if (m_Mario->GetState() == MarioState::HAMMERING) marioSize *= 1.8f;

           if (m_Fireball->IfCollides(m_Mario->GetPosition(), marioSize)) {
                if (m_Mario->GetState() == MarioState::HAMMERING) {
                    TriggerSmash(m_Fireball->GetPosition(), 800);
                    m_Fireball->SetVisible(false); // 擊碎火球
                } else {
                    m_Mario->Dead();
                }
            }
        }

        // 6. 道具偵測：撿起槌子
        MarioState s = m_Mario->GetState();
        bool canPickUp = (s == MarioState::JUMPING || s == MarioState::FALLING);
        if (m_HammerItem && m_HammerItem->GetVisibility() && canPickUp) {
            if (m_HammerItem->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                m_Mario->WaitForHammer();
                m_HammerItem->SetVisible(false);
            }
        }
        if (m_HammerItem2 && m_HammerItem2->GetVisibility() && canPickUp) {
            if (m_HammerItem2->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                m_Mario->WaitForHammer();
                m_HammerItem2->SetVisible(false);
            }
        }

        // 7. 更新 HUD (包含 Bonus 時間倒數)
        m_HUDText->Update(Util::Time::GetDeltaTimeMs());
    }

END_OF_LOGIC:
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
