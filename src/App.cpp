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

    /*測試用
    m_TestMarker = std::make_shared<Util::GameObject>();
    m_TestMarker->SetDrawable(std::make_shared<Util::Image>("../Resources/Images/fiamma2.png"));
    m_TestMarker->SetZIndex(5.0f); // 確保游標顯示在地圖上方
    m_Renderer.AddChild(m_TestMarker);
    */

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    
    //TODO: do your things here and delete this line <3

    /*測試用
    float speed = 5.0f;
    auto transform = m_TestMarker->GetTransform();
    if (Util::Input::IsKeyPressed(Util::Keycode::W)) { transform.translation.y += speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) { transform.translation.y -= speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) { transform.translation.x -= speed; }
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) { transform.translation.x += speed; }
    m_TestMarker->m_Transform = transform;
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        float x = transform.translation.x;
        float y = transform.translation.y;
        TileType tile = m_Map->GetTileAtPosition(x, y);
        LOG_INFO("游標座標: ({}, {}), 對應 TXT 代號: {}", x, y, static_cast<int>(tile));
    }
    */
    // 假設按下 Enter 鍵切換到第二關
    if (Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
        m_Map->LoadNewMap("../Resources/Images/dk.png", "level2.txt");
    }
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
