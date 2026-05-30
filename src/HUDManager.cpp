#include "HUDManager.hpp"
#include <iomanip>
#include <sstream>
#include "Character.hpp"
#include "config.hpp"
#include "CoordinateManager.hpp"

static std::string FormatInt(int score, int width) {
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << score;
    return ss.str();
}

HUDManager::HUDManager() {
    // --- 這裡只設定一次 ---
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
        lifeIcon->SetScale({1.0f, 1.0f}); // 縮小比例

        // 絕對邏輯座標 X=-110 起始，每個間距 22；Y 統一為 75
        float logicX = -110.0f + (i * 22.0f);
        float logicY = 75.0f;
        lifeIcon->SetPosition(CoordinateManager::LogicToEngine({logicX, logicY}));

        lifeObjects.push_back(lifeIcon);
    }

    // 在生命圖示下方放置 Hammer 圖示與初始值 0
    hammerIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    hammerIcon->SetZIndex(100);
    hammerIcon->SetScale({1.5f, 1.5f}); // 縮小比例
    hammerIcon->SetPosition(CoordinateManager::LogicToEngine({-110.0f, 100.0f})); // 對齊左側 X=-110，往下排 Y=100

    hammerCountText = std::make_shared<Util::Text>(fontPath, 16, " 0", white);
    hammerCountObject = std::make_shared<Util::GameObject>();
    hammerCountObject->SetDrawable(hammerCountText);
    hammerCountObject->SetZIndex(100);
    hammerCountObject->m_Transform.translation = CoordinateManager::LogicToEngine({-70.0f, 100.0f}); // 文字靠右 X=-70
    hammerCountObject->SetVisible(true);

    // 在 Hammer 下方放置 Barrel 圖示與初始值 0
    barrelIcon = std::make_shared<Character>(RESOURCE_DIR"/Images/Barrel1.png");
    barrelIcon->SetZIndex(100);
    barrelIcon->SetScale({1.0f, 1.0f}); // 縮小比例
    barrelIcon->SetPosition(CoordinateManager::LogicToEngine({-110.0f, 125.0f})); // 對齊左側 X=-110，再往下排 Y=125

    barrelCountText = std::make_shared<Util::Text>(fontPath, 16, " 0", white);
    barrelCountObject = std::make_shared<Util::GameObject>();
    barrelCountObject->SetDrawable(barrelCountText);
    barrelCountObject->SetZIndex(100);
    barrelCountObject->m_Transform.translation = CoordinateManager::LogicToEngine({-70.0f, 125.0f}); // 文字靠右 X=-70
    barrelCountObject->SetVisible(true);
}

void HUDManager::Init() {
    currentScore = 0;
    scoreText->SetText(FormatInt(currentScore, 6));

    // 【新增】初始化生命值顯示
    mLives = 3;
    for (auto& life : lifeObjects) {
        life->SetVisible(true);
    }

    ResetBonus(5000);
}

void HUDManager::AddToRenderer(Util::Renderer& renderer) {
    // 將包裝後的 GameObject 加入渲染器，而非 Drawable 本身
    renderer.AddChild(scoreObject);
    renderer.AddChild(highScoreObject);
    renderer.AddChild(levelObject);
    renderer.AddChild(bonusObject);

    // 將生命值圖示加入渲染器
    for (auto& life : lifeObjects) {
        renderer.AddChild(life);
    }

    // 加入新的圖示與文字到渲染器
    renderer.AddChild(hammerIcon);
    renderer.AddChild(hammerCountObject);
    renderer.AddChild(barrelIcon);
    renderer.AddChild(barrelCountObject);
}

void HUDManager::Update(float deltaTime) {
    scoreText->SetText(FormatInt(currentScore, 6));   // test only

    // 更新 Bonus 倒數
    bonusTimer += deltaTime;
    if (bonusTimer >= 2000.0f && bonusTime > 00.0f) {
        bonusTime -= 100;
        bonusTimer = 0.0f;
        bonusText->SetText(FormatInt(bonusTime, 4));

        //TODO: if (bonusTime==0) 玩家死亡
    }
}

void HUDManager::AddScore(int points) {
    currentScore += points;
    scoreText->SetText(FormatInt(currentScore, 6));
    if (currentScore > highScore) {
        highScore = currentScore;
        highScoreText->SetText(FormatInt(highScore, 6));
    }
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

// 【新增】扣除生命值並隱藏對應圖示
void HUDManager::DecreaseLife() {
    if (mLives > 0) {
        mLives--;
        if (mLives < (int)lifeObjects.size()) {
            lifeObjects[mLives]->SetVisible(false);
        }
    }
}
