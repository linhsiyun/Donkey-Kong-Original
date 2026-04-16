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
    float hammerYOffset = 3.0f * marioScale;
    float hammerXOffset = 0.0f;

    // 如果向右轉時感覺左右偏移不對稱，也可以在這裡根據 m_Direction 微調 XOffset
    // if (m_Direction == MarioDIR::RIGHT) hammerXOffset = 2.0f * marioScale;

    m_Hammer->SetPosition(position + glm::vec2{hammerXOffset, hammerYOffset});

    m_Dead->SetPosition(position);
    m_DiedFinal->SetPosition(position);
    //TODO: Fall, Hammer, HammerIdle, Dead, Win 等也要一起更新位置
}

glm::vec2 Mario::GetSize() const {
    if (m_CurrentState == MarioState::HAMMERING){
        return m_Hammer->GetSize();
    }
    else if (m_CurrentState == MarioState::HAMMER_IDLE){
        return m_HammerIdle->GetSize();
    }
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

// Walk() 處理 Walking and Hammering 時的水平移動。
// 這裡只管「座標」(m_Position) 與「方向狀態」(m_Direction), 圖片的反轉（SetScale）由 Update() 處理。
void Mario::Walk(MarioDIR dir) {
    // TODO: 偵測邊界，如果有邊界的話就不能再往那個方向移動了 ...
    if (dir == MarioDIR::LEFT) {
        m_Position.x -= movingStep;
        m_Direction = MarioDIR::LEFT;
    } else if (dir == MarioDIR::RIGHT) {
        m_Position.x += movingStep;
        m_Direction = MarioDIR::RIGHT;
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
    // 確保只有在非跳躍狀態，且沒有拿槌子的時候才能起跳
    if (!m_IsJumping && m_CurrentState != MarioState::HAMMERING) {
        LOG_DEBUG("JUMPSTART");

        m_IsJumping = true;

        // ==========================================
        // [修改核心] 賦予向上的物理初速度
        // (這兩行取代了舊的 m_JumpStartPosition 與 m_JumpTimer)
        // ==========================================
        m_VelocityY = m_JumpForce;

        // 備份跳躍開始時的當前方向
        m_BackupDirection = m_Direction;
        m_Direction = MarioDIR::NONE;    // 預設為垂直跳躍

        // 偵測是否同時按住左右鍵
        if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) m_Direction = MarioDIR::RIGHT;
        if (Util::Input::IsKeyPressed(Util::Keycode::LEFT))  m_Direction = MarioDIR::LEFT;

        // 如果是垂直跳躍，讓跳躍貼圖的面向跟隨目前 m_BackupDirection 的面向
        if (m_Direction == MarioDIR::NONE) {
            if (m_BackupDirection == MarioDIR::LEFT) {
                m_Jump->SetScale({marioScale, marioScale});
            }
            else {
                m_Jump->SetScale({-marioScale, marioScale}); // 直接寫 -marioScale 即可
            }
        }
        else if (m_Direction == MarioDIR::LEFT) {
            m_Jump->SetScale({marioScale, marioScale});
        }
        else if (m_Direction == MarioDIR::RIGHT) {
            m_Jump->SetScale({-marioScale, marioScale});
        }

        SetState(MarioState::JUMPING);
    }
}

void Mario::WaitForHammer(){
    m_WaitForHammer = true;
}

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


// 注意：標頭檔 (Mario.hpp) 記得要將 Update 改為接收這兩個參數
// void Update(float deltaTime, const std::shared_ptr<Map>& map);

void Mario::Update(float deltaTime, const std::shared_ptr<Map>& map) {

    // ==========================================
    // 1. 物理與座標運算 (Physics & Movement)
    // ==========================================
    glm::vec2 pos = GetPosition(); // 從底層取得當前座標

    // 如果在空中，套用重力與水平拋物線移動
    if (m_CurrentState == MarioState::JUMPING || m_CurrentState == MarioState::FALLING) {

        // Y 軸：受重力影響遞減
        m_VelocityY -= m_Gravity * (deltaTime / 1000.0f);
        pos.y += m_VelocityY * (deltaTime / 1000.0f);

        // X 軸：依照 JumpStart 決定的方向，進行水平飛行
        if (m_Direction == MarioDIR::LEFT) {
            pos.x -= movingStep;
        } else if (m_Direction == MarioDIR::RIGHT) {
            pos.x += movingStep;
        }

        // 單向平台地面碰撞偵測 (只有往下掉時才檢查)
        if (m_VelocityY < 0.0f && map != nullptr) {
            float footY = pos.y - (GetSize().y / 2.0f);
            float centerX = pos.x;

            TileType footTile = map->GetTileAtPosition(centerX, footY);

            if (footTile == TileType::FLOOR) {
                m_VelocityY = 0.0f; // 踩到地板，速度歸零

                SetState(MarioState::IDLE);
                m_Direction = m_BackupDirection; // 落地後恢復原本面向
                m_IsJumping = false;             // 允許再次跳躍
            }
        }
    }

    // 將物理引擎算好的新座標寫回物件
    SetPosition(pos);
    m_Position = pos; // 確保你原本自己維護的 m_Position 也同步更新

    // ==========================================
    // 2. 視覺與動畫圖層管理 (你原本寫好的完美邏輯)
    // ==========================================

    // 先把所有人隱藏
    m_Idle->SetVisible(false);
    m_Walk->SetVisible(false);
    m_Climb->SetVisible(false);
    m_Jump->SetVisible(false);
    m_Hammer->SetVisible(false);
    m_Dead->SetVisible(false);
    m_DiedFinal->SetVisible(false);
    // TODO: m_Fall->SetVisible(false); 等等...

    // 停止不該播放的動畫
    if (m_CurrentState != MarioState::WALKING) m_Walk->Stop();
    if (m_CurrentState != MarioState::CLIMBING) m_Climb->Stop();
    if (m_CurrentState != MarioState::DEAD) m_Dead->Stop();

    // 根據面向 (m_Direction) 統一處理縮放邏輯
    MarioDIR renderDir = (m_Direction == MarioDIR::NONE) ? m_BackupDirection : m_Direction;
    float scaleX = (renderDir == MarioDIR::LEFT) ? marioScale : -1.0f * marioScale;

    // 根據現在的狀態決定顯示哪個圖層，並執行該狀態的特殊邏輯
    switch (m_CurrentState) {
        case MarioState::IDLE:
            m_Idle->SetVisible(true);
            m_Idle->SetScale({scaleX, marioScale});
            break;
        case MarioState::WALKING:
            m_Walk->SetVisible(true);
            m_Walk->SetScale({scaleX, marioScale});
            m_Walk->Play();
            break;
        case MarioState::JUMPING:
            m_Jump->SetVisible(true);
            m_Jump->SetScale({scaleX, marioScale});
            break;
        case MarioState::CLIMBING:
            m_Climb->SetVisible(true);
            m_Climb->Play();
            break;
        case MarioState::CLIMB_IDLE:
            m_Climb->SetVisible(true);
            break;
        case MarioState::FALLING:
            // TODO: 如果有 FALL 圖層，在這裡顯示
            break;
        case MarioState::HAMMERING:
            m_Hammer->SetVisible(true);
            m_Hammer->Play();

            // 處理計時器
            m_HammerTimer -= 1.0f; // 如果是用 deltaTime 可以改成 m_HammerTimer -= deltaTime;
            if (m_HammerTimer <= 0) {
                LOG_DEBUG("HAMMER EXPIRED");
                SetState(MarioState::IDLE);
            }
            m_Hammer->SetScale({scaleX, marioScale});
            break;
        case MarioState::HAMMER_IDLE:
            // TODO: m_HammerIdle->SetVisible(true);
            break;
        case MarioState::DEAD:
            m_DeadTimer += 1.0f; // 同上，可考慮替換為 deltaTime

            if (m_DeadTimer < 30.0f) {
                m_Dead->SetVisible(true);
                m_Dead->Stop();
            }
            else if (m_DeadTimer < 102.0f) {
                m_Dead->SetVisible(true);
                if (!m_Dead->IsPlaying()) {
                    LOG_DEBUG("End");
                    m_Dead->SetLooping(true);
                    m_Dead->SetInterval(100);
                    m_Dead->Play();
                }
            }
            else {
                m_Dead->Stop();
                m_Dead->SetVisible(false);
                m_DiedFinal->SetVisible(true);
            }
            break;
        case MarioState::WIN:
            m_Idle->SetVisible(true);
            m_Idle->SetScale({scaleX, marioScale});
            break;
    }

    // 將所有動畫組件的座標與 Mario 本體同步
    // (這一段通常在 GameObject::Update() 或是你的 AddToRenderer 裡面會處理，
    // 若你有自定義的同步函式，也可以放在這裡呼叫)
}