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
        bool foundBarrelSurface = false;
        const float tileH = m_Map->GetTileHeight();
        const float searchRange = tileH * 1.5f; // 搜尋範圍設為 1.5 格，足以涵蓋斜坡落差

        // [地表掃描]：判斷酒桶目前是站在地板上，還是懸空
        // 採用你發現的邏輯：往下深探 1 像素，避免邊界判定誤差
        TileType currentBarrelTile = m_Map->GetTileAtPosition(pos.x, barrelFootY - 1.0f);

        if (currentBarrelTile == TileType::FLOOR || currentBarrelTile == TileType::RIVET) {
            // 向上修正：如果酒桶稍微陷入地板，將其抬起至表面
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tileAboveBarrel = m_Map->GetTileAtPosition(pos.x, barrelFootY + dy + 1.0f);
                if (tileAboveBarrel == TileType::EMPTY || tileAboveBarrel == TileType::LADDER) {
                    targetFootY = barrelFootY + dy;
                    foundBarrelSurface = true;
                    break;
                }
            }
        } else {
            // 向下修正：處理下坡情況，讓酒桶「貼著」斜坡向下滾動
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tileBelowBarrel = m_Map->GetTileAtPosition(pos.x, barrelFootY - dy);
                if (tileBelowBarrel == TileType::FLOOR || tileBelowBarrel == TileType::RIVET) {
                    targetFootY = barrelFootY - dy + 1.0f; // 修正到地板上方
                    foundBarrelSurface = true;
                    break;
                }
            }
        }

        // 偵測酒桶正下方是否有梯子 (稍微往下探測 2 像素)
        TileType footTile = m_Map->GetTileAtPosition(pos.x, targetFootY - 2.0f);

        // --- 3. 狀態機切換邏輯 ---
        if (barrel->GetState() == Barrel::State::ROLLING) {
            if (!foundBarrelSurface) {
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
            if (foundBarrelSurface) {
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
void App::UpdateCementPans(MarioState marioState) {

    for (auto it = m_CementPans.begin(); it != m_CementPans.end(); ) {
        auto& pan = *it;
        pan->Update(m_Map, m_ConveyorSystem); // 執行物理與位移

        // 碰撞偵測 (Mario)
        glm::vec2 marioSize = m_Mario->GetSize();
        if (marioState == MarioState::HAMMERING) marioSize *= 1.8f;

        if (pan->IfCollides(m_Mario->GetPosition(), marioSize)) {
            if (marioState == MarioState::HAMMERING) {
                // 搥擊成功：觸發特效、加分，並移除水泥塊
                TriggerSmash(pan->GetPosition(), 500);
                m_Renderer.RemoveChild(pan);
                it = m_CementPans.erase(it);
                continue;
            } else {
                // 碰撞失敗：Mario 死亡
                m_Mario->Dead();
            }
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
    SpawnPointVisual(position, score);
    m_FreezeTimer = 1500.0f; // 調整為凍結 1.5 秒
    LOG_DEBUG("SMASH TRIGGERED at ({}, {})", position.x, position.y);
}

void App::SpawnPointVisual(glm::vec2 position, int score) {
    std::string imagePath;
    if (score == 100) imagePath = RESOURCE_DIR"/Images/Point_100.png";
    else if (score == 300) imagePath = RESOURCE_DIR"/Images/Point_300.png";
    else if (score == 500) imagePath = RESOURCE_DIR"/Images/Point_500.png";
    else if (score == 800) imagePath = RESOURCE_DIR"/Images/Point_800.png";
    else return;

    auto visual = std::make_shared<Character>(imagePath);
    visual->SetPosition(position);
    visual->SetZIndex(80); // 確保顯示在特效與角色之上
    
    // 根據地圖縮放調整分數圖片大小
    visual->SetScale(m_Map->GetScale() * 1.8f); // 放大倍率調整為 1.8f，讓顯示更清楚

    m_PointVisuals.push_back({visual, 1500.0f}); // 1.5 秒後消失
    m_Renderer.AddChild(visual);
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
    int stageNum = level % 4;
    if (stageNum == 0)
        stageNum = 4;
    m_CurrentStage = static_cast<Stage>(stageNum);

    // 每次載入關卡前先清除舊酒桶(Stage 1)，確保場景完全重置
    for (auto& barrel : m_Barrels) m_Renderer.RemoveChild(barrel);
    m_Barrels.clear();

    // 只有在第一關時顯示固定木桶堆，其餘關卡隱藏。將此邏輯移至開頭以統一管理物件狀態。
    if (m_StaticBarrels) m_StaticBarrels->SetVisible(m_CurrentStage == Stage::BARRELS);

    // 每次載入關卡前先清除舊電梯(Stage 3)，確保畫面上不會殘留電梯踏板
    for (auto& el : m_Elevators) m_Renderer.RemoveChild(el);
    m_Elevators.clear();

    // 每次載入關卡前先清除舊得分特效
    for (auto& pv : m_PointVisuals) m_Renderer.RemoveChild(pv.character);
    m_PointVisuals.clear();

    // 每次載入關卡前先清除舊雨傘
    for (auto& u : m_Umbrellas) m_Renderer.RemoveChild(u);
    m_Umbrellas.clear();

    // 每次載入關卡前先清除舊皮包
    for (auto& p : m_Purses) m_Renderer.RemoveChild(p);
    m_Purses.clear();

    // 清除舊電梯擋板
    for (auto& stop : m_ElevatorStops) m_Renderer.RemoveChild(stop);
    m_ElevatorStops.clear();

    // 每次載入關卡前先清除舊插銷(Stage 4)，確保畫面上不會殘留
    for (auto& pair : m_RivetVisuals) m_Renderer.RemoveChild(pair.second);
    m_RivetVisuals.clear();

    // 清除舊伸縮梯子
    for (auto& ladder : m_MovingLadders) m_Renderer.RemoveChild(ladder);
    m_MovingLadders.clear();

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

    // 重置心形與公主狀態
    if (m_Heart) {
        m_Heart->SetVisible(false);
        m_Heart->SetImage(RESOURCE_DIR"/Images/heart.png"); // 還原成完整的愛心
    }
    if (m_Princess) {
        m_Princess->SetCurrentFrame(0); // 回到初始 Princess 圖案
        m_Princess->SetVisible(true);   // 確保公主重新顯示
        m_Princess->SetScale({m_Mario->marioScale, m_Mario->marioScale}); // 重置縮放
    }

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
        // 修正：不要在這裡 Init，否則會把剩餘生命值洗回 3
        m_HUDText->ResetBonus(5000);         
        m_HUDText->SetLevel(m_CurrentLevel); // 更新畫面上的 L=XX 文字
    }

    // 根據傳入的關卡編號，載入對應的地圖圖片與純文字檔 (MapX.txt)
    // 同時把所有物件 (Mario, 火球, 道具, 大金剛) 移動到該關卡適合的座標
    if (m_CurrentStage == Stage::BARRELS) {
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
            m_Princess->SetPosition({-15.0f, halfHeight - 35});
        }

    } else if (m_CurrentStage == Stage::CONVEYORS) {
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

        // 初始化兩側伸縮梯子 (座標需根據地圖實際梯子位置微調)
        float ladderY1 = 110.0f; // 伸長位置
        float ladderY2 = 65.0f; // 縮回位置 (向下移動約 48 像素)

        auto leftLadder = std::make_shared<MovingLadder>(-247.0f, ladderY1, ladderY2, MovingLadder::Side::LEFT, LadderState::EXTENDED);
        auto rightLadder = std::make_shared<MovingLadder>(247.0f, ladderY1, ladderY2, MovingLadder::Side::RIGHT, LadderState::RETRACTED);

        // 取得地圖目前的縮放比例，確保梯子倍率與背景一致
        glm::vec2 mapScale = m_Map->GetScale();

        // 設定梯子縮放與 ZIndex
        leftLadder->SetScale(mapScale);
        leftLadder->SetScale(mapScale);
        rightLadder->SetScale(mapScale);
        leftLadder->SetZIndex(10);
        rightLadder->SetZIndex(10);

        m_MovingLadders.push_back(leftLadder);
        m_MovingLadders.push_back(rightLadder);
        m_Renderer.AddChild(leftLadder);
        m_Renderer.AddChild(rightLadder);

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
            m_Princess->SetPosition({-15.0f, halfHeight - 35});
        }

    } else if (m_CurrentStage == Stage::ELEVATORS) {
        m_Map->LoadNewMap("../Resources/Images/board-elevators.png", "../Resources/Maps/Map3.txt");
        halfWidth = m_Map->GetMapWidth() / 2.0f;
        halfHeight = m_Map->GetMapHeight() / 2.0f;
        m_Mario->SetScreenBounds(halfWidth, halfHeight);
        m_Mario->SetPosition({-halfWidth + 50.0f, -halfHeight + 45.0f});

        // 取得地圖目前的縮放比例 (必須放在使用 mapScale 的邏輯之前)
        glm::vec2 mapScale = m_Map->GetScale();

        // 設定第三關 DK 只會搥胸
        m_DonkeyKong->SetPosition({-halfWidth + 120.0f, halfHeight - 110.0f});
        m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::MOVING_CHEST_BEATING);
        m_DonkeyKong->SetMoveBounds(-halfWidth + 120.0f, -halfWidth + 120.0f);
        if (m_Princess) {
            m_Princess->SetPosition({-15.0f, halfHeight - 35});
        }

        // --- 【新增】雨傘道具 (Stage 3) ---
        auto createUmbrella = [&](float x, float y) {
            auto u = std::make_shared<Character>(RESOURCE_DIR"/Images/umbrella.png");
            u->SetScale(mapScale * 1.5f);
            u->SetZIndex(40);
            u->SetPosition({x, y});
            m_Umbrellas.push_back(u);
            m_Renderer.AddChild(u);
        };

        // 1. 左邊最上層再往下一層 (DK 層下方)
        createUmbrella(-halfWidth + 30.0f, halfHeight - 260.0f);
        
        // 2. 最右邊最上面那層
        createUmbrella(halfWidth - 25.0f, halfHeight - 175.0f);

        // --- 【新增】皮包道具 (Stage 3) ---
        auto createPurse = [&](float x, float y) {
            auto p = std::make_shared<Character>(RESOURCE_DIR"/Images/purse.png");
            p->SetScale(mapScale * 1.5f);
            p->SetZIndex(40);
            p->SetPosition({x, y});
            m_Purses.push_back(p);
            m_Renderer.AddChild(p);
        };

        // 放在兩條移動電梯中間 (X 約在 -80)，高度設在中間層
        createPurse(-72.5f * mapScale.x, halfHeight - 460.0f);

        // 假設上下邊界是根據半高設定
        float elevatorTopY = 108.0f;
        float elevatorBotY = -halfHeight + 43.0f;
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

        // 新增：電梯擋板
        float leftElX = -145.0f * mapScale.x;
        float rightElX = -15.0f * mapScale.x;

        auto createStop = [&](float x, float y, bool upsideDown) {
            auto stop = std::make_shared<Character>(RESOURCE_DIR"/Images/ElevatorStop.png");
            stop->SetZIndex(40);
            if (upsideDown) {
                stop->SetScale({mapScale.x * 2.0f, -mapScale.y * 2.0f});
            } else {
                stop->SetScale(mapScale * 2.0f);
            }
            stop->SetPosition({x, y});
            m_ElevatorStops.push_back(stop);
            m_Renderer.AddChild(stop);
        };

        createStop(leftElX, elevatorTopY + 15.0f * mapScale.y, false);
        createStop(leftElX, elevatorBotY - 15.0f * mapScale.y, true);
        createStop(rightElX, elevatorTopY + 15.0f * mapScale.y, false);
        createStop(rightElX, elevatorBotY - 15.0f * mapScale.y, true);

    } else if (m_CurrentStage == Stage::RIVETS) {
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
    m_StaticBarrels = std::make_shared<Character>(RESOURCE_DIR"/Images/Barrel00.png");
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
    m_BlackCover->SetZIndex(90); // 層級提高，遮住地圖與一般角色，但低於 HUD (100)
    m_BlackCover->SetScale({1500.0f, 1500.0f}); // 放大到足以覆蓋全螢幕
    m_BlackCover->SetVisible(false);
    m_Renderer.AddChild(m_BlackCover);

    // 【新增】初始化過場文字物件
    m_TransitionText = std::make_shared<Util::Text>(RESOURCE_DIR"/Fonts/PressStart2P-Regular.ttf", 20, "HOW HIGH CAN YOU GET?", Util::Color(255, 255, 255, 255));
    m_TransitionTextObj = std::make_shared<Util::GameObject>();
    m_TransitionTextObj->SetDrawable(m_TransitionText);
    m_TransitionTextObj->SetZIndex(100);
    m_TransitionTextObj->SetVisible(false);
    m_Renderer.AddChild(m_TransitionTextObj);

    // 初始化公主精靈 (Princess + Princess2/Princess3 + Princess_win)
    m_Princess = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
        RESOURCE_DIR"/Images/Princess.png",
        RESOURCE_DIR"/Images/Princess2.png",
        RESOURCE_DIR"/Images/Princess3.png",
        RESOURCE_DIR"/Images/Princess_win.png" // Frame 3: 勝利圖案
    });
    m_Princess->SetZIndex(60); // 放在大金剛下方但角色上方

    // 初始化心形圖案
    m_Heart = std::make_shared<Character>(RESOURCE_DIR"/Images/heart.png");
    m_Heart->SetZIndex(70); // 顯示在最上層
    m_Heart->SetVisible(false);
    m_Renderer.AddChild(m_Heart);

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

    // 【新增】初始化 Game Over 視覺物件 (小長方形黑塊 + 紅色文字)
    m_GameOverBlock = std::make_shared<Character>(RESOURCE_DIR"/Images/black.png");
    m_GameOverBlock->SetZIndex(110); // 確保在所有物件之上
    m_GameOverBlock->SetScale({450.0f, 150.0f});
    m_GameOverBlock->SetPosition({0.0f, 0.0f});
    m_GameOverBlock->SetVisible(false);
    m_Renderer.AddChild(m_GameOverBlock);

    m_GameOverText = std::make_shared<Util::Text>(RESOURCE_DIR"/Fonts/PressStart2P-Regular.ttf", 30, "GAME OVER", Util::Color(255, 0, 0, 255));
    m_GameOverTextObj = std::make_shared<Util::GameObject>();
    m_GameOverTextObj->SetDrawable(m_GameOverText);
    m_GameOverTextObj->SetZIndex(151); // 提高層級，確保在最上層
    m_GameOverTextObj->m_Transform.translation = {0.0f, 0.0f}; // 強制定位在螢幕中心
    m_GameOverTextObj->SetVisible(false);
    m_Renderer.AddChild(m_GameOverTextObj);

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

    // 【新增】處理 Game Over 狀態下的重啟邏輯
    if (m_IsGameOver) {
        if (Util::Input::IsKeyDown(Util::Keycode::R) || Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
            m_IsGameOver = false;
            m_GameOverBlock->SetVisible(false);
            m_GameOverTextObj->SetVisible(false);
            m_CurrentLevel = 1;
            m_HUDText->Init(); // 這會同時重置分數、Bonus 與生命值
            LoadLevel(m_CurrentLevel);
        }
        m_Renderer.Update();
        return;
    }
#endif

    // 1. 取得當前狀態，判定是否處於「可遊玩」狀態
    MarioState marioState = m_Mario->GetState();
    bool isPlaying = (marioState != MarioState::DEAD && marioState != MarioState::WIN);
    float dt = static_cast<float>(Util::Time::GetDeltaTimeMs());

    // --- 【新增】過場畫面處理 ---
    if (m_InTransition) {
        m_TransitionTimer -= dt;
        m_BlackCover->SetVisible(true);
        m_BlackCover->SetPosition({0.0f, 0.0f}); // 確保黑屏在畫面中央
        
        m_BlackCover->SetScale({2000.0f, 2000.0f}); // 覆蓋全螢幕

        // --- 初始化過場堆疊物件 (僅在過場開始時執行一次) ---
        if (m_TransitionIcons.empty()) {
            m_DonkeyKong->SetVisible(false); // 隱藏原本的 DK
            m_Princess->SetVisible(false);   // 隱藏公主
            
            int targetLevel = m_CurrentLevel + 1; // 準備進入的關卡
            int displayCount = std::min(targetLevel, 6); // 最多顯示 6 層 (150m)

            for (int i = 1; i <= displayCount; ++i) {
                // 建立 Donkey 圖示
                auto kong = std::make_shared<Character>(RESOURCE_DIR"/Images/height-kong.png");
                // 建立高度文字圖示 (height-1.png, height-2.png...)
                auto label = std::make_shared<Character>(RESOURCE_DIR"/Images/height-" + std::to_string(i) + ".png");

                float yPos = -160.0f + (i - 1) * 90.0f; // 調整起始高度與每一層的向上間距
                kong->SetPosition({40.0f, yPos});      // Donkey 在右側
                label->SetPosition({-90.0f, yPos});    // 高度數字在左側

                kong->SetScale(m_Map->GetScale() * 1.0f);  // 縮小倍率 1.5f
                label->SetScale(m_Map->GetScale() * 1.0f);
                kong->SetZIndex(95);
                label->SetZIndex(95);

                m_TransitionIcons.push_back(kong);
                m_TransitionIcons.push_back(label);
                m_Renderer.AddChild(kong);
                m_Renderer.AddChild(label);
            }

            m_TransitionText->SetText("HOW HIGH CAN YOU GET?");
            m_TransitionTextObj->m_Transform.translation = {0.0f, -270.0f}; // 調整文字在底部的座標
            m_TransitionTextObj->SetVisible(true);
        }

        if (m_TransitionTimer <= 0.0f) {
            // 清理過場物件
            for (auto& icon : m_TransitionIcons) m_Renderer.RemoveChild(icon);
            m_TransitionIcons.clear();

            m_InTransition = false;
            m_DonkeyKong->SetVisible(true);
            m_TransitionTextObj->SetVisible(false);
            m_CurrentLevel++;
            LoadLevel(m_CurrentLevel); // 載入新關卡（這會自動隱藏 m_BlackCover）
        }
        m_HUDText->Update(dt);
        m_Renderer.Update();
        return; // 過場中不執行下方遊戲邏輯
    }

    // 更新得分視覺顯示的倒數計時與移除邏輯
    // 放在 Update 開頭確保即使在 Freeze 期間也能處理消失
    for (auto it = m_PointVisuals.begin(); it != m_PointVisuals.end(); ) {
        it->remainingTimeMs -= dt;
        if (it->remainingTimeMs <= 0.0f) {
            m_Renderer.RemoveChild(it->character);
            it = m_PointVisuals.erase(it);
        } else {
            ++it;
        }
    }

    // 當目前已經通關，並且處於勝利狀態時，我們讓玩家按下按鈕後可以自動進到下一關
    if (marioState == MarioState::WIN) {
        
        // --- 【新增】勝利視覺表現 ---
        if (m_Princess) {
            m_Princess->SetCurrentFrame(3); // 切換到 Princess_win.png
        }
        if (m_Heart) {
            m_Heart->SetVisible(true);
            glm::vec2 pPos = m_Princess->GetPosition();
            glm::vec2 mPos = m_Mario->GetPosition();
            // 將心形放在兩人中間，稍微偏上方
            m_Heart->SetPosition({(pPos.x + mPos.x) / 2.0f, pPos.y + 10.0f});
            m_Heart->SetScale(m_Map->GetScale() * 1.5f);
        }

        // --- 【新增】Donkey Kong 撤退動畫 (Stage 1-3) ---
        if (m_CurrentStage != Stage::RIVETS) {
            if (m_DonkeyKong->GetBehavior() != DonkeyKong::Behavior::CLIMBING_AWAY &&
                m_DonkeyKong->GetBehavior() != DonkeyKong::Behavior::CLIMBING_WITH_PRINCESS) {
                m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::CLIMBING_AWAY);
                // 將 DK 移動到公主下方的階梯位置 (通常在公主所在平台的階梯口)
                glm::vec2 pPos = m_Princess->GetPosition();
                // 調整 X 偏移量 (約 -85) 使其位於雙梯正中央，高度設在平台下方
                m_DonkeyKong->SetPosition({pPos.x - 85.0f, pPos.y - 80.0f});
                // 放大 Donkey Kong 的比例 (從原先縮小改為放大 1.2 倍)
                // 這樣他的寬度就能夠涵蓋兩條並排的梯子
                float dkScale = m_Mario->marioScale * 1.2f;
                m_DonkeyKong->SetScale({dkScale, dkScale});
            }

            // 偵測高度：當 DK 爬到與公主相同高度時，觸發抓走公主的劇情
            if (m_DonkeyKong->GetBehavior() == DonkeyKong::Behavior::CLIMBING_AWAY) {
                if (m_DonkeyKong->GetPosition().y >= m_Princess->GetPosition().y) {
                    m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::CLIMBING_WITH_PRINCESS);
                    m_Princess->SetVisible(false); // 隱藏原本的公主
                    m_Heart->SetImage(RESOURCE_DIR"/Images/heart_broken.png"); // 愛心碎裂
                    LOG_DEBUG("Donkey Kong kidnapped the Princess!");
                }
            }

            // 【自動觸發過場】當 Donkey Kong 抱著公主爬出畫面頂端時，自動進入過場
            if (m_DonkeyKong->GetBehavior() == DonkeyKong::Behavior::CLIMBING_WITH_PRINCESS &&
                m_DonkeyKong->GetPosition().y > halfHeight + 50.0f) {
                m_InTransition = true;
                m_TransitionTimer = 2500.0f; // 顯示 2.5 秒
            }

            m_DonkeyKong->Update(); // 勝利時也需要驅動 DK 的 Update 以執行爬行位移
        }

        // --- Stage 4 特有的通關動畫：DK 掉下去後自動進入過場 ---
        if (m_CurrentStage == Stage::RIVETS) {
            m_BlackCover->SetVisible(true);
            m_BlackCover->SetPosition({0.0f, -45.0f}); // 僅遮住中間結構，保留兩側地圖

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
            } else {
                // 當 DK 掉到底部後，自動觸發全黑屏過場
                if (!m_InTransition) {
                    m_InTransition = true;
                    m_TransitionTimer = 3000.0f; 
                }
            }
        }
    }

    // --- 【新增】死亡後續處理：生命值減少與 Game Over 判定 ---
    if (marioState == MarioState::DEAD && m_Mario->IsDeathAnimationDone()) {
        m_HUDText->DecreaseLife();
        if (m_HUDText->GetLives() <= 0) {
            // 進入 Game Over 狀態
            m_IsGameOver = true;
            m_Mario->SetPosition({0.0f, -2000.0f}); // 將 Mario 移出畫面
            m_GameOverBlock->SetVisible(true);
            m_GameOverBlock->SetZIndex(150); // 確保黑塊在 HUD 之上
            m_GameOverTextObj->SetVisible(true);
        } else {
            // 生命還夠，重新載入當前關卡
            LoadLevel(m_CurrentLevel);
        }
        m_Renderer.Update();
        return;
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

#if 1 //sdbg
        // 2. 更新 DonkeyKong (若停止更新，其產酒桶的回呼就不會觸發)
        if (m_DonkeyKong) {
            m_DonkeyKong->Update();
        }
#endif
        // 更新木桶邏輯
        if (m_CurrentStage == Stage::BARRELS) {
            UpdateBarrels(marioState);
        }

        // 更新傳送帶系統與 Spawner
        if (m_CurrentStage == Stage::CONVEYORS && m_ConveyorSystem) {
            float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
            m_ConveyorSystem->Update(dtMs);     // 更新傳送帶系統內部計時與方向狀態

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

            UpdateCementPans(marioState);
        }

        // 更新伸縮梯子計時器
        if (m_CurrentStage == Stage::CONVEYORS) {
            for (auto& ladder : m_MovingLadders) {
                ladder->Update(static_cast<float>(Util::Time::GetDeltaTimeMs()));
            }
        }

        // 更新電梯位移邏輯
        if (m_CurrentStage == Stage::ELEVATORS) {
            for (auto& elevator : m_Elevators) {
                elevator->Update();
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
        glm::vec2 currentGroundTilePos = {0.0f, 0.0f}; // 【新增】儲存當前踩踏地板的中心座標
        const float tileH = m_Map->GetTileHeight();

        // 跳躍中搜尋範圍可以稍微加大，避免高速下落穿透
        const float searchRange = m_Mario->IsJumping() ? tileH * 2.0f : tileH * 1.5f;

        // 採用你發現的邏輯：往下深探 1 像素，確保穩定偵測到當前踩踏的地板
        TileType currentMarioFootTile = m_Map->GetTileAtPosition(marioPos.x, footY - 1.0f);
        TileType tileCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y); // 用中心點檢查
        TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) + 3.0f);

        // [修正] 地面偵測邏輯：加入對傳送帶 Tile 的判定
        if (currentMarioFootTile == TileType::FLOOR ||
            currentMarioFootTile == TileType::RIVET ||
            currentMarioFootTile == TileType::CONVEYOR1 ||
            currentMarioFootTile == TileType::CONVEYOR2 ||
            currentMarioFootTile == TileType::CONVEYOR3) {
            // 向上找 FLOOR
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tile = m_Map->GetTileAtPosition(marioPos.x, footY + dy + 1.0f);
                if (tile == TileType::EMPTY || tile == TileType::LADDER) {
                    targetFootY = footY + dy;
                    foundSurface = true;
                    break;
                }
            }
        } else {
            // 向下找 FLOOR
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tileBelowDY = m_Map->GetTileAtPosition(marioPos.x, footY - dy);
                if ((tileFoot1 != TileType::FLOOR) &&
                 (tileBelowDY == TileType::FLOOR ||
                    tileBelowDY == TileType::RIVET ||
                    tileBelowDY == TileType::CONVEYOR1 ||
                    tileBelowDY == TileType::CONVEYOR2 ||
                    tileBelowDY == TileType::CONVEYOR3)) {
                    targetFootY = footY - dy + 1.0f;
                    foundSurface = true;
                    currentMarioFootTile = tileBelowDY; // 重要：更新目前偵測到的 Tile 類型，以便後續處理傳送帶速度
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
        if (!foundSurface && m_CurrentStage == Stage::ELEVATORS) {
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
        if (m_CurrentStage == Stage::RIVETS) {
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
                        SpawnPointVisual(m_ActiveRivetPos, 100);
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
            //TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) + 3.0f);
            TileType tileFoot2 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - 1.0f);
            //TileType tileCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y); // 用中心點檢查

            // [落地檢查]
            // 如果在墜落狀態中偵測到表面，則恢復為靜止狀態。
            if (foundSurface && m_Mario->GetState() == MarioState::FALLING) {
                m_Mario->IDLE();
            }

            MarioState state = m_Mario->GetState();
            bool isClimbing = (state == MarioState::CLIMBING || state == MarioState::CLIMB_IDLE);

            // [新增] 伸縮梯子即時狀態檢查：若正在爬伸縮梯，但梯子縮回，則強迫墜落
            if (isClimbing) {
                if (tileCenter == TileType::MOVING_LADDER_LEFT || tileCenter == TileType::MOVING_LADDER_RIGHT) {
                    auto side = (tileCenter == TileType::MOVING_LADDER_LEFT) ? MovingLadder::Side::LEFT : MovingLadder::Side::RIGHT;
                    bool canStillClimb = false;
                    for (auto& ladder : m_MovingLadders) {
                        if (ladder->GetSide() == side && ladder->IsClimbable()) {
                            canStillClimb = true;
                            break;
                        }
                    }
                    if (!canStillClimb) {
                        m_Mario->Fall();
                    }
                }
            }

            // [物理吸附與墜落轉換]
            // 只有在非跳躍、非攀爬的狀態下才進行地面貼合修正
            if (state != MarioState::JUMPING && !isClimbing) {
                if (foundSurface) {
                    // 將 Mario 的位置修正到地板表面高度
                    m_Mario->SetPosition({marioPos.x, targetFootY + (marioSize.y / 2.0f)});

                    // --- [新增] 傳送帶推力邏輯 ---
                    if (m_CurrentStage == Stage::CONVEYORS && m_ConveyorSystem) {
                        // 根據當前踩踏的 Tile 取得速度 (px/s)
                        float beltVelocity = m_ConveyorSystem->GetVelocity(currentMarioFootTile);
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
                if ((tileFoot1 == TileType::LADDER || //tileFoot2 == TileType::LADDER || // tileAtCenter == TileType::LADDER)
                    ((tileBelow == TileType::LADDER || tileBelow == TileType::MOVING_LADDER_LEFT || tileBelow == TileType::MOVING_LADDER_RIGHT) && !foundSurface) ||
                   // (tileBelow == TileType::LADDER && !foundSurface) ||
                    tileFoot1 == TileType::MOVING_LADDER_LEFT || tileFoot1 == TileType::MOVING_LADDER_RIGHT ||
                    tileFoot2 == TileType::MOVING_LADDER_LEFT || tileFoot2 == TileType::MOVING_LADDER_RIGHT)
                 && m_Mario->GetState() != MarioState::HAMMERING) {

                    // 若踩在伸縮梯子上，額外判斷梯子狀態
                    bool ladderOk = true;
                    //if (tileFoot1 == TileType::MOVING_LADDER_LEFT || tileFoot2 == TileType::MOVING_LADDER_LEFT) {
                    if (tileCenter == TileType::MOVING_LADDER_LEFT) {
                        for(auto& l : m_MovingLadders) if(l->GetSide()==MovingLadder::Side::LEFT) ladderOk = l->IsClimbable();
                    } else
                    //if (tileFoot1 == TileType::MOVING_LADDER_RIGHT || tileFoot2 == TileType::MOVING_LADDER_RIGHT) {
                    if (tileCenter == TileType::MOVING_LADDER_RIGHT) {
                        for(auto& l : m_MovingLadders) if(l->GetSide()==MovingLadder::Side::RIGHT) ladderOk = l->IsClimbable();
                    }

                    if (ladderOk) {
                        m_Mario->Climb(CLIMB_DIR::UP);
                    }
                }
            }
            // 向下攀爬
            else if (Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
                if ((tileFoot2 == TileType::LADDER || tileBelow == TileType::LADDER ||
                     tileFoot2 == TileType::MOVING_LADDER_LEFT || tileFoot2 == TileType::MOVING_LADDER_RIGHT ||
                     tileBelow == TileType::MOVING_LADDER_LEFT || tileBelow == TileType::MOVING_LADDER_RIGHT)
                 &&  m_Mario->GetState() != MarioState::HAMMERING) {

                    bool ladderOk = true;
                    if (tileFoot2 == TileType::MOVING_LADDER_LEFT || tileBelow == TileType::MOVING_LADDER_LEFT){
                        for(auto& l : m_MovingLadders) if(l->GetSide()==MovingLadder::Side::LEFT) ladderOk = l->IsClimbable();
                    } else if (tileFoot2 == TileType::MOVING_LADDER_RIGHT || tileBelow == TileType::MOVING_LADDER_RIGHT) {
                        for(auto& l : m_MovingLadders) if(l->GetSide()==MovingLadder::Side::RIGHT) ladderOk = l->IsClimbable();
                    }

                    if (ladderOk) {
                        m_Mario->Climb(CLIMB_DIR::DOWN);
                    }
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

        // 3.5 電梯擋板碰撞偵測
        for (auto& stop : m_ElevatorStops) {
            if (stop->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                glm::vec2 mPos = m_Mario->GetPosition();
                glm::vec2 sPos = stop->GetPosition();
                glm::vec2 mSize = m_Mario->GetSize();
                glm::vec2 sSize = stop->GetSize();

                // 找出最小推擠量
                float overlapX = (mSize.x + sSize.x) / 2.0f - std::abs(mPos.x - sPos.x);
                float overlapY = (mSize.y + sSize.y) / 2.0f - std::abs(mPos.y - sPos.y);

                if (overlapX < overlapY) {
                    if (mPos.x < sPos.x) mPos.x -= overlapX;
                    else mPos.x += overlapX;
                } else {
                    if (mPos.y < sPos.y) mPos.y -= overlapY;
                    else mPos.y += overlapY;
                }
                m_Mario->SetPosition(mPos);
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

        // 6.2 道具偵測：雨傘 (Umbrella)
        for (auto it = m_Umbrellas.begin(); it != m_Umbrellas.end(); ) {
            if ((*it)->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                glm::vec2 pos = (*it)->GetPosition();
                m_HUDText->AddScore(300); // 加上 300 分
                m_Renderer.RemoveChild(*it);
                it = m_Umbrellas.erase(it);
                SpawnPointVisual(pos, 300);
                LOG_DEBUG("Mario picked up an umbrella! +300 points.");
            } else {
                ++it;
            }
        }

        // 6.3 道具偵測：皮包 (Purse)
        for (auto it = m_Purses.begin(); it != m_Purses.end(); ) {
            if ((*it)->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                glm::vec2 pos = (*it)->GetPosition();
                m_HUDText->AddScore(500); // 加上 500 分
                m_Renderer.RemoveChild(*it);
                it = m_Purses.erase(it);
                SpawnPointVisual(pos, 500);
                LOG_DEBUG("Mario picked up a purse! +500 points.");
            } else {
                ++it;
            }
        }

        // 6.5 碰撞偵測：Mario 與公主 (m_Princess)
        if (m_Princess && m_Princess->GetVisibility()) {
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            const auto princessPos = m_Princess->GetPosition();
            const auto princessSize = m_Princess->GetSize();

            const auto marioHalfSize = marioSize / 2.0f;
            const auto princessHalfSize = princessSize / 2.0f;

            if (std::abs(marioPos.x - princessPos.x) < (marioHalfSize.x + princessHalfSize.x) &&
                std::abs(marioPos.y - princessPos.y) < (marioHalfSize.y + princessHalfSize.y) &&
                marioPos.y >= (halfHeight - 50.0f)) {
                m_Mario->Win();
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
