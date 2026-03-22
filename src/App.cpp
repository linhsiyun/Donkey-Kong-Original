#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    m_Mario = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk0.png");
    m_Mario1 = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk1.png");
    m_Mario2 = std::make_shared<Character>(RESOURCE_DIR"/Images/Walk2.png");
    m_Mario->SetPosition({-112.5f, -140.5f});
    m_Mario->SetZIndex(50);
    m_Mario->SetScale({2.5f, 2.5f});
    m_Mario1->SetVisible(true);
    m_Root.AddChild(m_Mario);

    m_Mario1->SetPosition({-112.5f, -140.5f});
    m_Mario1->SetZIndex(50);
    m_Mario1->SetVisible(false);
    m_Mario1->SetScale({2.5f, 2.5f});
    m_Root.AddChild(m_Mario1);

    m_Mario2->SetPosition({-112.5f, -140.5f});
    m_Mario2->SetZIndex(50);
    m_Mario2->SetVisible(false);
    m_Mario2->SetScale({2.5f, 2.5f});
    m_Root.AddChild(m_Mario2);

    m_CurrentState = State::UPDATE;
}

void App::Update() {

    //TODO: do your things here
    auto marioPosition = m_Mario->GetPosition();
    const float step = 1.0F;
    if (Util::Input::IsKeyPressed(Util::Keycode::UP)) {
        marioPosition.y += step;
    }
    else if (Util::Input::IsKeyPressed(Util::Keycode::DOWN)) {
        marioPosition.y -= step;
    }
    else if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
        static int frameCounter = 0;
        if (marioState == 0) {
            marioState = 1;
        } else if (frameCounter++ > 15) {
            marioState = (marioState == 1) ? 2 : 1;
            frameCounter = 0;
        }
        marioPosition.x -= step;
        m_Mario->SetScale({2.5f, 2.5f});
        m_Mario1->SetScale({2.5f, 2.5f});
        m_Mario2->SetScale({2.5f, 2.5f});
    }
    else if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
        static int frameCounter = 0;
        if (marioState == 0) {
            marioState = 1;
        } else if (frameCounter++ > 15) {
            marioState = (marioState == 1) ? 2 : 1;
            frameCounter = 0;
        }
        marioPosition.x += step;
        m_Mario->SetScale({-2.5f, 2.5f});
        m_Mario1->SetScale({-2.5f, 2.5f});
        m_Mario2->SetScale({-2.5f, 2.5f});
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::RIGHT)) {
        marioState = 0;
    }
    m_Mario->SetPosition(marioPosition);
    m_Mario1->SetPosition(marioPosition);
    m_Mario2->SetPosition(marioPosition);
    if (marioState==1) {
        m_Mario->SetVisible(false);
        m_Mario1->SetVisible(true);
        m_Mario2->SetVisible(false);
    }
    else if (marioState==2){
        m_Mario2->SetVisible(true);
        m_Mario1->SetVisible(false);
        m_Mario->SetVisible(false);
    }
    else {
        m_Mario->SetVisible(true);
        m_Mario1->SetVisible(false);
        m_Mario2->SetVisible(false);
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
