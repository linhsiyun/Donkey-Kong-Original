#ifndef HUDMANAGER_HPP
#define HUDMANAGER_HPP

#include <memory>
#include <vector>
#include "Util/Text.hpp"
#include "Util/Color.hpp"
#include "Util/GameObject.hpp"
#include "Util/Renderer.hpp"
#include "Character.hpp"

/* 計分規則:
  1. 跳躍得分 (Jumping)
     跳過一個障礙物（木桶、火球、水泥塊）：100 分
     同時跳過兩個以上的障礙物：300 / 500 / 800 分（視難度與版本而定，通常一次跳過兩個桶子會獲得更高獎勵）。

     程式邏輯建議：在跳躍狀態下，檢查玩家的 X 軸是否越過了一個障礙物的 X 軸，且玩家當時處於 isGrounded == false。

  2. 槌子擊碎得分 (Smashing with Hammer)
     當玩家撿起槌子並擊碎敵對目標時，分數是隨機產生的。
     - 擊碎木桶 (Barrels)：隨機獲得 300、500 或 800 分。
     - 擊碎火球 (Fireballs)：隨機獲得 300、500 或 800 分。
     - 擊碎水泥塊 (Sand Piles)：隨機獲得 300、500 或 800 分。

     開發小細節：原版遊戲並不是真正的「純隨機」，它通常根據一個快速跳動的計時器（Timer）來決定給哪一個分數。

   3. 任務目標得分 (Stage Objectives)
      抵達頂端：在 25m, 50m, 75m 關卡中抵達波琳所在的平台。這部分本身不直接給大額固定分，而是觸發「Bonus 結算」。

      每一關開始時，Bonus 會從一個數值開始（第一關 5000，隨關卡增加）。
      大約每 2 秒鐘，Bonus 會自動減少 100 分。

      當玩家過關時，畫面上剩餘的 Bonus 數值會直接加進 Current Score。
      失敗條件：如果 Bonus 倒數到 0，玩家會立即損失一條生命。

    4. 獎勵生命 (Extra Life)
       標準規則：當總分達到 7,000 分 時，玩家獲得額外的一條生命（1UP）。
       之後是否還有獎勵（如每 20,000 分）可由你在程式中自行設定。
 */


// 文字元素 (HUD - Heads Up Display)
class HUDManager {
private:
    int currentScore = 0;
    int highScore = 0;      // 歷史最高紀錄。
    int bonusTime = 5000;   // 這不只是分數，它是限時器。每一關開始時從 5000 開始倒數(-100)，歸零時玩家死亡。
    int level = 1;          // 目前的關卡數（例如 L=01）。
    int mLives = 3;         // 【新增】剩餘生命值

    // Text 負責顯示文字內容，GameObject 負責處理位置與渲染層級 (ZIndex)
    std::shared_ptr<Util::Text> scoreText;
    std::shared_ptr<Util::GameObject> scoreObject;

    std::shared_ptr<Util::Text> highScoreText;
    std::shared_ptr<Util::GameObject> highScoreObject;

    std::shared_ptr<Util::Text> bonusText;
    std::shared_ptr<Util::GameObject> bonusObject;

    std::shared_ptr<Util::Text> levelText;
    std::shared_ptr<Util::GameObject> levelObject;

    // 用於顯示生命值的圖示清單
    std::vector<std::shared_ptr<Util::GameObject>> lifeObjects;

    // 新增：槌子圖示與計數
    std::shared_ptr<Character> hammerIcon;
    std::shared_ptr<Util::Text> hammerCountText;
    std::shared_ptr<Util::GameObject> hammerCountObject;

    // 新增：木桶圖示與計數
    std::shared_ptr<Character> barrelIcon;
    std::shared_ptr<Util::Text> barrelCountText;
    std::shared_ptr<Util::GameObject> barrelCountObject;

    // --- 高分提醒相關成員 ---
    std::shared_ptr<Character> m_HighScoreNotifyIcon;
    bool m_IsShowingHighScoreIcon = false;
    float m_HighScoreIconTimer = 0.0f;

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
    void DecreaseLife();       // 【新增】扣除生命
    void SaveHighScore();      // 【新增】宣告存檔函式
    int GetLives() const { return mLives; } // 【新增】取得剩餘生命
    int GetBonus() { return bonusTime; }
    void SetLevel(int level);
};
#endif // HUDMANAGER_HPP
