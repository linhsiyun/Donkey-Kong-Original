#ifndef OPENING_SCENE_HPP
#define OPENING_SCENE_HPP

#include <memory>
#include "DonkeyKong.hpp"
#include "Map.hpp"
#include "Mario.hpp"
#include "AnimatedCharacter.hpp"

class OpeningScene {
public:
    OpeningScene(std::shared_ptr<DonkeyKong> dk, 
                 std::shared_ptr<Map> map, 
                 std::shared_ptr<Mario> mario, 
                 std::shared_ptr<AnimatedCharacter> princess);

    // 啟動開場動畫（重置計時器與位置）
    void Start();

    // 每幀更新動畫邏輯
    void Update(float dt);

    // 讓 App 知道動畫是否播完了
    bool IsFinished() const { return m_IsFinished; }

private:
    std::shared_ptr<DonkeyKong> m_DonkeyKong;
    std::shared_ptr<Map> m_Map;
    std::shared_ptr<Mario> m_Mario;
    std::shared_ptr<AnimatedCharacter> m_Princess;

    bool m_IsFinished = false;
    int m_Phase = 0;
    float m_Timer = 0.0f;
    int m_StompCount = 0;

    float m_LogicY = 600.0f;
    int m_BounceCount = 0;
};

#endif