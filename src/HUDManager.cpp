#include "HudManager.hpp"
#include <iomanip>
#include <sstream>
#include "config.hpp"

static std::string FormatInt(int score, int width) {
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << score;
    return ss.str();
}

HUDManager::HUDManager() {
    // 透過 PTSD_Config 取得視窗大小
    float halfWidth = static_cast<float>(WINDOW_WIDTH) / 2.0f;
    float halfHeight = static_cast<float>(WINDOW_HEIGHT) / 2.0f;

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
    scoreObject->m_Transform.translation = {-halfWidth + 100.0f, halfHeight - 50.0f};
    scoreObject->SetVisible(true);

    highScoreText = std::make_shared<Util::Text>(fontPath, 16, FormatInt(highScore, 6), white);
    highScoreObject = std::make_shared<Util::GameObject>();
    highScoreObject->SetDrawable(highScoreText);
    highScoreObject->SetZIndex(100);
    highScoreObject->m_Transform.translation = {0.0f, halfHeight - 50.0f};
    highScoreObject->SetVisible(true);

    levelText = std::make_shared<Util::Text>(fontPath, 16, "L=" + FormatInt(level,2), blue);
    levelObject = std::make_shared<Util::GameObject>();
    levelObject->SetDrawable(levelText);
    levelObject->SetZIndex(100);
    levelObject->m_Transform.translation = {halfWidth - 100.0f, halfHeight - 50.0f};
    levelObject->SetVisible(true);


    bonusText = std::make_shared<Util::Text>(fontPath, 16, std::to_string(bonusTime), white);
    bonusObject = std::make_shared<Util::GameObject>();
    bonusObject->SetDrawable(bonusText);
    bonusObject->SetZIndex(100);
    bonusObject->m_Transform.translation = {halfWidth - 100.0f, halfHeight - 100.0f}; // 往下位移避免重疊
    bonusObject->SetVisible(true);
}

void HUDManager::Init() {
    currentScore = 0;
    scoreText->SetText(FormatInt(currentScore, 6));
    ResetBonus(5000);
}

void HUDManager::AddToRenderer(Util::Renderer& renderer) {
    // 將包裝後的 GameObject 加入渲染器，而非 Drawable 本身
    renderer.AddChild(scoreObject);
    renderer.AddChild(highScoreObject);
    renderer.AddChild(levelObject);
    renderer.AddChild(bonusObject);
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
