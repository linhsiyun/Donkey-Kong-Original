#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    // 初始化 Mario 靜止狀態 (Character)
    m_Mario = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk0.png");
    m_Mario->SetPosition({-112.5f, -140.5f});
    m_Mario->SetZIndex(50);
    m_Mario->SetScale({2.5f, 2.5f});
    m_Mario->SetVisible(true);
    m_Root.AddChild(m_Mario);

    // 初始化 Mario 行走動畫 (AnimatedCharacter)
    std::vector<std::string> marioWalkImages;
    marioWalkImages.reserve(2);
    for (int i = 0; i < 2; ++i) {
        marioWalkImages.emplace_back(RESOURCE_DIR"/Images/Walk" + std::to_string(i + 1) + ".png");
    }
    m_MarioWalk = std::make_shared<AnimatedCharacter>(marioWalkImages);
    m_MarioWalk->SetZIndex(50);
    m_MarioWalk->SetVisible(false);
    m_Root.AddChild(m_MarioWalk);

    // 初始化 Mario 攀爬動畫 (AnimatedCharacter)
    std::vector<std::string> marioClimbImages;
    marioClimbImages.reserve(2);
    for (int i = 0; i < 2; ++i) {
        marioClimbImages.emplace_back(RESOURCE_DIR"/Images/Climbing" + std::to_string(i + 1) + ".png");
    }
    m_MarioClimb = std::make_shared<AnimatedCharacter>(marioClimbImages);
    m_MarioClimb->SetZIndex(50);
    m_MarioClimb->SetVisible(false);
    m_MarioClimb->SetScale({2.5f, 2.5f});
    m_Root.AddChild(m_MarioClimb);

    // 初始化 Mario 跳躍狀態 (Character) - 這裡使用靜態圖片
    m_MarioJump = std::make_shared<Character>(RESOURCE_DIR"/Images/Jump.png");
    m_MarioJump->SetPosition({-112.5f, -140.5f});
    m_MarioJump->SetZIndex(50);
    m_MarioJump->SetVisible(false);
    m_MarioJump->SetScale({2.5f, 2.5f});
    m_Root.AddChild(m_MarioJump);

    // 初始化跳躍狀態，確保一開始不會誤動作
    m_IsJumping = false;
    m_JumpTimer = 0.0f;
    m_JumpDirection = 0;
    m_CurrentState = State::UPDATE;
}

void App::Update() {

    auto marioPosition = m_Mario->GetPosition();
    const float movingStep = 2.0F;  // 走路與跳躍速度
    const float climbingStep = 1.0F; // 攀爬速度

    // TODO: MarioState::HAMMERING, MarioState::FALLING

    // 1. 偵測跳躍觸發
    // 條件：不在跳躍中 且 沒按下攀爬(狀態非3或4) 且 按下空白鍵
    if (!m_IsJumping && (marioState != MarioState::CLIMBING) && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
        m_IsJumping = true;
        m_JumpStartPosition = marioPosition;
        m_JumpTimer = 0.0f;
        m_JumpDirection = 0;

        // 偵測是否同時按住左右鍵
        if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) m_JumpDirection = 1;
        if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) m_JumpDirection = -1;

        // 如果是垂直跳躍，讓跳躍貼圖的面向跟隨目前 Mario 的面向
        if (m_JumpDirection == 0 && m_Mario->GetScale().x < 0) {
             m_MarioJump->SetScale({-2.5f, 2.5f});
        } else if (m_JumpDirection == 0) { // 預設面向右
             m_MarioJump->SetScale({2.5f, 2.5f});
        }

        // 強制先切換狀態，讓這一幀就能看到改變
        LOG_DEBUG("JUMPING");
        marioState = MarioState::JUMPING;
    }

    // 2. 根據狀態執行邏輯
    if (m_IsJumping) {
        // 設定跳躍參數 (後續可以修改這裡的數值)
        const float totalJumpTime = 35.0f; // 跳躍總時間 (Frames) - 時間越短跳越快
        const float jumpHeight = 35.0f;    // 跳躍高度

        m_JumpTimer += 1.0f;

        if (m_JumpTimer > totalJumpTime) {
            m_IsJumping = false;
            marioPosition.y = m_JumpStartPosition.y; // 確保回到地面
            // 落地後不強制設為 0，讓後續輸入判斷決定是否要變成行走或站立
            // 這裡設回站立，避免落地瞬間還顯示跳躍圖
            LOG_DEBUG("IDLE");
            marioState = MarioState::IDLE;
            m_JumpTimer = 0.0f;
            m_JumpDirection = 0;
        } else {
            // 計算拋物線高度: 4 * h * t * (1-t)
            float progress = m_JumpTimer / totalJumpTime;
            float yOffset = 4.0f * jumpHeight * progress * (1.0f - progress);
            marioPosition.y = m_JumpStartPosition.y + yOffset;

            // 水平移動與面向設定
            if (m_JumpDirection != 0) {
                marioPosition.x += (float)m_JumpDirection * movingStep;
                // 根據移動方向翻轉貼圖 (負值為向左)
                float scaleX = (m_JumpDirection > 0) ? -2.5f : 2.5f;
                m_MarioJump->SetScale({scaleX, 2.5f});
            }
            LOG_DEBUG("JUMPING2");
            marioState = MarioState::JUMPING; // 設定為跳躍狀態
        }
    }
    // 3. 一般移動邏輯 (使用 else 確保只有在「非跳躍中」才執行)
    else {
        // 向上攀爬
        if (Util::Input::IsKeyPressed(Util::Keycode::UP)) {
            marioPosition.y += climbingStep;
            // 如果狀態切換為攀爬，開始播放動畫
            if (marioState!=MarioState::CLIMBING) {
                m_MarioClimb->SetLooping(true);
                m_MarioClimb->SetInterval(200);      // 每(x)ms更新動畫
                //m_MarioClimb->SetScale({2.5f, 2.5f});
                m_MarioClimb->Play();
                LOG_DEBUG("CLIMBING: UP");
                marioState = MarioState::CLIMBING;
                marioDir = MarioDIR::UPDOWN;
            }
        }
        // 向下攀爬
        else if (Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
            marioPosition.y -= climbingStep;
            // 如果狀態切換為攀爬，開始播放動畫
            if (marioState!=MarioState::CLIMBING) {
                m_MarioClimb->SetLooping(true);
                m_MarioClimb->SetInterval(200);      // 每(x)ms更新動畫
                //m_MarioClimb->SetScale({2.5f, 2.5f});
                m_MarioClimb->Play();
                LOG_DEBUG("CLIMB: DOWN");
                marioState = MarioState::CLIMBING;
                marioDir = MarioDIR::UPDOWN;
            }
        }
        // 放開上下鍵時，停止攀爬動畫
        else if (Util::Input::IsKeyUp(Util::Keycode::UP)
              || Util::Input::IsKeyUp(Util::Keycode::DOWN)) {
            LOG_DEBUG("CLIMB_IDLE");
            marioState = MarioState::CLIMB_IDLE;
            m_MarioClimb->Stop();
        }

        // 向左移動
        else if (Util::Input::IsKeyPressed(Util::Keycode::LEFT))  {

            marioPosition.x -= movingStep;
            // 如果狀態切換或方向改變，更新動畫設定
            if ((marioState!=MarioState::WALKING) || (marioDir!=MarioDIR::LEFT)) {
                m_Mario->SetScale({2.5f, 2.5f});    // m_Mario向左

                m_MarioWalk->SetLooping(true);
                m_MarioWalk->SetInterval(200);      // 每(x)ms更新動畫
                m_MarioWalk->SetScale({2.5f, 2.5f});
                m_MarioWalk->Play();
                LOG_DEBUG("WALKING: LEFT");
                marioState = MarioState::WALKING;
                marioDir = MarioDIR::LEFT;
            }
        }
        // 向右移動
        else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {

            marioPosition.x += movingStep;

            // 如果狀態切換或方向改變，更新動畫設定
            if ((marioState!=MarioState::WALKING) || (marioDir!=MarioDIR::RIGHT)) {
                m_Mario->SetScale({-2.5f, 2.5f});   // m_Mario向右

                m_MarioWalk->SetLooping(true);
                m_MarioWalk->SetInterval(200);      // 每(x)ms更新動畫
                m_MarioWalk->SetScale({-2.5f, 2.5f});
                m_MarioWalk->Play();
                LOG_DEBUG("WALKING: RIGHT");
                marioState = MarioState::WALKING;
                marioDir = MarioDIR::RIGHT;
            }
        }
        // 放開左右鍵時，重置回靜止狀態
        else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::RIGHT)) {
            LOG_DEBUG("IDLE");
            marioState = MarioState::IDLE;

            m_MarioWalk->Stop();
            m_MarioWalk->SetVisible(false);
            m_Mario->SetVisible(true);
        }
    }

    m_Mario->SetPosition(marioPosition);    // 每次 App::Update都是從m_Mario拿到Mario's position, m_Marioi一定要每次都更新position

    // 4. 顯示邏輯：根據目前的 marioState 決定顯示哪個物件 (Mario, Walk, Climb, Jump)
    // 攀爬中
    if ((marioState == MarioState::CLIMBING) || (marioState == MarioState::CLIMB_IDLE)) {
        m_Mario->SetVisible(false);
        m_MarioWalk->SetVisible(false);
        m_MarioJump->SetVisible(false);

        m_MarioClimb->SetPosition(marioPosition);
        m_MarioClimb->SetVisible(true);
    }
    // 跳躍中
    else if (marioState == MarioState::JUMPING) {
        m_Mario->SetVisible(false);
        m_MarioClimb->SetVisible(false);
        m_MarioWalk->SetVisible(false);

        m_MarioJump->SetPosition(marioPosition);
        m_MarioJump->SetVisible(true);
    }
    // 行走中
    else if (marioState == MarioState::WALKING){
        m_MarioWalk->SetPosition(marioPosition);
        m_Mario->SetVisible(false);
        m_MarioWalk->SetVisible(true);
        m_MarioClimb->SetVisible(false);
        m_MarioJump->SetVisible(false);
    }
    // 靜止 (IDLE) 或其他狀態
    else {
        m_MarioWalk->SetVisible(false);
        m_MarioClimb->SetVisible(false);
        m_MarioJump->SetVisible(false);

        m_Mario->SetVisible(true);
    }

    // 在 PTSD 框架中，m_Root 是場景的根節點 (Renderer)，必須在每一幀 (Frame) 都呼叫 Update()，
    // 它才會去繪製所有加入其中的角色 (如 Mario)。
    m_Root.Update();


    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
