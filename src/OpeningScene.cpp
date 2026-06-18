#include "OpeningScene.hpp"
#include "CoordinateManager.hpp"
#include "config.hpp" // 需要取得 RESOURCE_DIR
#include <glm/gtc/constants.hpp>

OpeningScene::OpeningScene(std::shared_ptr<DonkeyKong> dk, 
                           std::shared_ptr<Map> map, 
                           std::shared_ptr<Mario> mario, 
                           std::shared_ptr<AnimatedCharacter> princess)
    : m_DonkeyKong(dk), m_Map(map), m_Mario(mario), m_Princess(princess) {}

void OpeningScene::Start() {
    m_IsFinished = false;
    m_Phase = 0;
    m_Timer = 0.0f;
    m_BounceCount = 0;
    m_LogicY = 697.0f; // 從 Y=600 開始往上爬

    // 設定大金剛狀態與大小
    m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::OPENING_CLIMBING);
    m_DonkeyKong->SetScale({m_Mario->marioScale, m_Mario->marioScale});

    // 把瑪利歐藏到畫面外，並隱藏公主
    m_Mario->SetPosition({-1000.0f, -1000.0f});
    if (m_Princess) m_Princess->SetVisible(false);

    // 載入直立的地圖背景
    m_Map->LoadNewMap(RESOURCE_DIR"/Images/intro-1.png", RESOURCE_DIR"/Maps/Map1.txt");

    // 將大金剛放在底部中心點 (400, 600)
    glm::vec2 dkEngineFoot = CoordinateManager::LogicToEngine({400.0f, m_LogicY});
    float dkSpawnY = dkEngineFoot.y + (m_DonkeyKong->GetSize().y / 2.0f);
    m_DonkeyKong->SetPosition({dkEngineFoot.x, dkSpawnY});
}

void OpeningScene::Update(float dt) {
    if (m_IsFinished) return;

    if (m_Phase == 0) {


        // 階段 0：往上爬，每次固定爬 20 像素
        m_Timer += dt;
        if (m_Timer >= 150.0f) { // 每 150 毫秒爬一步 (數字可微調)
            m_Timer = 0.0f;
            m_LogicY -= 23.0f;   // 每次上移 20 邏輯像素

            // 根據目前高度，讓抱公主的圖片（Index 8 和 9）交替輪播
            int currentFrame = (static_cast<int>(m_LogicY) / 22) % 2 == 0 ? 8 : 9;
            m_DonkeyKong->SetCurrentFrame(currentFrame);

            // 爬到預想位置 Y=100
            if (m_LogicY <= 168.0f) {
                m_LogicY = 168.0f;
                m_Phase = 1;
                m_Timer = 0.0f;
            }

            // 更新實際引擎座標
            glm::vec2 dkEngineFoot = CoordinateManager::LogicToEngine({410.0f, m_LogicY});
            float dkSpawnY = dkEngineFoot.y + (m_DonkeyKong->GetSize().y / 2.0f);
            m_DonkeyKong->SetPosition({dkEngineFoot.x, dkSpawnY});
        }
    }
    else if (m_Phase == 1) {
        // 階段 1：抵達頂部，變成 DKGrin，換成 intro-2
        if (m_Timer == 0.0f) {
            m_Map->LoadNewMap(RESOURCE_DIR"/Images/intro-2.png", RESOURCE_DIR"/Maps/Map1.txt");

            // 停止動畫，並強制設定為剛剛新增的第 11 張圖 (DKGrin.png)
            m_DonkeyKong->SetBehavior(DonkeyKong::Behavior::STATIONARY_LOOKING);
            m_DonkeyKong->Stop();
            m_DonkeyKong->SetCurrentFrame(11);

            // (可選) 讓公主站在旁邊
            if (m_Princess) {
                m_Princess->SetPosition(CoordinateManager::LogicToEngine({360.0f, 40.0f}));
                m_Princess->SetVisible(true);
            }
        }

        m_Timer += dt;
        if (m_Timer >= 800.0f) { // 讓玩家看 0.8 秒的得逞笑臉
            m_Phase = 2;
            m_Timer = 0.0f;
            m_BounceCount = 0;
        }
    }
    else if (m_Phase == 2) {
        // 階段 2：「一蹬一蹬」往左跳 (載入 intro-3 ~ intro-7)
        float bounceDuration = 400.0f; // 每次彈跳花費 0.4 秒
        float bounceHeight = 35.0f;    // 彈跳的拋物線高度
        float bounceDistX = 52.0f;     // 每次彈跳向左移動 52 像素 (跳 5 次後剛好移到 140 左右)

        if (m_Timer == 0.0f) {
            // 起跳瞬間改變背景 (從 intro-3 開始)
            int introIndex = 3 + m_BounceCount;
            if (introIndex <= 8) {
                m_Map->LoadNewMap(RESOURCE_DIR"/Images/intro-" + std::to_string(introIndex) + ".png", RESOURCE_DIR"/Maps/Map1.txt");
            }
        }

        m_Timer += dt;
        float progress = m_Timer / bounceDuration;
        if (progress >= 1.0f) progress = 1.0f;

        // 【一蹬一蹬的數學核心：Sine 拋物線跳躍】
        // X 軸線性往左平移
        float currentX = 400.0f - (m_BounceCount * bounceDistX) - (bounceDistX * progress);

        // Y 軸利用 sin(0 到 PI) 呈現完美的拋物線
        float currentY = 168.0f - std::sin(progress * glm::pi<float>()) * bounceHeight;

        glm::vec2 dkEngineFoot = CoordinateManager::LogicToEngine({currentX, currentY});
        float dkSpawnY = dkEngineFoot.y + (m_DonkeyKong->GetSize().y / 2.0f);
        m_DonkeyKong->SetPosition({dkEngineFoot.x, dkSpawnY});

        if (progress >= 1.0f) {
            m_Timer = 0.0f; // 重置計時器準備下一次彈跳
            m_BounceCount++;

            // 跳了 5 次後 (intro-7 播完)，進入收尾
            if (m_BounceCount >= 5) {
                m_Map->LoadNewMap(RESOURCE_DIR"/Images/intro-8.png", RESOURCE_DIR"/Maps/Map1.txt");
                m_Phase = 3;
                m_Timer = 0.0f;
            }
        }
    }
    else if (m_Phase == 3) {
        // 階段 3：動畫結束，稍等 0.5 秒後通知 App 可以開始遊戲了
        m_Timer += dt;
        if (m_Timer >= 500.0f) {
            m_IsFinished = true;
        }
    }
}