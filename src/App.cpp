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
#include <cstdlib> // 加入 rand()
#include "ConveyorSystem.hpp"
#include "CementSpawner.hpp"

// 引用 Mario.cpp 中定義的全域變數
extern float g_MarioTotalJumpTime;

// 地面上的槌子道具物件
static std::shared_ptr<Character> m_HammerItem;
static std::shared_ptr<Character> m_HammerItem2;
static std::shared_ptr<Character> m_StaticBarrels;
static std::shared_ptr<Character> m_OilBarrel;
static std::shared_ptr<AnimatedCharacter> m_BurningOilBarrel;
static std::shared_ptr<AnimatedCharacter> m_FuelCan;
static std::vector<std::shared_ptr<Character>> m_Ufos; // 用於管理 UFO 道具

// 傳送帶關卡物件
static std::shared_ptr<ConveyorSystem> m_ConveyorSystem;
static std::vector<std::shared_ptr<CementSpawner>> m_CementSpawners;
static std::shared_ptr<AnimatedCharacter> m_PivotLeft;
static std::shared_ptr<AnimatedCharacter> m_PivotRight;
static std::shared_ptr<AnimatedCharacter> m_PivotTopLeft;
static std::shared_ptr<AnimatedCharacter> m_PivotTopRight;
static std::shared_ptr<AnimatedCharacter> m_PivotMidLeft;
static std::shared_ptr<AnimatedCharacter> m_PivotMidRight;

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

    // 先宣告指標就好，不要提早初始化，避免浪費資源
    std::shared_ptr<Barrel> newBarrel;

    if (m_CurrentStage == App::Stage::BARRELS && m_IsFirstBarrel) {
        // 設定一出生就是 FALLING_EDGE 狀態，並把種類設為 BLUE
        newBarrel = std::make_shared<Barrel>(Barrel::State::FALLING_EDGE, Barrel::Direction::LEFT, Barrel::BarrelType::BLUE);
        m_IsFirstBarrel = false; // 標記用完
    } else {
        newBarrel = std::make_shared<Barrel>(Barrel::State::ROLLING, Barrel::Direction::RIGHT, Barrel::BarrelType::NORMAL);
    }

    // 讓木桶持有地圖指標，這樣它在 Barrel::Update() 裡才能呼叫 GetTileAtPosition
    newBarrel->SetMap(m_Map);
    newBarrel->SetZIndex(40);

    // 2. 取得大金剛與酒桶的相關資訊
    glm::vec2 dkPos = m_DonkeyKong->GetPosition();
    glm::vec2 dkSize = m_DonkeyKong->GetSize();
    glm::vec2 barrelSize = newBarrel->GetSize();

    // 3. 計算「腳邊」座標
    // X 軸：放在大金剛中心點右側，加上兩者寬度的一半再加上一點間隙 (5.0f)
    float spawnX = dkPos.x + (dkSize.x / 2.0f) + 10.0f;
    // Y 軸：大金剛的腳底 (dkPos.y - dkSize.y/2) 加上酒桶的一半高度，使其底部齊平
    float spawnY = (dkPos.y - (dkSize.y / 2.0f)) + (barrelSize.y / 2.0f);


    if (newBarrel->GetType() == Barrel::BarrelType::BLUE) {
        spawnX = dkPos.x - (dkSize.x / 2.0f) + 35.0f; // 根據實際畫面可微調這個 15.0f
        float spawnY = dkPos.y;
        newBarrel->SetPosition({spawnX, spawnY});
    }

    newBarrel->SetPosition({spawnX, spawnY});
    // 告知木桶：你現在就在這個高度，不要拿 0.0 跟我算高低差
    newBarrel->ResetFallHeight(spawnY);

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
    for (auto& barrel : m_Barrels) {
        // 1. 呼叫木桶自己的更新邏輯 (包含滾動、掉落、地圖偵測)
        barrel->Update();

        // 2. 檢查是否與 Mario 發生碰撞 (如果木桶還在畫面上的話)
        if (barrel->GetVisibility()) {

            // ========================================================
            // 【修改重點開始】：處理所有木桶與油桶的碰撞
            // ========================================================
            bool hitOilBarrel = false;

            // 狀況 A：檢查是否碰到「未起火的靜態油桶」
            if (m_OilBarrel && m_OilBarrel->GetVisibility() &&
                barrel->IfCollides(m_OilBarrel->GetPosition(), m_OilBarrel->GetSize())) {
                hitOilBarrel = true;
            }
            // 狀況 B：檢查是否碰到「已經起火的動態油桶」
            else if (m_BurningOilBarrel && m_BurningOilBarrel->GetVisibility() &&
                     barrel->IfCollides(m_BurningOilBarrel->GetPosition(), m_BurningOilBarrel->GetSize())) {
                hitOilBarrel = true;
            }

            // 只要碰到任何一種油桶，就執行沒收邏輯
            if (hitOilBarrel) {
                barrel->SetVisible(false); // 1. 不管什麼顏色，木桶一律消失！

                // 2. 只有「藍色木桶」且「油桶還沒起火」時，才觸發點火與給分
                if (barrel->GetType() == Barrel::BarrelType::BLUE && m_OilBarrel && m_OilBarrel->GetVisibility()) {

                    m_OilBarrel->SetVisible(false); // 隱藏靜態油桶

                    if (m_BurningOilBarrel) {
                        m_BurningOilBarrel->SetVisible(true);
                        m_BurningOilBarrel->Play(); // 播放起火動畫
                    }
                    if (m_HUDText) {
                        m_HUDText->AddScore(100);
                    }
                    if (m_Fireball && !m_Fireball->GetVisibility()) {
                        m_Fireball->SetVisible(true);
                        // 將火焰的初始座標設定在油桶的位置
                        m_Fireball->SetPosition(m_BurningOilBarrel->GetPosition());
                        // (可選) 根據你的實作，可能需要呼叫 m_Fireball->Reset() 或初始化方向

                        m_Fireball->SetState(Fiamma::State::FALLING);
                    }
                }

                // 木桶已經掉進油桶消失了，不用再往下檢查是否砸到瑪利歐
                continue;
            }
            // ========================================================
            // 【修改重點結束】
            // ========================================================

            // 根據 Mario 狀態調整碰撞框大小
            glm::vec2 marioSize = m_Mario->GetSize();
            if (marioState == MarioState::HAMMERING) {
                marioSize *= 1.8f; // 拿槌子時攻擊範圍變大
            } else if (marioState == MarioState::JUMPING) {
                // [優化] 跳躍時縮減瑪利歐頂部的判定，避免撞到上層地板的木桶
                // 將高度判定縮減為 70%，且位置稍微往下靠
                marioSize.y *= 0.7f;
            }

            // AABB 碰撞偵測 (與瑪利歐)
            if (barrel->IfCollides(m_Mario->GetPosition(), marioSize)) {
                if (marioState == MarioState::HAMMERING) {
                    // Mario 拿著槌子：擊碎木桶
                    TriggerSmash(barrel->GetPosition(), 500); // 隨機或固定給分
                    barrel->SetVisible(false); // 標記為不可見，等待 App::Update 回收
                } else {
                    // Mario 沒拿槌子：死亡
                    m_Mario->Dead();

                    // 【建議補上這個安全機制，防止死後還能二段跳】
                    goto END_OF_LOGIC;
                }
            }
        }
    }
END_OF_LOGIC:
    return; // 提早結束這回合的木桶更新
}

/**
 * @brief 更新水泥塊邏輯更新器。
 */
void App::UpdateCementPans(MarioState marioState) {

    for (auto it = m_CementPans.begin(); it != m_CementPans.end(); ) {
        auto& pan = *it;

        glm::vec2 pos = pan->GetPosition();
        TileType type = pan->GetTargetBelt();
        // 取得中層傳送帶的標準高度 (Logic 300 -> Engine 60)
        static const float midLayerEngineY = CoordinateManager::LogicToEngine({0.0f, 300.0f}).y;
        float fuelCanEngineY = 20.0f; // 這是畫面中央油桶的實際引擎 Y 座標

        // 判定水泥塊類型
        bool canFall = (type == TileType::CONVEYOR2 || type == TileType::CONVEYOR3);
        bool isFalling = canFall && (pos.y < midLayerEngineY - 2.0f && pos.y > fuelCanEngineY - 10.0f);

        if (isFalling) {
            float dtSec = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
            // 顯著降低下墜速度
            pos.y -= 115.0f * dtSec;

            // 增加向中心（油桶）匯聚的位移，讓水泥塊看起來是「掉進」油桶
            if (pos.x > 2.0f) pos.x -= 40.0f * dtSec;
            else if (pos.x < -2.0f) pos.x += 40.0f * dtSec;

            pan->SetPosition(pos);
            // 抵達油桶高度後正式移除
            if (pos.y <= fuelCanEngineY) {
                m_Renderer.RemoveChild(pan);
                it = m_CementPans.erase(it);
                continue;
            }
        } else {
            pan->Update(m_Map, m_ConveyorSystem); // 執行正常的傳送帶物理與位移
        }

        // 碰撞偵測 (Mario)
        glm::vec2 marioSize = m_Mario->GetSize();
        if (marioState == MarioState::HAMMERING) {
            marioSize *= 1.8f;
        } else if (marioState == MarioState::JUMPING) {
            marioSize.y *= 0.7f; // 跳躍時縮減高度判定
        }

        const auto panPos = pan->GetPosition();
        const auto panSize = pan->GetSize();
        const auto marioPos = m_Mario->GetPosition();
        if (std::abs(panPos.x - marioPos.x) < (panSize.x + marioSize.x) / 2.0f &&
            std::abs(panPos.y - marioPos.y) < (panSize.y + marioSize.y) / 2.0f) {
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
        pos = pan->GetPosition();

        // [新增] 掉落效果觸發邏輯：
        // 當水泥在中間層（y=55）且接近中心區域時。
         // 當水泥在中間層（y=55）且因為到達末端準備消失（ShouldRemove），且位於中心轉軸附近時
        if (canFall && !isFalling && pan->ShouldRemove() && std::abs(pos.y - midLayerEngineY) < 15.0f && std::abs(pos.x) < 75.0f) {
            // 1. 切換圖片為 flip_cement.png
            pan->SetImage(RESOURCE_DIR"/Images/flip_cement.png");

            // [修正] 設定較低的 Z-Index (40)，確保它在 FuelCan (45) 的圖層下面
            pan->SetZIndex(40);

            // 2. 處理縮放與翻轉 (由右邊過來的 x 為正，需水平翻轉圖案)
            float baseScale = m_Map->GetScale().x * 1.7f;
            if (pos.x > 0) {
                pan->SetScale({-baseScale, baseScale});
            } else {
                pan->SetScale({baseScale, baseScale});
            }

            // 3. 向下移動足夠距離，確保下一幀必定進入 isFalling 判定區間
            pos.y -= 8.0f;
            pan->SetPosition(pos);

            // 4. 攔截本次移除動作，讓它能繼續存在於列表中進行下墜動畫
            ++it;
            continue;
        }

        // 邊界檢查：只有不在下墜狀態，且（確定移除或超出畫面邊界）才銷毀
        if (!isFalling && (pan->ShouldRemove() || std::abs(pos.x) > halfWidth + 20.0f)) {
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

void App::CheckJumpScore() {
    // 只有在跳躍狀態才進行判定
    if (m_Mario->GetState() != MarioState::JUMPING && m_Mario->GetState() != MarioState::FALLING) {
        return;
    }

    glm::vec2 marioPos = m_Mario->GetPosition();
    glm::vec2 marioSize = m_Mario->GetSize();
    float marioBottom = marioPos.y - (marioSize.y / 2.0f);

    auto process = [&](void* id, glm::vec2 objPos, glm::vec2 objSize) {
        if (m_JumpOverObstacles.count(id)) return;

        // 檢查 X 軸重疊：瑪利歐的 X 軸與障礙物交會時
        float marioLeft = marioPos.x - (marioSize.x / 4.0f); // 縮小範圍增加精準度
        float marioRight = marioPos.x + (marioSize.x / 4.0f);
        float objLeft = objPos.x - (objSize.x / 2.0f);
        float objRight = objPos.x + (objSize.x / 2.0f);

        if (marioRight > objLeft && marioLeft < objRight) {
            // 檢查 Y 軸：瑪利歐底部必須高於障礙物頂部
            float objTop = objPos.y + (objSize.y / 2.0f);
            float distY = marioBottom - objTop;
            // 僅紀錄跳過的物件 ID，不在此處加分
            if (distY > 0 && distY < 50.0f) {
                m_JumpOverObstacles.insert(id);
            }
        }
    };

    for (auto& b : m_Barrels) {
        if(b->GetVisibility()) process(b.get(), b->GetPosition(), b->GetSize());
    }
    if (m_Fireball && m_Fireball->GetVisibility()) {
        process(m_Fireball.get(), m_Fireball->GetPosition(), m_Fireball->GetSize());
    }
    if (m_Fireball2 && m_Fireball2->GetVisibility()) {
        process(m_Fireball2.get(), m_Fireball2->GetPosition(), m_Fireball2->GetSize());
    }
    if (m_Fireball3 && m_Fireball3->GetVisibility()) {
        process(m_Fireball3.get(), m_Fireball3->GetPosition(), m_Fireball3->GetSize());
    }
    if (m_Fireball4 && m_Fireball4->GetVisibility()) {
        process(m_Fireball4.get(), m_Fireball4->GetPosition(), m_Fireball4->GetSize());
    }
    for (auto& p : m_CementPans) {
        process(p.get(), p->GetPosition(), p->GetSize());
    }
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

    int stageNum = level % 4;
    if (stageNum == 0) stageNum = 4;
    m_CurrentStage = static_cast<App::Stage>(stageNum);

    // 初始化邏輯半寬高，確保水泥塊與物件不會因為座標判定而消失
    halfWidth = CoordinateManager::MAP_LOGIC_SIZE / 2.0f;
    halfHeight = CoordinateManager::MAP_LOGIC_SIZE / 2.0f;

    // 根據關卡動態調整跳躍性能：所有關卡都使用 30.0f
    g_MarioTotalJumpTime = 30.0f;

    // --- 1. 全域資源清理與重置 ---
    // 清除舊酒桶、電梯踏板與插銷，確保渲染器不會殘留上一關的物件
    for (auto& barrel : m_Barrels) m_Renderer.RemoveChild(barrel);
    m_Barrels.clear();
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

    // 每次載入關卡前先清除舊UFO
    for (auto& u : m_Ufos) m_Renderer.RemoveChild(u);
    m_Ufos.clear();

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
    if (m_FuelCan) m_FuelCan->SetVisible(false);

    // 清除舊傳送帶 Spawners
    for (auto& s : m_CementSpawners) m_Renderer.RemoveChild(s);
    m_CementSpawners.clear();

    // 清除舊轉軸
    if (m_PivotLeft) { m_Renderer.RemoveChild(m_PivotLeft); m_PivotLeft.reset(); }
    if (m_PivotRight) { m_Renderer.RemoveChild(m_PivotRight); m_PivotRight.reset(); }
    if (m_PivotTopLeft) { m_Renderer.RemoveChild(m_PivotTopLeft); m_PivotTopLeft.reset(); }
    if (m_PivotTopRight) { m_Renderer.RemoveChild(m_PivotTopRight); m_PivotTopRight.reset(); }
    if (m_PivotMidLeft) { m_Renderer.RemoveChild(m_PivotMidLeft); m_PivotMidLeft.reset(); }
    if (m_PivotMidRight) { m_Renderer.RemoveChild(m_PivotMidRight); m_PivotMidRight.reset(); }

    // 重置通關特效與大金剛狀態 (確保從 Stage 4 勝利後切換或重玩時狀態正確)
    // 重置特殊狀態標記
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
        m_DonkeyKong->SetLevel(level);
    }

    // 重置 Stage 4 特有的邏輯標記 (移至開頭以確保不論從哪一關離開，狀態都乾淨)
    m_ActiveRivetPos = {0.0f, 0.0f};
    m_HasActiveRivet = false;
    m_RivetCount = 0;

    // 清除過場畫面殘留物件，避免在過場中重置遊戲時圖示留在畫面上
    for (auto& icon : m_TransitionIcons) m_Renderer.RemoveChild(icon);
    m_TransitionIcons.clear();
    if (m_TransitionTextObj) m_TransitionTextObj->SetVisible(false);

    m_InTransition = false;
    m_DKFallTimer = 0.0f;
    m_FireballTimerMs = 0.0f;
    m_FireballJumping = false;
    m_JumpOverObstacles.clear();

    // // 重置 Mario 以及其它遊戲角色的狀態與可見性 (移至開頭以統一管理)
    m_Mario->Reset();
    m_Fireball->SetVisible(false);
    m_Fireball2->SetVisible(false);
    m_Fireball3->SetVisible(false);
    m_Fireball4->SetVisible(false);

    // 根據是否為 Rivets 關卡更新火球外觀
    bool isRivetStage = (m_CurrentStage == App::Stage::RIVETS);
    m_Fireball->SetStageStyle(isRivetStage);
    m_Fireball2->SetStageStyle(isRivetStage);
    m_Fireball3->SetStageStyle(isRivetStage);
    m_Fireball4->SetStageStyle(isRivetStage);

    if (m_HammerItem) m_HammerItem->SetVisible(true);
    if (m_HammerItem2) m_HammerItem2->SetVisible(true);

    // 重置 HUD 資訊
    if (m_HUDText) {
        // 修正：不要在這裡 Init，否則會把剩餘生命值洗回 3
        m_HUDText->ResetBonus(5000);
        m_HUDText->SetLevel(m_CurrentLevel); // 更新畫面上的 L=XX 文字
    }
    if (m_OilBarrel) {
        m_Renderer.RemoveChild(m_OilBarrel);
        m_OilBarrel = nullptr;
    }
    if (m_BurningOilBarrel) {
        m_BurningOilBarrel->SetVisible(false);
        m_BurningOilBarrel->Stop();
    }


    // --- 2. 載入地圖資源 ---
    // 必須先載入地圖，後續所有基於地圖縮放（Scale）的計算才會正確
    std::string mapImg, mapTxt;
    if (m_CurrentStage == App::Stage::BARRELS) {
        mapImg = "../Resources/Images/board-barrels.png";
        mapTxt = "../Resources/Maps/Map1.txt";
    } else if (m_CurrentStage == App::Stage::CONVEYORS) {
        mapImg = "../Resources/Images/board-conveyors.png";
        mapTxt = "../Resources/Maps/Map2.txt";
    } else if (m_CurrentStage == App::Stage::ELEVATORS) {
        mapImg = "../Resources/Images/board-elevators.png";
        mapTxt = "../Resources/Maps/Map3.txt";
    } else {
        mapImg = "../Resources/Images/board-rivets.png";
        mapTxt = "../Resources/Maps/Map4.txt";
    }

    // 根據傳入的關卡編號，載入對應的地圖圖片與純文字檔 (MapX.txt)
    // 同時把所有物件 (Mario, 火球, 道具, 大金剛) 移動到該關卡適合的座標
    m_Map->LoadNewMap(mapImg, mapTxt);
    m_DonkeyKong->SetMap(m_Map); // 讓 DK 持有地圖指標

    // --- 3. 定義邏輯座標與行為 (基於 720x720 系統) ---
    // 以下數值皆為 720x720 邏輯空間中的精準座標 (X, Y)
    glm::vec2 dkLogicPos, marioLogicPos;
    float dkMinLogicX = 0.0f, dkMaxLogicX = 0.0f;
    DonkeyKong::Behavior dkBehavior = DonkeyKong::Behavior::STATIONARY_LOOKING;

    if (m_CurrentStage == App::Stage::BARRELS) {
        m_IsFirstBarrel = true;
        dkLogicPos = {140.0f, 165.0f};      // DK 在左上方平台
        marioLogicPos = {100.0f, 665.0f};   // Mario 在左下方起點
        dkBehavior = DonkeyKong::Behavior::STATIONARY_LOOKING;
        dkMinLogicX = 120.0f; dkMaxLogicX = 120.0f;

        if (m_StaticBarrels) {
            m_StaticBarrels->SetVisible(true);
            m_StaticBarrels->SetPosition(CoordinateManager::LogicToEngine({40.0f, 120.0f}));
        }
        // 第一關道具座標
        // m_Fireball->SetPosition(CoordinateManager::LogicToEngine({260.0f, 430.0f}));
        if (m_HammerItem) m_HammerItem->SetPosition(CoordinateManager::LogicToEngine({510.0f, 535.0f}));
        if (m_HammerItem2) m_HammerItem2->SetPosition(CoordinateManager::LogicToEngine({80.0f, 210.0f}));

        m_OilBarrel = std::make_shared<Character>(RESOURCE_DIR"/Images/OilBarrel.png");
        // 2. 設定渲染層級（在背景地圖之上，瑪利歐之下即可）
        m_OilBarrel->SetZIndex(40);

        // 3. 設定縮放（如果需要跟 Mario 保持相同比例，就套用你的 marioScale）
        m_OilBarrel->SetScale({m_Mario->marioScale, m_Mario->marioScale});

        // 4. 設定邏輯位置 (30, 690)，並轉換為引擎座標
        m_OilBarrel->SetPosition(CoordinateManager::LogicToEngine({60.0f, 670.0f}));

        // 5. 設定可見度
        m_OilBarrel->SetVisible(true);

        // 7. 加入渲染器統一繪製
        m_Renderer.AddChild(m_OilBarrel);

        if (m_CurrentLevel > 1) { // add Fireball2
            m_Fireball2->SetPosition(CoordinateManager::LogicToEngine({620.0f, 300.0f}));;
            m_Fireball2->SetVisible(true);
            m_Fireball2->SetState(Fiamma::State::FALLING);
        }

    } else if (m_CurrentStage == App::Stage::CONVEYORS) {
        // 第二關 DK 會左右移動且只會搥胸
        dkLogicPos = {110.0f, 180.0f};
        dkBehavior = DonkeyKong::Behavior::MOVING_CHEST_BEATING;
        dkMinLogicX = 100.0f; dkMaxLogicX = 620.0f; // 巡邏範圍

        marioLogicPos = {50.0f, 665.0f};
        if (m_StaticBarrels) m_StaticBarrels->SetVisible(false);

        // 第二關道具座標

         // 設定左右遮罩，讓水泥塊能漸漸出現/消失
        if (m_LeftMask) {
            m_LeftMask->SetVisible(true);
            m_LeftMask->SetPosition(CoordinateManager::LogicToEngine({-55.0f, 460.0f}));
        }
        if (m_RightMask) {
            m_RightMask->SetVisible(true);
            m_RightMask->SetPosition(CoordinateManager::LogicToEngine({775.0f, 460.0f}));
        }

        // 初始化傳送帶邏輯系統
        m_ConveyorSystem = std::make_shared<ConveyorSystem>();

        // 建立四個 Cement Spawners
        // 1. 定義生成器在 720x720 空間中的「水平邏輯座標」
        // 定義「下層軌道 (s1, s2)」的水平邏輯座標
        float logicX1_left  = 10.0f; //160.0f; // 根據你的地圖，這裡可能需要微調 (例如 150~200 之間)
        float logicX1_right = 710.0f; //540.0f; // 根據你的地圖，這裡可能需要微調 (例如 520~560 之間)

        // 定義「中層軌道 (s3, s4)」的水平邏輯座標 (中層較長，可維持在畫面邊緣)
        float logicX2_left  = 10.0f;
        float logicX2_right = 710.0f;

        // 2. 定義兩條軌道的「垂直邏輯座標」
        // (依據公式：logicY = 360.0f - engineY 反推而得)
        float spawnerY1 = 558.0f; //557.0f; // 對應原本下方軌道(第二層)
        float spawnerY2 = 300.0f; // 對應原本上方軌道(第四層)

        // 3. 建立生成器，並統一透過 LogicToEngine 進行自動座標轉換與縮放
        auto s1 = std::make_shared<CementSpawner>(TileType::CONVEYOR1, CementSpawner::Side::LEFT);
        s1->SetPosition(CoordinateManager::LogicToEngine({logicX1_left, spawnerY1}));

        auto s2 = std::make_shared<CementSpawner>(TileType::CONVEYOR1, CementSpawner::Side::RIGHT);
        s2->SetPosition(CoordinateManager::LogicToEngine({logicX1_right, spawnerY1}));

        auto s3 = std::make_shared<CementSpawner>(TileType::CONVEYOR2, CementSpawner::Side::LEFT);
        s3->SetPosition(CoordinateManager::LogicToEngine({logicX2_left, spawnerY2}));

        auto s4 = std::make_shared<CementSpawner>(TileType::CONVEYOR3, CementSpawner::Side::RIGHT);
        s4->SetPosition(CoordinateManager::LogicToEngine({logicX2_right, spawnerY2}));
        // 4. 塞入容器並加入渲染器
        m_CementSpawners = {s1, s2, s3, s4};
        for(auto& s : m_CementSpawners) m_Renderer.AddChild(s);

        // --- Stage 2 轉軸配置 ---
        glm::vec2 mapScale = m_Map->GetScale();

        // 1. 左側轉軸 (第二層最左邊)
        m_PivotLeft = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot2.png",
            RESOURCE_DIR"/Images/pivot3.png"
        });
        m_PivotLeft->SetScale(mapScale * 2.0f);
        m_PivotLeft->SetZIndex(40);
        //m_PivotLeft->SetPosition({-213.0f * mapScale.x, spawnerY2 - 236});
        //m_PivotLeft->SetPosition({-213.0f * mapScale.x, spawnerY2});
        m_PivotLeft->SetPosition(CoordinateManager::LogicToEngine({logicX1_left, spawnerY1 + 20}));
        m_PivotLeft->SetInterval(500);
        m_PivotLeft->SetLooping(true);
        m_PivotLeft->Play();
        m_Renderer.AddChild(m_PivotLeft);

        // 2. 右側轉軸 (第二層最右邊，翻轉版本)
        m_PivotRight = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot2.png",
            RESOURCE_DIR"/Images/pivot3.png"
        });
        m_PivotRight->SetScale({-mapScale.x * 2.0f, mapScale.y * 2.0f}); // 水平翻轉
        m_PivotRight->SetZIndex(40);
        m_PivotRight->SetPosition(CoordinateManager::LogicToEngine({logicX1_right, spawnerY1 + 20}));
        m_PivotRight->SetInterval(500);
        m_PivotRight->SetLooping(true);
        m_PivotRight->Play();
        m_Renderer.AddChild(m_PivotRight);

        // 3. 左側頂層轉軸 (Donkey 層，放大2倍)
        m_PivotTopLeft = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot3.png",
            RESOURCE_DIR"/Images/pivot2.png"
        });
        m_PivotTopLeft->SetScale(mapScale * 2.0f);
        m_PivotTopLeft->SetZIndex(40);
        m_PivotTopLeft->SetPosition(CoordinateManager::LogicToEngine({logicX1_left, spawnerY2 - 107}));
        m_PivotTopLeft->SetInterval(500);
        m_PivotTopLeft->SetLooping(true);
        m_PivotTopLeft->Play();
        m_Renderer.AddChild(m_PivotTopLeft);

        // 4. 右側頂層轉軸 (Donkey 層，翻轉版本，放大2倍)
        m_PivotTopRight = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot3.png",
            RESOURCE_DIR"/Images/pivot2.png"
        });
        m_PivotTopRight->SetScale({-mapScale.x * 2.0f, mapScale.y * 2.0f}); // 水平翻轉
        m_PivotTopRight->SetZIndex(40);
        m_PivotTopRight->SetPosition(CoordinateManager::LogicToEngine({logicX1_right, spawnerY2 - 107}));
        m_PivotTopRight->SetInterval(500);
        m_PivotTopRight->SetLooping(true);
        m_PivotTopRight->Play();
        m_Renderer.AddChild(m_PivotTopRight);

        // 5. 中間層左側轉軸 (Fuel Can 旁，翻轉版本，放大2倍)
        m_PivotMidLeft = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot3.png",
            RESOURCE_DIR"/Images/pivot2.png"
        });
        m_PivotMidLeft->SetScale({-mapScale.x * 2.0f, mapScale.y * 2.0f}); // 水平翻轉
        m_PivotMidLeft->SetZIndex(40);
        //m_PivotMidLeft->SetPosition({-30.0f * mapScale.x, 33.0f});
        m_PivotMidLeft->SetPosition(CoordinateManager::LogicToEngine({320, spawnerY2 + 22}));
        m_PivotMidLeft->SetInterval(500);
        m_PivotMidLeft->SetLooping(true);
        m_PivotMidLeft->Play();
        m_Renderer.AddChild(m_PivotMidLeft);

        // 6. 中間層右側轉軸 (Fuel Can 旁，不翻轉，放大2倍)
        m_PivotMidRight = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
            RESOURCE_DIR"/Images/pivot1.png",
            RESOURCE_DIR"/Images/pivot3.png",
            RESOURCE_DIR"/Images/pivot2.png"
        });
        m_PivotMidRight->SetScale(mapScale * 2.0f);
        m_PivotMidRight->SetZIndex(40);
        //m_PivotMidRight->SetPosition({30.0f * mapScale.x, 33.0f});
        m_PivotMidRight->SetPosition(CoordinateManager::LogicToEngine({400, spawnerY2 + 22}));
        m_PivotMidRight->SetInterval(500);
        m_PivotMidRight->SetLooping(true);
        m_PivotMidRight->Play();
        m_Renderer.AddChild(m_PivotMidRight);

        // 初始化兩側伸縮梯子 (座標需根據地圖實際梯子位置微調)
        // 1. 定義梯子伸長與縮回的「邏輯 Y 座標」(0~720，越往下數字越大)
        float logicY1 = 232.0f;
        float logicY2 = 280.0f;

        // 重新拿回地圖的基礎縮放比例 (自動包含地圖原本的放大倍率)
        //glm::vec2 mapScale = m_Map->GetScale();

        // 2. 建立梯子，把 mapScale 當作最後一個參數帶入
        auto leftLadder = std::make_shared<MovingLadder>(64.0f, logicY1, logicY2, MovingLadder::Side::LEFT, LadderState::EXTENDED, mapScale);
        auto rightLadder = std::make_shared<MovingLadder>(656.0f, logicY1, logicY2, MovingLadder::Side::RIGHT, LadderState::RETRACTED, mapScale);

        // 3. 加入陣列與渲染器 (不需要再呼叫 SetScale 和 SetZIndex 了，因為建構子已經做好了！)
        m_MovingLadders.push_back(leftLadder);
        m_MovingLadders.push_back(rightLadder);
        m_Renderer.AddChild(leftLadder);
        m_Renderer.AddChild(rightLadder);

        // --- 【新增】Stage 2 道具配置 ---
        // 1. 皮包 (最下面那層)
        auto p2 = std::make_shared<Character>(RESOURCE_DIR"/Images/purse.png");
        p2->SetScale(mapScale * 2.0f);
        p2->SetZIndex(40);
        p2->SetPosition(CoordinateManager::LogicToEngine({410, spawnerY1 + 120}));
        m_Purses.push_back(p2);
        m_Renderer.AddChild(p2);

        // 2. 雨傘 (中間層 y2=55.0f)
        auto u2 = std::make_shared<Character>(RESOURCE_DIR"/Images/umbrella.png");
        u2->SetScale(mapScale * 2.0f);
        u2->SetZIndex(40);
        u2->SetPosition(CoordinateManager::LogicToEngine({660, spawnerY2 + 115}));
        m_Umbrellas.push_back(u2);
        m_Renderer.AddChild(u2);

        // 3. UFO (中間層 y2=55.0f)
        auto ufo2 = std::make_shared<Character>(RESOURCE_DIR"/Images/ufo.png");
        ufo2->SetScale(mapScale * 2.0f);
        ufo2->SetZIndex(40);
        ufo2->SetPosition(CoordinateManager::LogicToEngine({230, spawnerY2 + 115}));   //({-110.0f, spawnerY2 - 100.0f});
        m_Ufos.push_back(ufo2);
        m_Renderer.AddChild(ufo2);

        // 設定中央的 Fuel Can 動畫
        m_FuelCan->SetPosition({0.0f, 20.0f}); // 放置於螢幕中央稍偏上方
        m_FuelCan->SetVisible(true);
        m_FuelCan->SetScale(m_Map->GetScale() * 2.0f);

        //m_Fireball->SetPosition(m_FuelCan->GetPosition() - 200.0f);
        m_Fireball->SetVisible(false); // 初始先隱藏
        m_FireballTimerMs = 1000.0f; // 等待 1 秒後出現
        m_FireballJumping = false;

        //m_Fireball2->SetPosition(m_FuelCan->GetPosition() - 200.0f);
        m_Fireball2->SetVisible(false);
        m_FireballTimerMs2 = 2000.0f; // 等待 2 秒後出現 (比第一個多一秒)
        m_FireballJumping2 = false;

        if (m_HammerItem) m_HammerItem->SetPosition(CoordinateManager::LogicToEngine({360.0f, 480.0f})); //({0.0f, -120.5f});
        if (m_HammerItem2) m_HammerItem2->SetPosition(CoordinateManager::LogicToEngine({50.0f, 350.0f}));

    } else if (m_CurrentStage == App::Stage::ELEVATORS) {
        dkLogicPos = {120.0f, 180.0f};
        marioLogicPos = {50.0f, 665.0f};
        dkBehavior = DonkeyKong::Behavior::MOVING_CHEST_BEATING;
        dkMinLogicX = 120.0f; dkMaxLogicX = 120.0f; // 原地搥胸
        if (m_StaticBarrels) m_StaticBarrels->SetVisible(false);

        // 第三關道具座標
        // 1. 定義電梯的上下極限邊界 (邏輯 Y 座標)
        float logicTopY = 200.0f;
        float logicBotY = 680.0f;
        float logicLeftX = 130.0f;
        float logicRightX = 335.0f;
        float logicSpacing = 160.0f;

        // 轉換為引擎座標
        float engTopY = CoordinateManager::LogicToEngine({0.0f, logicTopY}).y;
        float engBotY = CoordinateManager::LogicToEngine({0.0f, logicBotY}).y;
        glm::vec2 mapScale = m_Map->GetScale();

        // 雨傘道具 (Stage 3)
        auto createUmbrella = [&](float logicX, float logicY) {
            auto u = std::make_shared<Character>(RESOURCE_DIR"/Images/umbrella.png");
            u->SetScale(mapScale * 1.5f);
            u->SetZIndex(40);
            u->SetPosition(CoordinateManager::LogicToEngine({logicX, logicY}));
            m_Umbrellas.push_back(u);
            m_Renderer.AddChild(u);
        };

        // 1. 左邊最上層再往下一層 (DK 層下方)
        createUmbrella(35.0f, 300.0f);
        // 2. 最右邊最上面那層
        createUmbrella(690.0f, 200.0f);

        // 皮包道具 (Stage 3)
        auto createPurse = [&](float logicX, float logicY) {
            auto p = std::make_shared<Character>(RESOURCE_DIR"/Images/purse.png");
            p->SetScale(mapScale * 1.5f);
            p->SetZIndex(40);
            p->SetPosition(CoordinateManager::LogicToEngine({logicX, logicY}));
            m_Purses.push_back(p);
            m_Renderer.AddChild(p);
        };

        // 放在兩條移動電梯中間 (X 約在 -80)，高度設在中間層
        createPurse(245.0f, 550.0f);

        // 建立左側「上升」電梯 (UP)
        for (float currY = logicBotY; currY >= logicTopY; currY -= logicSpacing) {
            auto el = std::make_shared<Elevator>(Elevator::Direction::UP, engBotY, engTopY, 1.0f);
            el->SetScale(mapScale);
            el->SetPosition(CoordinateManager::LogicToEngine({logicLeftX, currY}));
            m_Elevators.push_back(el);
            m_Renderer.AddChild(el);
        }

        // 建立右側「下降」電梯 (DOWN)
        for (float currY = logicTopY; currY <= logicBotY; currY += logicSpacing) {
            auto el = std::make_shared<Elevator>(Elevator::Direction::DOWN, engBotY, engTopY, 1.0f);
            el->SetScale(mapScale);
            el->SetPosition(CoordinateManager::LogicToEngine({logicRightX, currY}));
            m_Elevators.push_back(el);
            m_Renderer.AddChild(el);
        }

        float leftElX = CoordinateManager::LogicToEngine({logicLeftX, 0.0f}).x;
        float rightElX = CoordinateManager::LogicToEngine({logicRightX, 0.0f}).x;

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

        // 傳入精準的邏輯轉換世界座標，並結合高低微調
        createStop(leftElX,  engTopY - 4.0f * mapScale.y, false); // 左上
        createStop(leftElX,  engBotY - 7.0f * mapScale.y, true);  // 左下
        createStop(rightElX, engTopY - 4.0f * mapScale.y, false); // 右上
        createStop(rightElX, engBotY - 7.0f * mapScale.y, true);  // 右下

        m_Fireball->SetPosition(CoordinateManager::LogicToEngine({200.0f, 300.0f}));;
        m_Fireball->SetVisible(true);
        m_Fireball->SetState(Fiamma::State::FALLING);

        m_Fireball2->SetPosition(CoordinateManager::LogicToEngine({600.0f, 300.0f}));;
        m_Fireball2->SetVisible(true);
        m_Fireball2->SetState(Fiamma::State::FALLING);

        // 第三關不需要槌子，將其全部隱藏
        if (m_HammerItem) m_HammerItem->SetVisible(false);
        if (m_HammerItem2) m_HammerItem2->SetVisible(false);

    } else if (m_CurrentStage == App::Stage::RIVETS) {
        dkLogicPos = {360.0f, 180.0f};      // DK 在頂端中心
        marioLogicPos = {50.0f, 665.0f};
        dkBehavior = DonkeyKong::Behavior::MOVING_CHEST_BEATING;
        dkMinLogicX = 360.0f; dkMaxLogicX = 360.0f;
        if (m_StaticBarrels) m_StaticBarrels->SetVisible(false);

        // --- Stage 4 道具配置 (位置與倍率調整) ---
        glm::vec2 mapScale = m_Map->GetScale();

        // 1. 皮包 (下方第一層)
        auto p = std::make_shared<Character>(RESOURCE_DIR"/Images/purse.png");
        p->SetScale(mapScale * 2.0f);
        p->SetZIndex(40);
        p->SetPosition(CoordinateManager::LogicToEngine({410.0f, 680.0f}));
        m_Purses.push_back(p);
        m_Renderer.AddChild(p);

        // 2. 雨傘 (跟 Donkey 同一層最左邊，由 1.5x 改為 2x)
        auto u = std::make_shared<Character>(RESOURCE_DIR"/Images/umbrella.png");
        u->SetScale(mapScale * 2.0f);
        u->SetZIndex(40);
        u->SetPosition(CoordinateManager::LogicToEngine({112.0f, 155.0f}));
        m_Umbrellas.push_back(u);
        m_Renderer.AddChild(u);

        // 3. UFO (第二層最右邊，由 1.5x 改為 2x)
        auto ufo = std::make_shared<Character>(RESOURCE_DIR"/Images/ufo.png");
        ufo->SetScale(mapScale * 2.0f);
        ufo->SetZIndex(40);
        ufo->SetPosition(CoordinateManager::LogicToEngine({653.0f, 540.0f}));
        m_Ufos.push_back(ufo);
        m_Renderer.AddChild(ufo);

        if (m_HammerItem) m_HammerItem->SetPosition(CoordinateManager::LogicToEngine({50.0f, 350.0f}));
        if (m_HammerItem2) m_HammerItem2->SetPosition(CoordinateManager::LogicToEngine({360.0f, 220.0f}));

        m_Fireball->SetPosition(CoordinateManager::LogicToEngine({650.0f, 650.0f}));;
        m_Fireball->SetVisible(true);
        m_Fireball->SetState(Fiamma::State::FALLING);

        m_Fireball2->SetPosition(CoordinateManager::LogicToEngine({250.0f, 350.0f}));;
        m_Fireball2->SetVisible(true);
        m_Fireball2->SetState(Fiamma::State::FALLING);

        m_Fireball3->SetPosition(CoordinateManager::LogicToEngine({650.0f, 450.0f}));
        m_Fireball3->SetVisible(true);
        m_Fireball3->SetState(Fiamma::State::FALLING);

        m_Fireball4->SetPosition(CoordinateManager::LogicToEngine({250.0f, 250.0f}));
        m_Fireball4->SetVisible(true);
        m_Fireball4->SetState(Fiamma::State::FALLING);

        const auto& data = m_Map->GetLevelData();
        for (int y = 0; y < data.GetHeight(); ++y) {
            for (int x = 0; x < data.GetWidth(); ++x) {
                if (data.GetTile(x, y) == TileType::RIVET) {
                    auto rivet = std::make_shared<Character>(RESOURCE_DIR"/Images/rivet.png");
                    // 根據 Tile 大小調整縮放，這裡假設使用與 Mario 類似的縮放倍率
                    rivet->SetScale({m_Mario->marioScale*1.5f, m_Mario->marioScale*1.5f});
                    rivet->SetZIndex(-5); // 放在地圖上方，角色下方

                    glm::vec2 rivetPos = m_Map->GetTileWorldPosition(x, y);
                    rivetPos.y -= 15.0f * CoordinateManager::GetScaleRatio();
                    rivet->SetPosition(rivetPos);
                    LOG_DEBUG("({},{})", x, y);
                    m_RivetVisuals[{x, y}] = rivet;
                    m_Renderer.AddChild(rivet);
                    m_RivetCount++;
                }
            }
        }
    }

    // 公主座標所有關卡皆相同，可以直接放在 if 外面
    if (m_Princess) {
        m_Princess->SetPosition(CoordinateManager::LogicToEngine({340.0f, 50.0f}));
    }

    // --- 4. 統一執行m_Mario, m_DonkeyKong座標轉換與套用 ---
    m_Mario->SetPosition(CoordinateManager::LogicToEngine(marioLogicPos));

    glm::vec2 dkEngineFoot = CoordinateManager::LogicToEngine(dkLogicPos);
    float dkSpawnY = dkEngineFoot.y + (m_DonkeyKong->GetSize().y / 2.0f);
    m_DonkeyKong->SetPosition({dkEngineFoot.x, dkSpawnY});
    m_DonkeyKong->SetBehavior(dkBehavior);

    float engineMinX = CoordinateManager::LogicToEngine({dkMinLogicX, 0.0f}).x;
    float engineMaxX = CoordinateManager::LogicToEngine({dkMaxLogicX, 0.0f}).x;
    m_DonkeyKong->SetMoveBounds(engineMinX, engineMaxX);

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

    // 將地圖實體交給 Mario
    m_Mario->SetMap(m_Map);

    // 建構火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Fireball->SetMap(m_Map);
    m_Renderer.AddChild(m_Fireball);

    // 建構火球2物件
    m_Fireball2 = std::make_shared<Fiamma>();
    m_Fireball2->SetMap(m_Map);
    m_Renderer.AddChild(m_Fireball2);

    m_Fireball3 = std::make_shared<Fiamma>();
    m_Fireball3->SetMap(m_Map);
    m_Renderer.AddChild(m_Fireball3);

    m_Fireball4 = std::make_shared<Fiamma>();
    m_Fireball4->SetMap(m_Map);
    m_Renderer.AddChild(m_Fireball4);

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

    m_OilBarrel = std::make_shared<Character>(RESOURCE_DIR"/Images/OilBarrel.png"); // 請確認你的檔名
    m_OilBarrel->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_OilBarrel->SetZIndex(40);
    m_Renderer.AddChild(m_OilBarrel);

    m_BurningOilBarrel = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
        RESOURCE_DIR"/Images/OilBarrel1.png",
        RESOURCE_DIR"/Images/OilBarrel2.png"
    });
    m_BurningOilBarrel->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_BurningOilBarrel->SetZIndex(41); // 稍微高於靜態油桶一點點或一樣即可
    m_BurningOilBarrel->SetPosition(CoordinateManager::LogicToEngine({60.0f, 658.0f}));
    m_BurningOilBarrel->SetVisible(false); // 一開始先隱藏，等藍色木桶撞到才顯示
    m_BurningOilBarrel->SetLooping(true);
    m_BurningOilBarrel->SetInterval(150);
    m_Renderer.AddChild(m_BurningOilBarrel);

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
    m_BlackCover = std::make_shared<Character>(RESOURCE_DIR"/Images/black_04.png");
    m_BlackCover->SetZIndex(10); // 初始設為較低層級（在地圖 -10 之後，但在角色 50 之前）
    m_BlackCover->SetScale({2.0f, 1.0f}); // 初始化為預設比例，過關時再動態調整
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

    // 初始化 Fuel Can 動畫 (用於 Stage 2)
    m_FuelCan = std::make_shared<AnimatedCharacter>(std::vector<std::string>{
        RESOURCE_DIR"/Images/fuel_can1.png",
        RESOURCE_DIR"/Images/fuel_can2.png"
    });
    m_FuelCan->SetZIndex(45);
    m_FuelCan->SetVisible(false);
    m_FuelCan->SetLooping(true);
    m_FuelCan->SetInterval(200); // 每 200ms 切換一次圖片
    m_FuelCan->Play();

    m_Renderer.AddChild(m_Princess);
    m_Princess->Stop();
    // m_PrincessRefSize = m_Princess->GetSize();
    // if (m_PrincessRefSize.x > 0 && m_PrincessRefSize.y > 0) {
    //     m_Princess->SetPivot(m_PrincessRefSize / 2.0f);
    // }

    // 初始化 Conveyor1,2,3 左右遮罩 (使用與 BlackCover 相同的黑色圖片)
    m_LeftMask = std::make_shared<Character>(RESOURCE_DIR"/Images/black2.png");
    m_LeftMask->SetZIndex(60); // ZIndex 必須高於 CementPan (45) 與 Mario (50)
    m_LeftMask->SetVisible(false);
    m_Renderer.AddChild(m_LeftMask);

    m_RightMask = std::make_shared<Character>(RESOURCE_DIR"/Images/black2.png");
    m_RightMask->SetZIndex(60);
    m_RightMask->SetVisible(false);
    m_Renderer.AddChild(m_RightMask);

    m_GameOverIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/game_over.png");
    m_GameOverIcon->SetZIndex(100);
    m_GameOverIcon->SetScale({3.0f, 3.0f}); // 縮放成適合的大小
    m_GameOverIcon->SetPosition(CoordinateManager::LogicToEngine({360.0f, 360.0f})); // 置中
    m_GameOverIcon->SetVisible(false);
    m_Renderer.AddChild(m_GameOverIcon);

    // 載入當前關卡 (這會負責載入地圖、設定角色的初始位置與重置狀態，也處理 DonkeyKong 給 Mario 的邊界傳遞)
    m_Renderer.AddChild(m_FuelCan);
    m_CurrentLevel = 1;
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

#if 1
    // 將結束程式的檢查移到最上方，確保在 Game Over 狀態下也能觸發
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
#endif

#if 1  //sdbg: 按下 N 鍵切換到下一關測試, 按下 R 鍵 reset
    if (Util::Input::IsKeyDown(Util::Keycode::N)) {
        m_CurrentLevel++;
        if (m_CurrentLevel == 6) m_CurrentLevel = 1;   // temp: max 5-level
        LoadLevel(m_CurrentLevel);
        m_TransitionTimer = 0.0f;
    }
    else if (Util::Input::IsKeyDown(Util::Keycode::R)) {
        LoadLevel(m_CurrentLevel);
        m_TransitionTimer = 0.0f;
    }
    else if (Util::Input::IsKeyDown(Util::Keycode::H)) {
        // 按下 H 鍵重置最高分
        m_HUDText->ResetHighScore();
    }
    else if (Util::Input::IsKeyDown(Util::Keycode::F) && m_CurrentStage == Stage::RIVETS) {
        // Debug: 立即拔掉所有插銷並觸發勝利動畫
        for (auto& pair : m_RivetVisuals) {
            m_Renderer.RemoveChild(pair.second);
            auto [rx, ry] = pair.first;
            glm::vec2 worldPos = m_Map->GetTileWorldPosition(rx, ry);
            m_Map->SetTileAtPosition(worldPos.x, worldPos.y, TileType::EMPTY);
        }
        m_RivetVisuals.clear();
        m_RivetCount = 0;
        m_Mario->Win();
    }

    // 處理 Game Over 狀態下的重啟邏輯
    if (m_IsGameOver) {
        if (Util::Input::IsKeyDown(Util::Keycode::R) || Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
            m_IsGameOver = false;
            m_GameOverIcon->SetVisible(false);
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
        m_BlackCover->SetZIndex(90); // 過場時提升層級，遮住後方的遊戲角色

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
                label->SetPosition({-120.0f, yPos});    // 高度數字在左側

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
            if (m_CurrentLevel == 6) m_CurrentLevel = 1;   // temp: max 5-level
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
        // 當進入勝利狀態的第一幀，將剩餘的 Bonus 點數轉換為總分
        if (m_HUDText && m_HUDText->GetBonus() > 0) {
            m_HUDText->AddScore(m_HUDText->GetBonus());
            m_HUDText->ResetBonus(0); // 將 Bonus 歸零以防在動畫期間重複加分
        }


        // --- 【新增】勝利視覺表現 ---
        if (m_Princess) {
            m_Princess->SetCurrentFrame(3); // 切換到 Princess_win.png
        }
        if (m_Heart) {
            m_Heart->SetVisible(true);
            glm::vec2 pPos = m_Princess->GetPosition();
            // 將心形固定放在公主右側 (約 +25 像素)，稍微偏上方 (+15 像素)
            m_Heart->SetPosition({pPos.x + 25.0f, pPos.y + 15.0f});
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
    }

    // --- Stage 4 特有的通關動畫：DK 掉下去後自動進入過場 ---
    if (m_CurrentStage == Stage::RIVETS && marioState == MarioState::WIN) {
        m_BlackCover->SetVisible(true);
        m_BlackCover->SetScale(m_Map->GetScale() * glm::vec2(2.0f, 2.0f));
        m_BlackCover->SetZIndex(49);
        m_BlackCover->SetPosition(CoordinateManager::LogicToEngine({350.0f, 442.0f}));

        glm::vec2 dkPos = m_DonkeyKong->GetPosition();
        if (m_DonkeyKong->GetBehavior() != DonkeyKong::Behavior::FALLING_STUNNED) {
            // 階段 A：下墜旋轉中
            if (dkPos.y > -180.0f) {
                dkPos.y -= 4.0f;
                m_DonkeyKong->SetPosition(dkPos);

                m_DKFallTimer += dt;
                if (m_DKFallTimer >= 200.0f) {
                    m_DKFallTimer = 0.0f;
                    m_DonkeyKong->SetScale({m_DonkeyKong->GetScale().x, -m_DonkeyKong->GetScale().y});
                }
                m_DonkeyKong->Update();
            } else {
                // 階段 B：到達位置，切換到暈眩狀態
                m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::FALLING_STUNNED);
                // 將暈眩狀態的大金剛放大
                float stunScale = m_Mario->marioScale * 1.2f;
                m_DonkeyKong->SetScale({stunScale, stunScale});
                m_DKFallTimer = 0.0f; // 重置計時器，供接下來的 3 秒使用
            }
        } else {
            // 階段 C：停在原位撥放暈眩動畫 (Donkey_fall1/2) 計時 3 秒
            m_DKFallTimer += dt;
            m_DonkeyKong->Update();

            if (m_DKFallTimer >= 3000.0f) {
                if (!m_InTransition) {
                    m_InTransition = true;
                    m_TransitionTimer = 3000.0f;
                }
            }
        }
    }


    // --- 【新增】死亡後續處理：生命值減少與 Game Over 判定 ---
    if (marioState == MarioState::DEAD && m_Mario->IsDeathAnimationDone()) {
        // 在此時機點存檔
        if (m_HUDText) {
            m_HUDText->SaveHighScore();
        }

        m_HUDText->DecreaseLife();
        if (m_HUDText->GetLives() <= 0) {
            LOG_DEBUG("GAME OVER TRIGGERED");
            // 進入 Game Over 狀態
            m_IsGameOver = true;

            // 1. 顯示 Game Over 圖片。
            m_GameOverIcon->SetVisible(true);
            // 確保位置與縮放正確 (360, 360) 對應螢幕中心
            //m_GameOverIcon->SetPosition(CoordinateManager::LogicToEngine({360.0f, 360.0f}));
            //m_GameOverIcon->SetScale({3.0f, 3.0f});
            //m_GameOverIcon->SetZIndex(100); // 確保顯示在最前方
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
        if (m_CurrentStage == App::Stage::BARRELS) {
            UpdateBarrels(marioState);
            m_Barrels.erase(std::remove_if(m_Barrels.begin(), m_Barrels.end(),
                [this](const std::shared_ptr<Barrel>& b) {
                    if (!b->GetVisibility()) {
                        m_Renderer.RemoveChild(b);
                        return true;
                    }
                    return false;
                }), m_Barrels.end());
        }

        // 更新傳送帶系統與 Spawner
        if (m_CurrentStage == App::Stage::CONVEYORS && m_ConveyorSystem) {
            float dtMs = static_cast<float>(Util::Time::GetDeltaTimeMs());
            m_ConveyorSystem->Update(dtMs);     // 更新傳送帶系統內部計時與方向狀態

            // 檢查 Spawners 是否要生成新的水泥塊
            for (auto& spawner : m_CementSpawners) {
                int dir = m_ConveyorSystem->GetDirection(spawner->GetTargetBelt());
                if (spawner->ShouldSpawn(dtMs, dir)) {
                    LOG_DEBUG("CementPan spawned by Conveyor: {:d}", static_cast<int>(spawner->GetTargetBelt()));
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
        if (m_CurrentStage == App::Stage::CONVEYORS) {
            for (auto& ladder : m_MovingLadders) {
                ladder->Update(static_cast<float>(Util::Time::GetDeltaTimeMs()));
            }
        }

        // 更新電梯位移邏輯
        if (m_CurrentStage == App::Stage::ELEVATORS) {
            for (auto& elevator : m_Elevators) {
                elevator->Update();
            }
        }

        // 在處理 Mario 的移動邏輯之前，先更新 Donkey Kong 的邊界資訊給 Mario
        m_Mario->SetDonkeyKongBounds(m_DonkeyKong->GetPosition(), m_DonkeyKong->GetSize());

        // 3. 處理 Mario 輸入
        MarioState currentState = m_Mario->GetState();
        if ((currentState == MarioState::IDLE || currentState == MarioState::WALKING)
            && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {

            m_Mario->JumpStart();
            m_JumpOverObstacles.clear(); // [修正] 開始跳躍時才清空舊的越過紀錄
            }

        // 1. 先執行移動邏輯 (讓座標更新到這一幀的目標位置)
        if (m_Mario->IsJumping()) {
            m_Mario->Jump();
            CheckJumpScore(); // [新增] 檢查跳躍得分
        }

        // 2. 移動後，再取得最新位置進行地表偵測
        glm::vec2 marioPos = m_Mario->GetPosition();
        glm::vec2 marioSize = m_Mario->GetSize();
        float footY = marioPos.y - (marioSize.y / 2.0f);
        float targetFootY = footY;
        bool foundSurface = false;
        glm::vec2 currentGroundTilePos = {0.0f, 0.0f}; // 【新增】儲存當前踩踏地板的中心座標
        const float tileH = m_Map->GetTileHeight();

        // 【新增】取得縮放比例
        float scaleRatio = CoordinateManager::GetScaleRatio();

        // 跳躍中搜尋範圍可以稍微加大，避免高速下落穿透
        const float searchRange = m_Mario->IsJumping() ? tileH * 2.0f : tileH * 1.5f;

        // 採用你發現的邏輯：往下深探 1 像素，確保穩定偵測到當前踩踏的地板
        TileType currentMarioFootTile = m_Map->GetTileAtPosition(marioPos.x, footY - (1.0f * scaleRatio));
        TileType tileCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y); // 用中心點檢查
        TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, footY + (3.0f * scaleRatio));
        bool isInAirState = (m_Mario->GetState() == MarioState::FALLING || m_Mario->GetState() == MarioState::JUMPING);

        // [修正] 地面偵測邏輯：加入對傳送帶 Tile 的判定
        if (!isInAirState && (currentMarioFootTile == TileType::FLOOR ||
            currentMarioFootTile == TileType::RIVET ||
            currentMarioFootTile == TileType::CONVEYOR1 ||
            currentMarioFootTile == TileType::CONVEYOR2 ||
            currentMarioFootTile == TileType::CONVEYOR3)) {
            // 向上找 FLOOR
            for (float dy = 0.0f; dy <= searchRange; dy += 1.0f) {
                TileType tile = m_Map->GetTileAtPosition(marioPos.x, footY + dy + 1.0f);
                // 排除法：只要「不是」實體地板，就代表我們找到了地板的上方表面（空氣層）
                if (tile != TileType::FLOOR &&
                    tile != TileType::CONVEYOR1 &&
                    tile != TileType::CONVEYOR2 &&
                    tile != TileType::CONVEYOR3 &&
                    tile != TileType::RIVET) {

                    targetFootY = footY + dy;
                    foundSurface = true;
                    break;
                }
            }
        } else {
            // 向下找 FLOOR
            // 【修正】防止第一關跳躍時意外吸附到上層平台
            // 只有在第三關（電梯）且瑪利歐正在下墜時，才允許向上 5 像素的緩衝搜尋。
            // 在第一關（斜坡）等其他關卡，搜尋必須從腳底 (0.0) 開始，且嚴格檢查腳踝處不能在地板內。
            float startDy = 0.0f;
            bool isElevatorStage = (m_CurrentStage == Stage::ELEVATORS);
            if (isElevatorStage && m_Mario->GetState() == MarioState::FALLING) {
                startDy = -5.0f * scaleRatio;
            }

            for (float dy = startDy; dy <= searchRange; dy += 1.0f) {
                TileType tileBelowDY = m_Map->GetTileAtPosition(marioPos.x, footY - dy);
                // 關鍵修正：非電梯關卡時，必須確保腳踝處（tileFoot1）是空氣，防止跳躍時頭部穿進上層地板卻判定為落地
                bool canLand = (isElevatorStage && isInAirState) || (tileFoot1 != TileType::FLOOR);

                if (canLand &&
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

        // 【新增：精準吸附】如果找到了表面，將 targetFootY 修正為該 Tile 的物理頂部，防止浮點數誤差導致抖動
        if (foundSurface) {
            auto [gx, gy] = m_Map->GetTileIndexAtPosition(marioPos.x, targetFootY - 1.0f);
            glm::vec2 tileCenterPos = m_Map->GetTileWorldPosition(gx, gy);
            float tileTopY = tileCenterPos.y + (m_Map->GetTileHeight() / 2.0f);
            targetFootY = tileTopY;
        }

        // --- [新增] 動態電梯平台碰撞偵測 ---
        // 如果在靜態地圖上沒找到表面，且正處於電梯關卡，則檢查 Mario 是否站在電梯上
        // 如果 Mario 踩在上升電梯上，我們必須主動把 el->GetSpeed() 對應的位移量加回 m_Mario->SetPosition。
        //     否則，Mario 會停在原地，而電梯會直接「穿過」他的身體往上升，導致他瞬間變回懸空狀態。
        // marioFootY >= elTopY - 10.0f 提供了一個緩衝區，確保 Mario 在下墜過程中只要接近電梯頂部，
        //     就能被正確「吸附」上去，這能提供更流暢的操作感。
        if (!foundSurface && m_CurrentStage == App::Stage::ELEVATORS) {
            for (auto& el : m_Elevators) {
                // 使用 AABB 碰撞初步判斷 Mario 是否觸碰到電梯踏板
                if (el->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
                    float marioFootY = marioPos.y - (marioSize.y / 2.0f);
                    float elTopY = el->GetPosition().y + (el->GetSize().y / 2.0f);

                    // 判定 Mario 必須在踏板上方（允許 10 像素的吸附落差）
                    if (marioFootY >= elTopY - (10.0f * scaleRatio)) {
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
        if (m_CurrentStage == App::Stage::RIVETS) {
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

                    auto rivetKey = std::make_pair(rx, ry);
                    if (m_RivetVisuals.count(rivetKey)) {
                        m_Map->SetTileAtPosition(m_ActiveRivetPos.x, m_ActiveRivetPos.y, TileType::EMPTY);
                        m_Renderer.RemoveChild(m_RivetVisuals[rivetKey]);
                        m_RivetVisuals.erase(rivetKey);
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
            if (foundSurface) {
                // [修正] 只要落地就結算跳躍得分，不再檢查計時器，避免高速平台漏分
                // [新增] 成功落地時才結算跳躍得分
                if (!m_JumpOverObstacles.empty()) {
                    int score = 0;
                    size_t count = m_JumpOverObstacles.size();

                    // 原版獎勵邏輯：1個=100, 2個=300, 3個以上=500
                    if (count == 1) score = 100;
                    else if (count == 2) score = 300;
                    else if (count >= 3) score = 500;

                    m_HUDText->AddScore(score);
                    SpawnPointVisual(m_Mario->GetPosition(), score);
                    m_JumpOverObstacles.clear(); // 結算後清空
                }

                // 偵錯日誌：可以觀察落地時的 Y 座標落差
                // LOG_DEBUG("Jump Landing Attempt: footY={}, targetFootY={}", footY, targetFootY);
                m_Mario->Land(targetFootY);
            }
            else if (m_Mario->GetJumpTimer() >= g_MarioTotalJumpTime * 0.83f) { // 原 25/30 比例
                m_Mario->Fall();
            }
        }
        else {
            // 取得 Mario 中心點的格子類型，用於輔助判定穿越厚地板的爬行
            //TileType tileAtCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y);

            TileType tileBelow = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - (33.0f * scaleRatio));
            //TileType tileFoot1 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) + 3.0f);
            TileType tileFoot2 = m_Map->GetTileAtPosition(marioPos.x, marioPos.y - (marioSize.y / 2.0f) - (1.0f * scaleRatio));
            //TileType tileCenter = m_Map->GetTileAtPosition(marioPos.x, marioPos.y); // 用中心點檢查

            // [落地檢查]
            // 如果在墜落狀態中偵測到表面，則恢復為靜止狀態。
            if (foundSurface && m_Mario->GetState() == MarioState::FALLING) {
                // [修正] 墜落落地也要結算分數 (例如從平台跳下越過木桶)
                if (!m_JumpOverObstacles.empty()) {
                    int score = 0;
                    size_t count = m_JumpOverObstacles.size();
                    if (count == 1) score = 100;
                    else if (count == 2) score = 300;
                    else if (count >= 3) score = 500;

                    m_HUDText->AddScore(score);
                    SpawnPointVisual(m_Mario->GetPosition(), score);
                    m_JumpOverObstacles.clear();
                }
                m_Mario->IDLE();
                // 同步位置
                m_Mario->SetPosition({marioPos.x, targetFootY + (marioSize.y / 2.0f)});
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

        // 4. 更新火球移動邏輯
        if (m_CurrentStage == App::Stage::CONVEYORS) {
            if (!m_Fireball->GetVisibility() && m_FireballTimerMs > 0.0f) {
                m_FireballTimerMs -= static_cast<float>(Util::Time::GetDeltaTimeMs());
                if (m_FireballTimerMs <= 0.0f) {
                    m_Fireball->SetVisible(true);
                    // 設定火球從油桶的位置彈出
                    glm::vec2 fuelPos = m_FuelCan->GetPosition();
                    m_Fireball->SetPosition({fuelPos.x, fuelPos.y + 30.0f});
                    m_Fireball->SetState(Fiamma::State::FALLING); //
                    m_FireballJumping = true;      // 啟用跳躍狀態
                    m_FireballVelocityY = 180.0f;  // 降低初速度以縮短跳躍高度
                    m_FireballVelocityX = (std::rand() % 2 == 0 ? 50.0f : -50.0f); // 隨機向左或向右跳
                }
            }

            if (!m_Fireball2->GetVisibility() && m_FireballTimerMs2 > 0.0f) {
                m_FireballTimerMs2 -= static_cast<float>(Util::Time::GetDeltaTimeMs());
                if (m_FireballTimerMs2 <= 0.0f) {
                    m_Fireball2->SetVisible(true);
                    // 設定火球從油桶的位置彈出
                    glm::vec2 fuelPos = m_FuelCan->GetPosition();
                    m_Fireball2->SetPosition({fuelPos.x, fuelPos.y + 30.0f});
                    m_Fireball2->SetState(Fiamma::State::FALLING);
                    m_FireballJumping2 = true;      // 啟用跳躍狀態
                    m_FireballVelocityY2 = 180.0f;  // 降低初速度以縮短跳躍高度
                    m_FireballVelocityX2 = (std::rand() % 2 == 0 ? 50.0f : -50.0f); // 隨機向左或向右跳
                }
            }
        }

        // 更新火球移動邏輯
        auto fireballsToUpdate = {m_Fireball, m_Fireball2, m_Fireball3, m_Fireball4};

        if (m_CurrentStage == App::Stage::CONVEYORS) {
            // Custom jumping logic for m_Fireball
            if (m_Fireball->GetVisibility()) {
                if (m_FireballJumping) {
                    float dtSec = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
                    glm::vec2 fPos = m_Fireball->GetPosition();

                    fPos.y += m_FireballVelocityY * dtSec;
                    m_FireballVelocityY -= 300.0f * dtSec;
                    fPos.x += m_FireballVelocityX * dtSec;

                    m_Fireball->SetPosition(fPos);

                    float footY = fPos.y - (m_Fireball->GetSize().y / 2.0f);
                    if (m_FireballVelocityY < 0 && m_Map->GetTileAtPosition(fPos.x, footY - 2.0f) == TileType::FLOOR) {
                        m_FireballJumping = false;
                    }
                } else {
                    m_Fireball->Update(); // Use Fiamma's default update if not jumping
                }
            }

            // Custom jumping logic for m_Fireball2
            if (m_Fireball2->GetVisibility()) {
                if (m_FireballJumping2) {
                    float dtSec = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
                    glm::vec2 fPos = m_Fireball2->GetPosition();

                    fPos.y += m_FireballVelocityY2 * dtSec;
                    m_FireballVelocityY2 -= 300.0f * dtSec;
                    fPos.x += m_FireballVelocityX2 * dtSec;

                    m_Fireball2->SetPosition(fPos);

                    float footY = fPos.y - (m_Fireball2->GetSize().y / 2.0f);
                    if (m_FireballVelocityY2 < 0 && m_Map->GetTileAtPosition(fPos.x, footY - 2.0f) == TileType::FLOOR) {
                        m_FireballJumping2 = false;
                    }
                } else {
                    m_Fireball2->Update(); // Use Fiamma's default update if not jumping
                }
            }
        } else { // For stages other than CONVEYORS, or if not in custom jumping state
            // Update all fireballs using their default Fiamma::Update() logic
            for (auto& fireball : fireballsToUpdate) {
                if (fireball->GetVisibility()) {
                    fireball->Update();
                }
            }
        }

        // 5. 碰撞偵測：Mario 與火球
        auto fireballs = fireballsToUpdate; // Use the same list for collision
        for (auto& fireball : fireballs) {
            if (fireball->GetVisibility()) {
                glm::vec2 marioSize = m_Mario->GetSize();
                if (m_Mario->GetState() == MarioState::HAMMERING) {
                    marioSize *= 1.8f;
                } else if (m_Mario->GetState() == MarioState::JUMPING) {
                    marioSize.y *= 0.7f;
                }

                const auto firePos = fireball->GetPosition();
                const auto fireSize = fireball->GetSize();
                const auto marioPos = m_Mario->GetPosition();
                if (std::abs(firePos.x - marioPos.x) < (fireSize.x + marioSize.x) / 2.0f &&
                    std::abs(firePos.y - marioPos.y) < (fireSize.y + marioSize.y) / 2.0f) {
                    if (m_Mario->GetState() == MarioState::HAMMERING) {
                        TriggerSmash(fireball->GetPosition(), 800);
                        fireball->SetVisible(false); // 擊碎火球
                    } else {
                        m_Mario->Dead();
                    }
                }
            }
        }

        // 5.1 碰撞偵測：Mario 與 Fuel Can (僅限第二關)
        if (m_CurrentStage == Stage::CONVEYORS && m_FuelCan && m_FuelCan->GetVisibility()) {
            const auto fuelPos = m_FuelCan->GetPosition();
            const auto fuelSize = m_FuelCan->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(fuelPos.x - marioPos.x) < (fuelSize.x + marioSize.x) / 2.0f &&
                std::abs(fuelPos.y - marioPos.y) < (fuelSize.y + marioSize.y) / 2.0f) {
                m_Mario->Dead();
            }
        }

        // 6. 道具偵測：撿起槌子
        MarioState s = m_Mario->GetState();
        bool canPickUp = (s == MarioState::JUMPING || s == MarioState::FALLING);
        if (m_HammerItem && m_HammerItem->GetVisibility() && canPickUp) {
            const auto itemPos = m_HammerItem->GetPosition();
            const auto itemSize = m_HammerItem->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(itemPos.x - marioPos.x) < (itemSize.x + marioSize.x) / 2.0f &&
                std::abs(itemPos.y - marioPos.y) < (itemSize.y + marioSize.y) / 2.0f) {
                m_Mario->WaitForHammer();
                m_HammerItem->SetVisible(false);
            }
        }
        if (m_HammerItem2 && m_HammerItem2->GetVisibility() && canPickUp) {
            const auto itemPos = m_HammerItem2->GetPosition();
            const auto itemSize = m_HammerItem2->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(itemPos.x - marioPos.x) < (itemSize.x + marioSize.x) / 2.0f &&
                std::abs(itemPos.y - marioPos.y) < (itemSize.y + marioSize.y) / 2.0f) {
                m_Mario->WaitForHammer();
                m_HammerItem2->SetVisible(false);
            }
        }

        // 6.2 道具偵測：雨傘 (Umbrella)
        for (auto it = m_Umbrellas.begin(); it != m_Umbrellas.end(); ) {
            const auto itemPos = (*it)->GetPosition();
            const auto itemSize = (*it)->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(itemPos.x - marioPos.x) < (itemSize.x + marioSize.x) / 2.0f &&
                std::abs(itemPos.y - marioPos.y) < (itemSize.y + marioSize.y) / 2.0f) {
                glm::vec2 pos = (*it)->GetPosition();
                int points = (m_CurrentStage == Stage::CONVEYORS) ? 800 : 300;
                m_HUDText->AddScore(points);
                m_Renderer.RemoveChild(*it);
                it = m_Umbrellas.erase(it);
                SpawnPointVisual(pos, points);
                LOG_DEBUG("Mario picked up an umbrella! +{} points.", points);
            } else {
                ++it;
            }
        }

        // 6.3 道具偵測：皮包 (Purse)
        for (auto it = m_Purses.begin(); it != m_Purses.end(); ) {
            const auto itemPos = (*it)->GetPosition();
            const auto itemSize = (*it)->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(itemPos.x - marioPos.x) < (itemSize.x + marioSize.x) / 2.0f &&
                std::abs(itemPos.y - marioPos.y) < (itemSize.y + marioSize.y) / 2.0f) {
                glm::vec2 pos = (*it)->GetPosition();
                // 第二關為 800 分，其餘 300 分
                int points = (m_CurrentStage == Stage::CONVEYORS) ? 800 : 300;
                m_HUDText->AddScore(points);
                m_Renderer.RemoveChild(*it);
                it = m_Purses.erase(it);
                SpawnPointVisual(pos, points);
                LOG_DEBUG("Mario picked up a purse! +{} points.", points);
            } else {
                ++it;
            }
        }

        // 6.4 道具偵測：UFO
        for (auto it = m_Ufos.begin(); it != m_Ufos.end(); ) {
            const auto itemPos = (*it)->GetPosition();
            const auto itemSize = (*it)->GetSize();
            const auto marioPos = m_Mario->GetPosition();
            const auto marioSize = m_Mario->GetSize();
            if (std::abs(itemPos.x - marioPos.x) < (itemSize.x + marioSize.x) / 2.0f &&
                std::abs(itemPos.y - marioPos.y) < (itemSize.y + marioSize.y) / 2.0f) {
                glm::vec2 pos = (*it)->GetPosition();
                int points = (m_CurrentStage == Stage::CONVEYORS) ? 800 : 100; // 預設 UFO 分數
                if (m_CurrentStage == Stage::RIVETS) points = 300; // 原本 Stage 4 的設定
                m_HUDText->AddScore(points);
                m_Renderer.RemoveChild(*it);
                it = m_Ufos.erase(it);
                SpawnPointVisual(pos, points);
                LOG_DEBUG("Mario picked up a UFO! +{} points.", points);
            } else {
                ++it;
            }
        }

        // 6.5 碰撞偵測：Mario 與公主 (m_Princess)
        if (m_Princess && m_Princess->GetVisibility()) { // 只有當公主可見時才進行碰撞檢查
            const auto marioPos = m_Mario->GetPosition();

            // 【新增】Stage 2 (L=02) 勝利條件：Mario 站上與 Donkey Kong 同一層
            if (m_CurrentStage == Stage::CONVEYORS) {
                const auto dkPos = m_DonkeyKong->GetPosition();
                // 只有在非跳躍/墜落狀態下，達到高度才算贏
                if (marioState != MarioState::JUMPING && marioState != MarioState::FALLING &&
                    marioPos.y >= dkPos.y - 24.5f) {
                    m_Mario->Win();
                }
            }
            // 【新增】Stage 3 (L=03) 勝利條件：Mario 必須到達與公主相同的高度平台
            else if (m_CurrentStage == Stage::ELEVATORS) {
                const auto princessPos = m_Princess->GetPosition();
                const auto marioSize = m_Mario->GetSize();
                const auto princessSize = m_Princess->GetSize();

                // 取得公主與馬力歐的腳底 Engine Y 座標
                float princessFeetY = princessPos.y - (princessSize.y / 2.0f);
                float marioFeetY = marioPos.y - (marioSize.y / 2.0f);

                // 門檻放寬：只要馬力歐腳底距離公主平台 25 像素以內（約半身高度），在爬梯中也能觸發
                if (marioState != MarioState::JUMPING && marioState != MarioState::FALLING &&
                    marioFeetY >= princessFeetY &&
                    std::abs(marioPos.x - princessPos.x) < (marioSize.x + princessSize.x) / 2.0f &&
                    std::abs(marioPos.y - princessPos.y) < (marioSize.y + princessSize.y) / 2.0f) {
                    m_Mario->Win();
                }
            }
            // 其他關卡的勝利條件：Mario 碰撞公主
            else {
                const auto marioSize = m_Mario->GetSize();
                const auto princessPos = m_Princess->GetPosition();
                const auto princessSize = m_Princess->GetSize();
                const auto marioHalfSize = marioSize / 2.0f;
                const auto princessHalfSize = princessSize / 2.0f;

                // 【修正】針對第一關增加高度門檻限制，防止在爬最後一段梯子途中就提早觸發過關
                bool heightOk = true;
                if (m_CurrentStage == Stage::BARRELS) {
                    float princessFeetY = princessPos.y - princessHalfSize.y;
                    float marioFeetY = marioPos.y - marioHalfSize.y;

                    // 只要不是在跳躍或墜落，且爬到接近頂端（腳底在平台下 25 像素內）即可觸發
                    heightOk = (marioFeetY >= princessFeetY - 25.0f) &&
                               (marioState != MarioState::JUMPING && marioState != MarioState::FALLING);
                }

                if (heightOk &&
                    std::abs(marioPos.x - princessPos.x) < (marioHalfSize.x + princessHalfSize.x) &&
                    std::abs(marioPos.y - princessPos.y) < (marioHalfSize.y + princessHalfSize.y)) {
                    m_Mario->Win();
                }
            }
        }

        // 7. 更新 HUD (包含 Bonus 時間倒數)
        m_HUDText->Update(Util::Time::GetDeltaTimeMs());
    }

END_OF_LOGIC:
    // 即使遊戲結束，Mario 的動畫更新 (例如 Win 動畫) 與 Renderer 仍需持續運行
    m_Mario->Update();
    m_Renderer.Update();
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
