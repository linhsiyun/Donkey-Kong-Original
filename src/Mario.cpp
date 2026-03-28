#include "Mario.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

Mario::Mario() {
    // 初始化 靜止的 Mario (Character)
    m_Idle = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk0.png");
    m_Idle->SetZIndex(50);
    m_Idle->SetScale({marioScale, marioScale});
    m_Idle->SetVisible(true);

    // 初始化 Walk 動畫 (AnimatedCharacter)
    //std::vector<std::string> walkImages = { /*...*/ };
    //m_Walk = std::make_shared<AnimatedCharacter>(walkImages);
    std::vector<std::string> walkImages;
    walkImages.reserve(2);
    for (int i = 0; i < 2; ++i) {
        walkImages.emplace_back(RESOURCE_DIR"/Images/Walk" + std::to_string(i + 1) + ".png");
    }
    m_Walk = std::make_shared<AnimatedCharacter>(walkImages);
    m_Walk->SetZIndex(50);
    m_Walk->SetVisible(false); // 預設隱藏

    // 初始化 Mario 攀爬動畫 (AnimatedCharacter)
    std::vector<std::string> climbImages;
    climbImages.reserve(2);
    for (int i = 0; i < 2; ++i) {
        climbImages.emplace_back(RESOURCE_DIR"/Images/Climbing" + std::to_string(i + 1) + ".png");
    }
    m_Climb = std::make_shared<AnimatedCharacter>(climbImages);
    m_Climb->SetZIndex(50);
    m_Climb->SetVisible(false);  // 預設隱藏
    m_Climb->SetScale({marioScale, marioScale});

    // 初始化 Mario 跳躍狀態 (Character) - 這裡使用靜態圖片
    m_Jump = std::make_shared<Character>(RESOURCE_DIR"/Images/Jump.png");
    m_Jump->SetZIndex(50);
    m_Jump->SetVisible(false);
    m_Jump->SetScale({marioScale, marioScale});

    // 初始化 Mario 死亡動畫 (AnimatedCharacter: end1, 2, 3, 4)
    std::vector<std::string> deadImages;
    deadImages.reserve(4);
    for (int i = 1; i <= 4; ++i) {
        deadImages.emplace_back(RESOURCE_DIR"/Images/end" + std::to_string(i) + ".png");
    }
    m_Dead = std::make_shared<AnimatedCharacter>(deadImages);
    m_Dead->SetZIndex(55); // 死亡動畫層級設高一點
    m_Dead->SetVisible(false);
    m_Dead->SetScale({marioScale, marioScale});

    // 初始化 Mario 拿槌子的動畫 (Hammer1 ~ Hammer6)
    std::vector<std::string> hammerImages;
    hammerImages.reserve(6);
    for (int i = 1; i <= 6; ++i) {
        hammerImages.emplace_back(RESOURCE_DIR"/Images/Hammer" + std::to_string(i) + ".png");
    }
    m_Hammer = std::make_shared<AnimatedCharacter>(hammerImages);
    m_Hammer->SetZIndex(50);
    m_Hammer->SetVisible(false);
    m_Hammer->SetScale({marioScale, marioScale});
    m_Hammer->SetInterval(150); // 將間隔從 100ms 增加到 150ms 以放慢速度
    m_Hammer->SetLooping(true);

    // 初始化 Mario 最終死亡定格圖
    m_DiedFinal = std::make_shared<Character>(RESOURCE_DIR"/Images/mario_died.png");
    m_DiedFinal->SetZIndex(55);
    m_DiedFinal->SetVisible(false);
    m_DiedFinal->SetScale({marioScale, marioScale});

    // 初始化跳躍狀態，確保一開始不會誤動作
    m_IsJumping = false;
    m_JumpTimer = 0.0f;
}

// 這個函式讓 App 只要呼叫一次，就把所有狀態的圖層加進渲染器
void Mario::AddToRenderer(Util::Renderer& renderer) {
    renderer.AddChild(m_Idle);
    renderer.AddChild(m_Walk);
    renderer.AddChild(m_Climb);
    renderer.AddChild(m_Jump);
    renderer.AddChild(m_Hammer);
    renderer.AddChild(m_Dead);
    renderer.AddChild(m_DiedFinal);
    //TODO: Fall, Hammer, HammerIdle, Dead, Win 等也要加入渲染器
}

// 關鍵函式：設定座標時，所有潛在的圖層都一起更新座標
void Mario::SetPosition(const glm::vec2& position) {
    m_Position = position;
    m_Idle->SetPosition(position);
    m_Walk->SetPosition(position);
    m_Climb->SetPosition(position);
    m_Jump->SetPosition(position);

    // 修正槌子動畫中心點偏移問題
    // 因為槌子圖片通常比一般走路圖案高（包含槌子揮上去的高度），導致圖片中心點上移，Mario 的身體就會看起來往下掉或往上飄。
    // 可以微調 hammerYOffset 的數值（例如 -5.0f 到 -10.0f）直到身體對齊。
    float hammerYOffset = -10.0f * marioScale; 
    float hammerXOffset = 0.0f;
    
    // 如果向右轉時感覺左右偏移不對稱，也可以在這裡根據 m_Direction 微調 XOffset
    // if (m_Direction == MarioDIR::RIGHT) hammerXOffset = 2.0f * marioScale;

    m_Hammer->SetPosition(position + glm::vec2{hammerXOffset, hammerYOffset});

    m_Dead->SetPosition(position);
    m_DiedFinal->SetPosition(position);
    //TODO: Fall, Hammer, HammerIdle, Dead, Win 等也要一起更新位置
}

glm::vec2 Mario::GetSize() const {
    return m_Idle->GetSize();
}

void Mario::SetState(MarioState newState) {
    if (m_CurrentState == newState) return; // 狀態沒變就不處理

    m_CurrentState = newState;
    //UpdateVisibility(); // 狀態改變，立刻更新圖片顯示
}

void Mario::IDLE() {
    if (m_CurrentState != MarioState::IDLE && m_CurrentState != MarioState::HAMMERING) {
        LOG_DEBUG("IDLE");
        if (m_Direction == MarioDIR::LEFT) {
            m_Idle->SetScale({marioScale, marioScale});
        }
        else {
            m_Idle->SetScale({-1*marioScale, marioScale});
        }
        SetState(MarioState::IDLE);
    }
}

void Mario::Walk(MarioDIR dir) {
    // TODO: 偵測邊界，如果有邊界的話就不能再往那個方向移動了 ...
    if (dir == MarioDIR::LEFT) {
        m_Position.x -= movingStep;
        m_Direction = MarioDIR::LEFT;
        m_Walk->SetScale({marioScale, marioScale});
    } else if (dir == MarioDIR::RIGHT) {
        m_Position.x += movingStep;
        m_Direction = MarioDIR::RIGHT;
        m_Walk->SetScale({-1*marioScale, marioScale});
    }

    // 如果狀態切換為走路，開始播放動畫
    if (m_CurrentState != MarioState::WALKING && m_CurrentState != MarioState::HAMMERING) {
        LOG_DEBUG("WALKING: " + std::string((dir == MarioDIR::LEFT) ? "LEFT" : "RIGHT") );
        m_Walk->SetLooping(true);
        m_Walk->SetInterval(250);    // 每(x)ms更新動畫
        m_Walk->Play();
        SetState(MarioState::WALKING);
    }
    //SetPosition(m_Position); // 更新位置
}

void Mario::Climb(CLIMB_DIR dir) {
    // TODO: 偵測邊界，如果有邊界的話就不能再往那個方向移動了 ...

    // 只有在非槌子狀態下才允許位移與切換攀爬動畫
    if (m_CurrentState != MarioState::HAMMERING) {
        if (dir == CLIMB_DIR::UP) {
            m_Position.y += climbingStep;
        }
        else if (dir == CLIMB_DIR::DOWN) {
            m_Position.y -= climbingStep;
        }
    }

    // 如果狀態切換為攀爬，開始播放動畫
    if (m_CurrentState != MarioState::CLIMBING && m_CurrentState != MarioState::HAMMERING) {
        LOG_DEBUG("CLIMBING: " + std::string((dir == CLIMB_DIR::UP) ? "UP" : "DOWN") );

        m_Climb->SetLooping(true);
        m_Climb->SetInterval(200);      // 每(x)ms更新動畫
        m_Climb->SetScale({marioScale, marioScale});
        m_Climb->Play();
        SetState(MarioState::CLIMBING);
    }
    //SetPosition(m_Position); // 更新位置
}

void Mario::ClimbIdle() {
    if (m_CurrentState != MarioState::CLIMB_IDLE) {
        LOG_DEBUG("CLIMB_IDLE");
        SetState(MarioState::CLIMB_IDLE);
        //m_Climb->Stop(); // 停止攀爬動畫，回到第一幀（靜止在梯子上）
    }
}

void Mario::JumpStart() {
    if (!m_IsJumping && m_CurrentState != MarioState::HAMMERING) {
        LOG_DEBUG("JUMPSTART");
        m_IsJumping = true;
        m_JumpStartPosition = m_Position;
        m_JumpTimer = 0.0f;
        m_BackupDirection = m_Direction; // 跳躍開始時備份當前方向
        m_Direction = MarioDIR::NONE;    // 預設為垂直跳躍

        // 偵測是否同時按住左右鍵
        if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) m_Direction = MarioDIR::RIGHT;
        if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) m_Direction = MarioDIR::LEFT;

        // 如果是垂直跳躍，讓跳躍貼圖的面向跟隨目前 m_BackupDirection 的面向
        if (m_Direction == MarioDIR::NONE){
            if (m_BackupDirection == MarioDIR::LEFT) {
                m_Jump->SetScale({marioScale, marioScale});
            }
            else {
                m_Jump->SetScale({-1*marioScale, marioScale});
            }
        }
        else if (m_Direction == MarioDIR::LEFT) {
             m_Jump->SetScale({marioScale, marioScale});
        }
        else if (m_Direction == MarioDIR::RIGHT) {
             m_Jump->SetScale({-1*marioScale, marioScale});
        }

        SetState(MarioState::JUMPING);
    }
}

void Mario::Jump() {
    // 設定跳躍參數 (後續可以修改這裡的數值)
    const float totalJumpTime = 35.0f; // 跳躍總時間 (Frames) - 時間越短跳越快
    const float jumpHeight = 70.0f;    // 跳躍高度

    m_JumpTimer += 1.0f;

    if (m_JumpTimer > totalJumpTime) {
        LOG_DEBUG("IDLE");
        m_IsJumping = false;
        SetPosition({m_Position.x, m_JumpStartPosition.y}); // 確保回到地面

        // 這裡設回站立，避免落地瞬間還顯示跳躍圖
        SetState(MarioState::IDLE);
        m_JumpTimer = 0.0f;
        m_Direction = m_BackupDirection;   // 跳躍結束後恢復之前的方向
    }
    else {
        LOG_DEBUG("JUMP");
        // 計算拋物線高度: 4 * h * t * (1-t)
        float progress = m_JumpTimer / totalJumpTime;
        float yOffset = 4.0f * jumpHeight * progress * (1.0f - progress);
        m_Position.y = m_JumpStartPosition.y + yOffset;
        // 計算二次 m_Position.y += yOffset;

        // 水平移動與面向設定
        if (m_Direction == MarioDIR::LEFT){
            m_Position.x -= movingStep;
        }
        else if (m_Direction == MarioDIR::RIGHT){
            m_Position.x += movingStep;
        }

        SetState(MarioState::JUMPING);  // 確保狀態保持在 JUMPING
    }
}

void Mario::Fall(){
    LOG_DEBUG("FALLING");
    SetState(MarioState::FALLING);
    //TODO: 落地後要不要有個短暫的無敵狀態？（例如剛從平台邊緣掉下去，還沒接觸到地面就先判定為 FALLING，等落地後再切回 IDLE 或 WALKING）
};

void Mario::Hammer() {
    if (m_CurrentState != MarioState::DEAD) {
        LOG_DEBUG("HAMMER TIME! (10 SECONDS)");
        m_HammerTimer = 10.0f * 60.0f; // 假設 60 FPS，總共 900 幀
        SetState(MarioState::HAMMERING);
        m_Hammer->Play();
    }
}

void Mario::HammerIdle() {
    LOG_DEBUG("HAMMER_IDLE");
    SetState(MarioState::HAMMER_IDLE);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 HAMMER_IDLE 狀態下不能跳躍或攀爬，或者有個專屬的待機動畫等等
}

void Mario::Dead() {
    if (m_CurrentState != MarioState::DEAD) {
        LOG_DEBUG("DEAD SEQUENCE START");
        m_DeadTimer = 0.0f;
        SetState(MarioState::DEAD);
        m_Dead->Stop(); // 確保回到第一幀 (end1.png)
    }
}

void Mario::Win() {
    LOG_DEBUG("WIN");
    SetState(MarioState::WIN);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 WIN 狀態下不能移動或跳躍，或者有個專屬的過場動畫等等
}


// 每幀更新：給 App的Update 呼叫
void Mario::Update() {

    // 先把所有人隱藏
    m_Idle->SetVisible(false);
    m_Walk->SetVisible(false);
    m_Climb->SetVisible(false);
    m_Jump->SetVisible(false);
    m_Hammer->SetVisible(false);
    m_Dead->SetVisible(false);
    m_DiedFinal->SetVisible(false);
    // TODO:
    //m_Fall->SetVisible(false);
    // or
    //if(m_Fall) m_Fall->SetVisible(false);
    //if(m_Hammer) m_Hammer->SetVisible(false);
    // ...

    // 2. 停止不該播放的動畫
    if (m_CurrentState!=MarioState::WALKING) m_Walk->Stop();
    if (m_CurrentState!=MarioState::CLIMBING) m_Climb->Stop();
    if (m_CurrentState!=MarioState::DEAD) m_Dead->Stop();
    //TODO: m_FALL->Stop(); ... 其他動畫如果有的話也要停止

    // 3. 根據現在的狀態決定顯示哪個圖層，並執行該狀態的位移邏輯
    switch (m_CurrentState) {
        case MarioState::IDLE:
            m_Idle->SetVisible(true);
            break;
        case MarioState::WALKING:
            m_Walk->SetVisible(true);
            m_Walk->Play(); // 開始播放走路動畫
            break;
        case MarioState::JUMPING:
            m_Jump->SetVisible(true);
            break;
        case MarioState::CLIMBING:
            m_Climb->SetVisible(true);
            m_Climb->Play(); // 開始播放攀爬動畫
            break;
        case MarioState::CLIMB_IDLE:
            m_Climb->SetVisible(true);
            break;
        case MarioState::FALLING:
            break;
        case MarioState::HAMMERING:
            m_Hammer->SetVisible(true);
            m_Hammer->Play();
            
            // 處理計時器
            m_HammerTimer -= 1.0f;
            if (m_HammerTimer <= 0) {
                LOG_DEBUG("HAMMER EXPIRED");
                SetState(MarioState::IDLE);
            }

            // 根據面向調整縮放
            if (m_Direction == MarioDIR::LEFT) {
                m_Hammer->SetScale({marioScale, marioScale});
            } else {
                m_Hammer->SetScale({-1.0f * marioScale, marioScale});
            }
            break;
        case MarioState::HAMMER_IDLE:   // TODO: m_HammerIdle->SetVisible(true);
            break;
        case MarioState::DEAD:
            m_DeadTimer += 1.0f; // 假設遊戲運行於 60 FPS

            if (m_DeadTimer < 30.0f) {
                // 1. 前 0.5 秒：停在 end1.png
                m_Dead->SetVisible(true);
                m_Dead->Stop(); // 預設回到第 0 幀
            } 
            else if (m_DeadTimer < 102.0f) {
                // 2. 接下來約 1.2 秒：快速循環 3 次 (假設 100ms 一幀，一圈 4 幀需 400ms)
                m_Dead->SetVisible(true);
                if (!m_Dead->IsPlaying()) {
                    LOG_DEBUG("End"); // 旋轉開始時輸出 End
                    m_Dead->SetLooping(true);
                    m_Dead->SetInterval(100); // 快速播放
                    m_Dead->Play();
                }
            } 
            else {
                // 3. 最後：停在 mario_died.png
                m_Dead->Stop();
                m_Dead->SetVisible(false);
                m_DiedFinal->SetVisible(true);
            }
            break;
        case MarioState::WIN:    // TODO:
            break;
    }

    // 4. 重要：同步物理座標到所有圖片物件
    // 無論 Mario 是在走路、跳躍還是死亡墜落，這行保證了畫面上看到的圖會跟著 m_Position 移動
    SetPosition(m_Position);
}
