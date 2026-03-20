#include "Map.hpp"
#include "SpriteDict.hpp"
#include "Util/LoadTextFile.hpp"
#include <sstream>

std::vector<std::shared_ptr<Util::GameObject>> Map::LoadMap(const std::string& filepath) {
    m_Tiles.clear();

    // 呼叫底層的 LoadTextFile 讀取整個 txt 檔
    std::string fileContent = Util::LoadTextFile(filepath);
    std::stringstream ss(fileContent);

    // 計算偏移量 (假設 1280x720)
    const int offsetX = (1280 - (MAP_COLS * TILE_SIZE)) / 2; // 算出為 85
    
    // 注意：在多數 OpenGL 2D 封裝中，(0,0) 通常在畫面正中心。
    // 如果你的系統 (0,0) 是左上角，請把 originX 改為 0，originY 改為 0。
    // 這裡先假設 (0,0) 在中心，所以左上角座標是 (-640, 360)。
    const float originX = -640.0f + offsetX;
    const float originY = 360.0f;

    int tileNumber;
    int row = 0, col = 0;

    // 逐一讀取數字，直到檔案結束
    while (ss >> tileNumber) {
        // 如果不是空氣，也不是生成點，才需要畫出來
        if (tileNumber != TILE_EMPTY && tileNumber < TILE_SPAWN_DK) {
            
            auto tileObj = std::make_shared<Util::GameObject>();
            // 設定圖片
            std::string imagePath = SpriteDict::GetTilePath(tileNumber);
            if (!imagePath.empty()) {
                tileObj->SetDrawable(std::make_shared<Util::Image>(imagePath));
            }

            // 取得圖片的原始大小
            auto rawSize = tileObj->GetScaledSize();
            // 只計算高度的縮放倍率 (目標高度 30.0f / 圖片原始高度)
            // 這樣能保證所有方塊在垂直方向是對齊的
            float uniformScale = 30.0f / rawSize.y;
            //同時套用相同的倍率到 X 和 Y，維持原始比例
            tileObj->m_Transform.scale = glm::vec2(uniformScale, uniformScale);

            // 計算並設定座標 (依據你的系統原點可能需要微調 Y 軸是加或減)
            float posX = originX + (col * TILE_SIZE) + (TILE_SIZE / 2.0f);
            float posY = originY - (row * TILE_SIZE) - (TILE_SIZE / 2.0f);
            
            tileObj->m_Transform.translation = glm::vec2(posX, posY);
            tileObj->SetZIndex(0.0f); // 背景放在最底層

            m_Tiles.push_back(tileObj);
        }
        else if (tileNumber == TILE_SPAWN_DK) {
            // TODO: 記錄玩家出生座標，等待實作 Player 類別, 原本是TILE_PLAYER之後再定義
        }

        col++;
        if (col >= MAP_COLS) {
            col = 0;
            row++;
        }
    }

    return m_Tiles;
}