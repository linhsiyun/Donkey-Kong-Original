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

    // ... 依序初始化 Fall, Hammer 等

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
    //TODO: Fall, Hammer, HammerIdle, Dead, Win 等也要加入渲染器
}

// 關鍵函式：設定座標時，所有潛在的圖層都一起更新座標
void Mario::SetPosition(const glm::vec2& position) {
    m_Position = position;
    m_Idle->SetPosition(position);
    m_Walk->SetPosition(position);
    m_Climb->SetPosition(position);
    m_Jump->SetPosition(position);
    //TODO: Fall, Hammer, HammerIdle, Dead, Win 等也要一起更新位置
}

void Mario::SetState(MarioState newState) {
    if (m_CurrentState == newState) return; // 狀態沒變就不處理

    m_CurrentState = newState;
    //UpdateVisibility(); // 狀態改變，立刻更新圖片顯示
}

void Mario::IDLE() {
    if (m_CurrentState != MarioState::IDLE) {
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
    if (m_CurrentState != MarioState::WALKING) {
        LOG_DEBUG("WALKING: " + std::string((dir == MarioDIR::LEFT) ? "LEFT" : "RIGHT") );
        m_Walk->SetLooping(true);
        m_Walk->SetInterval(200);    // 每(x)ms更新動畫
        m_Walk->Play();
        SetState(MarioState::WALKING);
    }
    //SetPosition(m_Position); // 更新位置
}

void Mario::Climb(CLIMB_DIR dir) {
    // TODO: 偵測邊界，如果有邊界的話就不能再往那個方向移動了 ...

    // 簡單的向上移動，實際數值可調整
    if (dir == CLIMB_DIR::UP) {
        m_Position.y += climbingStep;
        dir = CLIMB_DIR::UP;
    }
    else if (dir == CLIMB_DIR::DOWN) {
        m_Position.y -= climbingStep;
        dir = CLIMB_DIR::DOWN;
    }

    // 如果狀態切換為攀爬，開始播放動畫
    if (m_CurrentState!=MarioState::CLIMBING) {
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
    if (!m_IsJumping) {
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
    LOG_DEBUG("HAMMERING");
    SetState(MarioState::HAMMERING);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 HAMMERING 狀態下不能跳躍或攀爬，或者有個專屬的攻擊動畫等等
}

void Mario::HammerIdle() {
    LOG_DEBUG("HAMMER_IDLE");
    SetState(MarioState::HAMMER_IDLE);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 HAMMER_IDLE 狀態下不能跳躍或攀爬，或者有個專屬的待機動畫等等
}

void Mario::Dead() {
    LOG_DEBUG("DEAD");
    SetState(MarioState::DEAD);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 DEAD 狀態下不能移動或跳躍，或者有個專屬的死亡動畫等等
}

void Mario::Win() {
    LOG_DEBUG("WIN");
    SetState(MarioState::WIN);
    //TODO: 這裡可以加入一些特殊邏輯，例如在 WIN 狀態下不能移動或跳躍，或者有個專屬的過場動畫等等
}


// 每幀更新：給 App的Update 呼叫
void Mario::Update() {
    SetPosition(m_Position); // 更新所有位置

    // 先把所有人隱藏
    m_Idle->SetVisible(false);
    m_Walk->SetVisible(false);
    m_Climb->SetVisible(false);
    m_Jump->SetVisible(false);
    // TODO:
    //m_Fall->SetVisible(false);
    // or
    //if(m_Fall) m_Fall->SetVisible(false);
    //if(m_Hammer) m_Hammer->SetVisible(false);
    // ...

    // 停止動畫
    if (m_CurrentState!=MarioState::WALKING) m_Walk->Stop();
    if (m_CurrentState!=MarioState::CLIMBING) m_Climb->Stop();
    //TODO: m_FALL->Stop(); ... 其他動畫如果有的話也要停止

    // 根據現在的狀態把對應的圖片打開
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
        case MarioState::FALLING:   // TODO: m_Fall->SetVisible(true);  m_Fall->Play();
            break;
        case MarioState::HAMMERING: // TODO: m_Hammer->SetVisible(true);
            break;
        case MarioState::HAMMER_IDLE:   // TODO: m_HammerIdle->SetVisible(true);
            break;
        case MarioState::DEAD:    // TODO:
            break;
        case MarioState::WIN:    // TODO:
            break;
    }
}
