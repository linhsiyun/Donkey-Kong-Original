#include "App.hpp"
#include "Map.hpp"
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    // 初始化地圖，並加入到 Renderer 渲染清單中
    m_Map = std::make_shared<Map>("../Resources/Images/DonkeyKongSprites2.png", "../Resources/Maps/Map1.txt");
    m_Renderer.AddChild(m_Map);

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    
    //TODO: do your things here and delete this line <3
    // 告訴渲染器，把所有 AddChild 進來的物件畫到畫面上 (包含你的地圖)
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
