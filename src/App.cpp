#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

// 地面上的槌子道具物件
static std::shared_ptr<Character> m_HammerItem;

void App::Start() {
    LOG_TRACE("Start");

    // 初始化 Mario 物件，並設定初始位置
    m_Mario = std::make_shared<Mario>();
    m_Mario->SetPosition({-112.5f, -140.5f});

    // 把 Mario 裡面所有的圖層一口氣加進 App 的Renderer中
    m_Mario->AddToRenderer(m_Renderer);

    // 初始化火球物件
    m_Fireball = std::make_shared<Fiamma>();
    m_Fireball->SetPosition({0.0f, 0.0f}); // 設定火球初始位置
    m_Renderer.AddChild(m_Fireball);

    // 初始化地面上的槌子道具並放在右側
    m_HammerItem = std::make_shared<Character>(RESOURCE_DIR"/Images/Hammer.png");
    m_HammerItem->SetPosition({150.0f, -140.5f});
    m_HammerItem->SetScale({m_Mario->marioScale, m_Mario->marioScale});
    m_Renderer.AddChild(m_HammerItem);

    // 設定 App 物件初始狀態為 UPDATE，開始遊戲主迴圈
    m_CurrentState = State::UPDATE;
}

void App::Update() {

    // 只有在 Mario 活著的時候才處理輸入與狀態轉換
    if (m_Mario->GetState() != MarioState::DEAD) {
        // 1. 偵測跳躍觸發
        if ((m_Mario->GetState() != MarioState::JUMPING)
            && (m_Mario->GetState() != MarioState::CLIMBING)
            && (m_Mario->GetState() != MarioState::HAMMERING) // 拿槌子時禁止跳躍
            && Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {
            m_Mario->JumpStart();
        }

        // 2. 根據狀態執行邏輯
        if (m_Mario->IsJumping()) {
            m_Mario->Jump();
        }
        // 3. 一般移動邏輯
        else {
            // 向上攀爬
            if (m_Mario->GetState() != MarioState::HAMMERING && Util::Input::IsKeyPressed(Util::Keycode::UP)) {
                m_Mario->Climb(CLIMB_DIR::UP);
            }
            // 向下攀爬
            else if (m_Mario->GetState() != MarioState::HAMMERING && Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
                m_Mario->Climb(CLIMB_DIR::DOWN);
            }
            // 放開上下鍵時，停止攀爬動畫
            else if (m_Mario->GetState() != MarioState::HAMMERING && 
                    (Util::Input::IsKeyUp(Util::Keycode::UP)
                     || Util::Input::IsKeyUp(Util::Keycode::DOWN))) {
                m_Mario->ClimbIdle();
            }

            // 向左移動
            else if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
                m_Mario->Walk(MarioDIR::LEFT);
            }
            // 向右移動
            else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
                m_Mario->Walk(MarioDIR::RIGHT);
            }
            // 放開左右鍵時，重置回靜止狀態
            else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::RIGHT)) {
                m_Mario->IDLE();
            }
        }
    }

    // 4. 顯示邏輯：根據目前的 marioState 決定顯示哪個物件, 以及是否播放動畫 (例如 WALKING 和 CLIMBING 是 AnimatedCharacter，需要呼叫 Play()，
    //    其他則是 Character，呼叫 Visible(true))
    // 注意：這裡的 Update() 是 Mario 類別裡的 Update()，它會根據當前狀態來決定哪個圖層要 Visible(true) 或 Play()，其他圖層則是 Visible(false) 
    // 或 Stop()。這樣的設計讓 App 的 Update() 可以專注在處理輸入和狀態轉換，而 Mario 的 Update() 則專注在根據狀態來控制圖層的顯示和動畫播放，
    // 達到職責分離。
    m_Mario->Update();

    // 更新火球邏輯（包含動畫切換與左右移動）
    m_Fireball->Update();

    // 5. 碰撞偵測：Mario 與火球 (只有在 Mario 還沒死的時候才偵測)
    if (m_Mario->GetState() != MarioState::DEAD &&
        m_Mario->GetState() != MarioState::HAMMERING && 
        m_Fireball->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
        LOG_DEBUG("DIED");
        m_Mario->Dead();
    }

    // 6. 道具偵測：撿起槌子
    if (m_HammerItem && m_HammerItem->GetVisibility() && m_Mario->GetState() != MarioState::DEAD) {
        if (m_HammerItem->IfCollides(m_Mario->GetPosition(), m_Mario->GetSize())) {
            m_Mario->Hammer();
            m_HammerItem->SetVisible(false); // 撿起後讓地面上的槌子消失
        }
    }

    // 在 PTSD 框架中，m_Renderer 是場景的根節點 (Renderer)，在每一幀(Frame)呼叫 m_Renderer.Update()，它會自動遞迴更新並繪製所有加入其中的子節點(Child)，
    // 因此我們不需要在這裡手動呼叫 每個物件的 Draw() 來繪製。只要確保在 Start() 中把 Mario 的所有圖層都加入 m_Renderer，並且在 Mario 的 Update() 中
    // 根據狀態來控制圖層的 Visible 和 Play，m_Renderer 就能讓整個場景正確顯示。
    m_Renderer.Update();


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
