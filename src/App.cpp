#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    // 1. 載入地圖並取得物件
    auto mapTiles = m_Map.LoadMap("../Resources/Maps/Map1.txt"); // 請換成你的 txt 路徑

    // 2. 把地圖物件交給 Renderer 管理
    m_Renderer.AddChildren(mapTiles);

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    
    //TODO: do your things here and delete this line <3

    // 讓 Renderer 每一幀自動畫出所有東西
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
