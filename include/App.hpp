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
//#include "AnimatedCharacter.hpp"

#include "Mario.hpp"
#include "Fiamma.hpp"
#include "HUDManager.hpp"
#include "Elevator.hpp"
#include "DonkeyKong.hpp"
#include "Barrel.hpp"

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
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
    std::vector<std::shared_ptr<Barrel>> m_Barrels; // 儲存所有畫面上的酒桶
    std::shared_ptr<AnimatedCharacter> m_SmashEffect; // 搥擊特效動畫
    std::shared_ptr<Character> m_BlackCover; // 【新增】通關時遮住中間結構的黑色方塊

    float halfWidth;
    float halfHeight;

    // 定義當前的關卡編號
    int m_CurrentLevel = 1;
    int m_CurrentStage = 1;
    float m_FreezeTimer = 0.0f; // 畫面凍結計時器
    float m_DKFallTimer = 0.0f; // 【新增】DK 下墜旋轉計時器
    int m_RivetCount = 0;       // 【新增】剩餘插銷數量

    // 【新增】紀錄 Level 4 的插銷視覺物件，Key 為網格座標 {x, y}
    std::map<std::pair<int, int>, std::shared_ptr<Character>> m_RivetVisuals;
    glm::vec2 m_ActiveRivetPos = {0.0f, 0.0f}; // 【新增】紀錄 Mario 目前正踩在哪個插銷的中心座標上
    bool m_HasActiveRivet = false;             // 【新增】標記目前是否正踩在插銷上
};

#endif