#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Util/Renderer.hpp"
#include "Map.hpp"

#include "Util/GameObject.hpp"
#include <map>

//#include "Character.hpp"
//not used, #include "Util/Text.hpp"
//not used, #include "PhaseResourceManger.hpp"
#include "AnimatedCharacter.hpp"

#include "Mario.hpp"
#include "Fiamma.hpp"
#include "HUDManager.hpp"
#include "Elevator.hpp"
#include "DonkeyKong.hpp"
#include "Barrel.hpp"
#include "CementPan.hpp"
#include "MovingLadder.hpp"
#include "OpeningScene.hpp"

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
    };

    enum class Stage {
        BARRELS = 1,
        CONVEYORS = 2,
        ELEVATORS = 3,
        RIVETS = 4
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Start();

    void Update();

    void End(); // NOLINT(readability-convert-member-functions-to-static)

    // 新增：載入關卡的函式，將傳入關卡編號
    void LoadLevel(int level);

private:
    void ValidTask();

    // 新增：將原本在 lambda 裡面的木桶生成邏輯抽出來變成 App 的成員函式
    void SpawnBarrel();
    void UpdateBarrels(MarioState marioState);
    void UpdateCementPans(MarioState marioState);

    // 【新增】得分視覺顯示邏輯
    struct PointVisual {
        std::shared_ptr<Character> character;
        float remainingTimeMs;
    };
    void SpawnPointVisual(glm::vec2 position, int score);

    void TriggerSmash(glm::vec2 position, int score);

    Util::Renderer m_Renderer;
    std::shared_ptr<Map> m_Map;

    //測試用 std::shared_ptr<Util::GameObject> m_TestMarker;

private:
    State m_CurrentState = State::START;

    //void ShowMario(void);
    std::shared_ptr<Mario> m_Mario;
    std::shared_ptr<Fiamma> m_Fireball;
    std::shared_ptr<HUDManager> m_HUDText;
    std::shared_ptr<DonkeyKong> m_DonkeyKong;
    std::vector<std::shared_ptr<Elevator>> m_Elevators; // 儲存所有畫面上的電梯
    std::vector<std::shared_ptr<Character>> m_Umbrellas; // 【新增】儲存所有雨傘道具
    std::vector<std::shared_ptr<Character>> m_Purses; // 【新增】儲存所有皮包道具
    std::vector<std::shared_ptr<Character>> m_ElevatorStops; // 儲存所有電梯擋板
    std::vector<std::shared_ptr<Barrel>> m_Barrels; // 儲存所有畫面上的酒桶
    std::vector<std::shared_ptr<MovingLadder>> m_MovingLadders; // 【新增】儲存伸縮梯子
    std::vector<std::shared_ptr<CementPan>> m_CementPans; // 儲存所有水泥塊
    std::shared_ptr<AnimatedCharacter> m_SmashEffect; // 搥擊特效動畫
    std::vector<PointVisual> m_PointVisuals; // 【新增】儲存目前畫面上顯示的分數數字圖片
    std::shared_ptr<Character> m_BlackCover; // 【新增】通關時遮住中間結構的黑色方塊
    std::shared_ptr<Character> m_LeftMask;   // 【新增】遮住左側地圖外的黑色遮罩
    std::shared_ptr<Character> m_RightMask;  // 【新增】遮住右側地圖外的黑色遮罩
    std::shared_ptr<Util::GameObject> m_TransitionTextObj; // 【新增】過場文字物件
    std::shared_ptr<Util::Text> m_TransitionText;           // 【新增】過場文字內容
    std::vector<std::shared_ptr<Character>> m_TransitionIcons; // 【新增】過場高度圖示清單

    // 【新增】Game Over 視覺物件
    std::shared_ptr<Character> m_GameOverBlock;
    std::shared_ptr<Util::Text> m_GameOverText;
    std::shared_ptr<Util::GameObject> m_GameOverTextObj;

    // 【新增】公主精靈與動畫控制參數
    std::shared_ptr<AnimatedCharacter> m_Princess;
    std::shared_ptr<Character> m_Heart; // 【新增】過關時顯示的心形
    float m_PrincessTimerMs = 5000.0f; // 每 5s 觸發一次動畫
    float m_PrincessAnimTimerMs = 0.0f; // 動畫內部切換計時器
    int m_PrincessToggleCount = 0;     // 幫助圖切換計數 (切換次數)
    bool m_PrincessAnimating = false;  // 目前是否正在執行切換動畫
    glm::vec2 m_PrincessRefSize = {0.0f, 0.0f}; // 參考幀尺寸，用於統一顯示大小

    // 定義當前的關卡編號
    int m_CurrentLevel = 1;
    Stage m_CurrentStage = Stage::BARRELS;
    float m_FreezeTimer = 0.0f; // 畫面凍結計時器
    float m_TransitionTimer = 0.0f; // 【新增】過場畫面計時器
    bool m_InTransition = false;   // 【新增】是否處於過場狀態
    float m_DKFallTimer = 0.0f; // 【新增】DK 下墜旋轉計時器
    int m_RivetCount = 0;       // 【新增】剩餘插銷數量
    bool m_IsGameOver = false;  // 【新增】是否遊戲結束

    // 【新增】紀錄 Level 4 的插銷視覺物件，Key 為網格座標 {x, y}
    std::map<std::pair<int, int>, std::shared_ptr<Character>> m_RivetVisuals;
    glm::vec2 m_ActiveRivetPos = {0.0f, 0.0f}; // 【新增】紀錄 Mario 目前正踩在哪個插銷的中心座標上
    bool m_HasActiveRivet = false;             // 【新增】標記目前是否正踩在插銷上
    std::shared_ptr<Character> m_OilBarrel;
    std::shared_ptr<AnimatedCharacter> m_BurningOilBarrel;
    bool m_IsFirstBarrel = true; // 用來追蹤是否為第一顆木桶
    std::shared_ptr<OpeningScene> m_OpeningScene;
    bool m_IsOpeningSequence = false;
};

#endif
