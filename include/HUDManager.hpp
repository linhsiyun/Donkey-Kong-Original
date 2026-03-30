#ifndef HUDMANAGER_HPP
#define HUDMANAGER_HPP

#include <memory>
#include "Util/Text.hpp"
#include "Util/Color.hpp"
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"

// 文字元素 (HUD - Heads Up Display)
class HUDManager {
private:
    int currentScore = 0;
    int highScore = 0;      // 歷史最高紀錄。
    int bonusTime = 5000;   // 這不只是分數，它是限時器。每一關開始時從 5000 開始倒數(-100)，歸零時玩家死亡。
    int level = 1;          // 目前的關卡數（例如 L=01）。

    // Text 負責顯示文字內容，GameObject 負責處理位置與渲染層級 (ZIndex)
    std::shared_ptr<Util::Text> scoreText;
    std::shared_ptr<Util::GameObject> scoreObject;

    std::shared_ptr<Util::Text> highScoreText;
    std::shared_ptr<Util::GameObject> highScoreObject;

    std::shared_ptr<Util::Text> bonusText;
    std::shared_ptr<Util::GameObject> bonusObject;

    std::shared_ptr<Util::Text> levelText;
    std::shared_ptr<Util::GameObject> levelObject;

    float bonusTimer = 0.0f; // 用來計時 1 秒扣一次分數

public:
    HUDManager();
    
    void Init();
    void Update(float deltaTime); // 處理 Bonus 倒數, deltaTime: 從上一幀到現在經過的秒數 

    // 將 所有 text 組件註冊到 App 的 Renderer 中
    void AddToRenderer(Util::Renderer& renderer);

    // 提供 API 給其他物件呼叫
    void AddScore(int points);
    void ResetBonus(int amount = 5000);
    int GetBonus() { return bonusTime; }
};
#endif // HUDMANAGER_HPP