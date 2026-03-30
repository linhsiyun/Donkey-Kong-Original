#include "HudManager.hpp"
#include <iomanip>
#include <sstream>

static std::string FormatInt(int score, int width) {
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << score;
    return ss.str();
}

HUDManager::HUDManager() {
    // Util::Text constructor requires: font, size, text, color, visibility
    scoreText = std::make_shared<Util::Text>(RESOURCE_DIR"/donkey-kong-classics.ttf", 16, FormatInt(currentScore, 6), Util::Color(255, 255, 255, 255), true);
    scoreObject = std::make_shared<Util::GameObject>();
    scoreObject->SetDrawable(scoreText);
    scoreObject->SetZIndex(100);  // HUD 應設為較高的 ZIndex 以顯示在最前方
    scoreObject->m_Transform.translation = {-200.0f, 150.0f};
    scoreObject->SetVisible(true);

    highScoreText = std::make_shared<Util::Text>(RESOURCE_DIR"/donkey-kong-classics.ttf", 16, FormatInt(highScore, 6), Util::Color(255, 255, 255, 255), true);
    highScoreObject = std::make_shared<Util::GameObject>();
    highScoreObject->SetDrawable(highScoreText);
    highScoreObject->SetZIndex(100);
    highScoreObject->m_Transform.translation = {0.0f, 150.0f}; 
    highScoreObject->SetVisible(true);

    levelText = std::make_shared<Util::Text>(RESOURCE_DIR"/donkey-kong-classics.ttf", 16, "L=" + FormatInt(level,2), Util::Color(0, 0, 255, 255), true);
    levelObject = std::make_shared<Util::GameObject>();
    levelObject->SetDrawable(levelText);
    levelObject->SetZIndex(100);
    levelObject->m_Transform.translation = {200.0f, 150.0f};
    levelObject->SetVisible(true);


    bonusText = std::make_shared<Util::Text>(RESOURCE_DIR"/donkey-kong-classics.ttf", 16, std::to_string(bonusTime), Util::Color(255, 255, 255, 255), true);
    bonusObject = std::make_shared<Util::GameObject>();
    bonusObject->SetDrawable(bonusText);
    bonusObject->SetZIndex(100);
    bonusObject->m_Transform.translation = {200.0f, 100.0f}; // 往下位移避免重疊
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
    if (bonusTimer >= 1000.0f) {
        bonusTime -= 100;
        bonusTimer = 0.0f;
        bonusText->SetText(FormatInt(bonusTime, 4));
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
