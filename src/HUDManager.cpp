#include "HUDManager.hpp"
#include <iomanip>
#include <sstream>
#include <fstream>
#include "Character.hpp"
#include "config.hpp"
#include "CoordinateManager.hpp"
#include "Util/Logger.hpp"

static std::string FormatInt(int score, int width) {
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << score;
    return ss.str();
}

static const float HIGH_SCORE_DISPLAY_DURATION = 3000.0f; // 顯示時長 (3秒)

HUDManager::HUDManager() {
    // --- 這裡只設定一次 ---
    // 初始化分數，確保變數有初始值
    currentScore = 0;
    m_ExtraLifeAwarded = false;

    // [修正] 歷史最高分應在物件建構時從檔案載入一次
    // 這樣可以確保程式開啟時即讀取到上次存檔的紀錄
    std::ifstream file(RESOURCE_DIR "/highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        highScore = 0;
    }

    const std::string fontPath = RESOURCE_DIR"/Fonts/PressStart2P-Regular.ttf";
    //const int fontSize = 16;
    const Util::Color white = Util::Color(255, 255, 255, 255);
    const Util::Color blue = Util::Color(0, 0, 255, 255);

    // Util::Text constructor requires: font, size, text, color, visibility
    scoreText = std::make_shared<Util::Text>(fontPath, 16, FormatInt(currentScore, 6), white);
    scoreObject = std::make_shared<Util::GameObject>();
    scoreObject->SetDrawable(scoreText);
    scoreObject->SetZIndex(100);  // HUD 應設為較高的 ZIndex 以顯示在最前方
    scoreObject->m_Transform.translation = CoordinateManager::LogicToEngine({-60.0f, 50.0f});
    scoreObject->SetVisible(true);

    highScoreText = std::make_shared<Util::Text>(fontPath, 16, FormatInt(highScore, 6), white);
    highScoreObject = std::make_shared<Util::GameObject>();
    highScoreObject->SetDrawable(highScoreText);
    highScoreObject->SetZIndex(100);
    // 高分文字位置 (對應原先的 -halfWidth + 100.0f, halfHeight - 25.0f)
    highScoreObject->m_Transform.translation = CoordinateManager::LogicToEngine({-60.0f, 25.0f});
    highScoreObject->SetVisible(true);

    levelText = std::make_shared<Util::Text>(fontPath, 16, "L=" + FormatInt(level,2), blue);
    levelObject = std::make_shared<Util::GameObject>();
    levelObject->SetDrawable(levelText);
    levelObject->SetZIndex(100);
    // 關卡文字位置 (對應原先的 halfWidth - 75.0f, halfHeight - 50.0f)
    levelObject->m_Transform.translation = CoordinateManager::LogicToEngine({770.0f, 65.0f});
    levelObject->SetVisible(true);

    bonusText = std::make_shared<Util::Text>(fontPath, 16, std::to_string(bonusTime), white);
    bonusObject = std::make_shared<Util::GameObject>();
    bonusObject->SetDrawable(bonusText);
    bonusObject->SetZIndex(100);
    // Bonus 文字位置 (對應原先的 halfWidth - 175.0f, halfHeight - 50.0f)
    bonusObject->m_Transform.translation = CoordinateManager::LogicToEngine({630.0f, 65.0f});
    bonusObject->SetVisible(true);

    // 在 currentScore 下方放置 3 個橫向排列的生命圖示
    for (int i = 0; i < 3; ++i) {
        auto lifeIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk0.png");
        lifeIcon->SetZIndex(100);
        lifeIcon->SetScale({1.5f, 1.5f}); // 放大生命圖示比例

        // 絕對邏輯座標 X=-110 起始，增加間距至 30 以對應放大的圖示；Y 統一為 75
        float logicX = -110.0f + (i * 30.0f);
        float logicY = 75.0f;
        lifeIcon->SetPosition(CoordinateManager::LogicToEngine({logicX, logicY}));

        lifeObjects.push_back(lifeIcon);
    }

    // --- 初始化 "HIGH SCORE" 圖片圖標 ---
    m_HighScoreNotifyIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/high_score.png");
    m_HighScoreNotifyIcon->SetZIndex(100); // 提高層級，確保在所有文字上方

    // 【參考 PointVisual】：大幅增加縮放倍率，使其在畫面上清晰可見
    m_HighScoreNotifyIcon->SetScale({2.0f, 2.0f});

    // 初始座標先設在分數旁邊
    m_HighScoreNotifyIcon->SetPosition(CoordinateManager::LogicToEngine({80.0f, 28.0f}));
    m_HighScoreNotifyIcon->SetVisible(false);
}

void HUDManager::Init() {
    currentScore = 0;
    scoreText->SetText(FormatInt(currentScore, 6));

    m_ExtraLifeAwarded = false; // 重置獎勵標記

    // 關鍵：重置時只更新文字顯示，但不將 highScore 歸零
    highScoreText->SetText(FormatInt(highScore, 6));

    // 【新增】初始化生命值顯示
    mLives = 3;
    for (int i = 0; i < (int)lifeObjects.size(); ++i) {
        lifeObjects[i]->SetVisible(i < 3); // 確保只顯示初始的 3 條命
    }

    ResetBonus(5000);

    // 重置提醒狀態
    m_IsShowingHighScoreIcon = false;
    m_HighScoreIconTimer = 0.0f;
    if (m_HighScoreNotifyIcon) m_HighScoreNotifyIcon->SetVisible(false);
}

void HUDManager::AddToRenderer(Util::Renderer& renderer) {
    m_Renderer = &renderer; // 取得並儲存渲染器，供後續 AddLife 使用
    // 將包裝後的 GameObject 加入渲染器，而非 Drawable 本身
    renderer.AddChild(scoreObject);
    renderer.AddChild(highScoreObject);
    renderer.AddChild(levelObject);
    renderer.AddChild(bonusObject);

    // 將生命值圖示加入渲染器
    for (auto& life : lifeObjects) {
        renderer.AddChild(life);
    }

    // 加入提醒文字到渲染器
    renderer.AddChild(m_HighScoreNotifyIcon);
}

void HUDManager::Update(float deltaTime) {
    // scoreText->SetText(FormatInt(currentScore, 6));   // 移除這行，避免每幀重複設定

    // 更新 Bonus 倒數
    bonusTimer += deltaTime;
    if (bonusTimer >= 2000.0f && bonusTime > 00.0f) { // 每 2 秒 bonus 扣 100 分
        bonusTime -= 100;
        bonusTimer = 0.0f;
        bonusText->SetText(FormatInt(bonusTime, 4));

        //TODO: if (bonusTime==0) 玩家死亡
    }

    // 處理高分紀錄顯示計時邏輯
    if (m_IsShowingHighScoreIcon) {
        m_HighScoreIconTimer += deltaTime;
        if (m_HighScoreIconTimer >= HIGH_SCORE_DISPLAY_DURATION) {
            m_IsShowingHighScoreIcon = false;
            m_HighScoreIconTimer = 0.0f;
            m_HighScoreNotifyIcon->SetVisible(false); // 時間到，隱藏圖示
        }
    }
}

void HUDManager::AddScore(int points) {
    int oldScore = currentScore;
    currentScore += points;
    scoreText->SetText(FormatInt(currentScore, 6));

    // --- 獎勵生命 (1UP) 檢查 ---
    if (!m_ExtraLifeAwarded && currentScore >= 7000) {
        m_ExtraLifeAwarded = true;
        AddLife();
        LOG_INFO("Extra Life Awarded! Score reached 7000.");
    }

    if (currentScore > highScore) {
        // 當分數超越歷史最高紀錄時觸發（若持續得分，則重置三秒計時，讓圖標維持顯示）
        if (oldScore <= highScore || m_IsShowingHighScoreIcon) {
            LOG_INFO("New High Score Triggered! Current: {} High: {}", currentScore, highScore);
            m_IsShowingHighScoreIcon = true;
            m_HighScoreIconTimer = 0.0f; // 重置計時器，確保顯示足夠三秒

            // 【動態對齊】：確保圖片位置跟隨最高分數文字的位置，並向右偏移
            glm::vec2 scorePos = highScoreObject->m_Transform.translation;
            m_HighScoreNotifyIcon->SetPosition({scorePos.x + 130.0f, scorePos.y});

            m_HighScoreNotifyIcon->SetVisible(true);
        }
        highScore = currentScore;
        highScoreText->SetText(FormatInt(highScore, 6));
        // [修正] 每當超越最高分時立即存檔，避免玩家直接關閉程式導致紀錄沒存到
        SaveHighScore();
    }
}

void HUDManager::SaveHighScore() {
    std::ofstream file(RESOURCE_DIR "/highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
        LOG_INFO("High score saved: {}", highScore);
    }
}

void HUDManager::ResetHighScore() {
    highScore = 0;
    highScoreText->SetText(FormatInt(highScore, 6));
    SaveHighScore();
    LOG_INFO("High Score has been reset to 0.");
}

void HUDManager::ResetBonus(int amount) {
    bonusTime = amount;
    bonusTimer = 0.0f;
    bonusText->SetText(FormatInt(bonusTime, 4));
}

void HUDManager::SetLevel(int level) {
    this->level = level;
    levelText->SetText("L=" + FormatInt(level, 2));
}

void HUDManager::AddLife() {
    mLives++;

    // 如果目前生命數超過了現有的圖示物件數量，則需要建立新的
    if (mLives > (int)lifeObjects.size()) {
        auto lifeIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk0.png");
        lifeIcon->SetZIndex(100);
        lifeIcon->SetScale({1.5f, 1.5f});

        // 計算新圖示的位置 (依序向右排列)
        float logicX = -110.0f + ((mLives - 1) * 30.0f);
        float logicY = 75.0f;
        lifeIcon->SetPosition(CoordinateManager::LogicToEngine({logicX, logicY}));

        lifeObjects.push_back(lifeIcon);
        if (m_Renderer) m_Renderer->AddChild(lifeIcon);
    } else {
        // 否則只需將隱藏的圖示重新顯示
        lifeObjects[mLives - 1]->SetVisible(true);
    }
}

// 【新增】扣除生命值並隱藏對應圖示
void HUDManager::DecreaseLife() {
    if (mLives > 0) {
        mLives--;
        if (mLives < (int)lifeObjects.size()) {
            lifeObjects[mLives]->SetVisible(false);
        }
    }
}
