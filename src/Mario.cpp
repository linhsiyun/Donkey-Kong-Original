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
    m_LastPosition = position; // 初始化或強制位移時也同步更新上一幀位置
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

void Mario::SetScreenBounds(float halfWidth, float halfHeight) {
    m_ScreenHalfWidth = halfWidth;
    m_ScreenHalfHeight = halfHeight;
}

// 注意：標頭檔 (Mario.hpp) 記得要將 Update 改為接收這兩個參數
// void Update(float deltaTime, const std::shared_ptr<Map>& map);

void Mario::Update(float deltaTime, const std::shared_ptr<Map>& map) {
    // ==========================================
    // 1. 物理與預測座標運算 (Physics & Movement)
    // ==========================================

    // 【關鍵修正 1】：不要用 GetPosition()！
    // m_LastPosition 是真正的上一幀安全位置
    // m_Position 則是經過 Walk() 改變後的「這幀目標位置」
    glm::vec2 prevPos = m_LastPosition;
    glm::vec2 nextPos = m_Position;

    // 防止遊戲剛啟動或拖曳視窗時，巨大的時間差導致瑪利歐一幀移動幾萬像素（瞬間穿牆）
    if (deltaTime > 100.0f) {
        deltaTime = 16.0f;
    }
    float dtSec = deltaTime / 1000.0f;

    // ==========================================
    // 【階梯與斜坡適應 (Terrain Adaptation)】
    // ==========================================
    if (m_CurrentState == MarioState::IDLE || m_CurrentState == MarioState::WALKING) {

        float marioHalfWidth = GetSize().x / 2.0f;
        float marioHalfHeight = GetSize().y / 2.0f;
        const float MAX_STEP_HEIGHT = GetSize().y * 0.4f;

        if (map != nullptr) {
            // ------------------------------------------------
            // 第一階段：上坡抬腳 (Step Up) & 撞牆阻擋
            // ------------------------------------------------
            // 【關鍵修正 2】：探測 X 座標要往前推！看瑪利歐的「前半身」而不是中心點
            float checkFrontX = nextPos.x;
            if (m_Direction == MarioDIR::RIGHT) checkFrontX += (marioHalfWidth * 0.8f);
            else if (m_Direction == MarioDIR::LEFT) checkFrontX -= (marioHalfWidth * 0.8f);

            float checkWallY = nextPos.y - marioHalfHeight + 2.0f; // 腳底往上一點點
            TileType frontTile = map->GetTileAtPosition(checkFrontX, checkWallY);

            if (frontTile == TileType::FLOOR) {
                // 前方有障礙物！檢查它是不是「矮階梯/斜坡」
                bool canStepUp = false;
                float stepScanY = checkWallY;
                float maxStepY = checkWallY + MAX_STEP_HEIGHT;
                // 精準紀錄掃描到的「最高實體地板」
                float highestFloorY = checkWallY;

                while (stepScanY <= maxStepY) {
                    if (map->GetTileAtPosition(checkFrontX, stepScanY) != TileType::FLOOR) {
                        canStepUp = true;
                        float surfaceY = map->GetGridSurfaceY(stepScanY - 1.0f);
                        nextPos.y = surfaceY + marioHalfHeight;
                        m_CurrentFloorY = surfaceY; // 【修正】只記憶真實表面高度
                        break;
                    }
                    stepScanY += 1.0f;
                }

                // 如果往上找了 MAX_STEP_HEIGHT 都還是牆壁，代表這是一堵高牆，不是斜坡
                if (!canStepUp) {
                    // 退回真正的安全 X 座標，完美擋住！
                    nextPos.x = prevPos.x;
                }
            }

            // ------------------------------------------------
            // 第二階段：下坡吸附 (Step Down) & 踩空掉落
            // ------------------------------------------------
            float currentFootY = nextPos.y - marioHalfHeight;
            bool foundFloor = false;

            // 下坡繼續用「中心點 (nextPos.x)」來探測，這樣瑪利歐重心過半才會往下走，手感更真實
            float groundScanY = currentFootY + 2.0f;
            float dropLimitY = currentFootY - MAX_STEP_HEIGHT;

            while (groundScanY >= dropLimitY) {
                if (map->GetTileAtPosition(nextPos.x, groundScanY) == TileType::FLOOR) {
                    foundFloor = true;
                    float surfaceY = map->GetGridSurfaceY(groundScanY);
                    nextPos.y = surfaceY + marioHalfHeight;
                    m_CurrentFloorY = surfaceY; // 【修正】
                    break;
                }
                groundScanY -= 1.0f;
            }

            if (!foundFloor) {
                SetState(MarioState::FALLING);
            }
        }
    }

    // 1.5 攀爬狀態邊界與著陸檢查 (Ladder Bounds)
    if (m_CurrentState == MarioState::CLIMBING || m_CurrentState == MarioState::CLIMB_IDLE) {
        if (map != nullptr) {
            float marioHalfWidth = GetSize().x / 2.0f;
            float marioHalfHeight = GetSize().y / 2.0f;
            float footY = nextPos.y - marioHalfHeight;

            // 取得腳底目前所在的格子，以及稍微往下探測的格子
            TileType footTile = map->GetTileAtPosition(nextPos.x, footY);
            TileType belowTile = map->GetTileAtPosition(nextPos.x, footY - 2.0f);

            // --- A. 往上爬 (Up) ---
            if (nextPos.y > prevPos.y) {
                if (footTile != TileType::LADDER && footTile != TileType::BROKEN_LADDER) {
                    TileType leftTile = map->GetTileAtPosition(nextPos.x - marioHalfWidth, footY - 2.0f);
                    TileType rightTile = map->GetTileAtPosition(nextPos.x + marioHalfWidth, footY - 2.0f);

                    if (belowTile == TileType::FLOOR || leftTile == TileType::FLOOR || rightTile == TileType::FLOOR || footTile == TileType::FLOOR) {
                        SetState(MarioState::IDLE);
                        float surfaceY = map->GetGridSurfaceY(footTile == TileType::FLOOR ? footY : footY - 2.0f);
                        nextPos.y = surfaceY + marioHalfHeight;
                        m_CurrentFloorY = surfaceY; // 【修正】
                    } else {
                        nextPos.y = prevPos.y;
                    }
                }
            }
            // --- B. 往下爬 (Down) ---
            else if (nextPos.y < prevPos.y) {
                bool landed = false;
                float targetSurfaceY = 0.0f;

                if (footTile == TileType::FLOOR) {
                    float surfY = map->GetGridSurfaceY(footY);
                    if (surfY < m_CurrentFloorY - 5.0f) { // 現在條件不會被誤判了！
                        landed = true;
                        targetSurfaceY = surfY;
                    }
                }
                else if (belowTile == TileType::FLOOR) {
                    float surfY = map->GetGridSurfaceY(footY - 2.0f);
                    if (surfY < m_CurrentFloorY - 5.0f) {
                        landed = true;
                        targetSurfaceY = surfY;
                    }
                }

                if (landed) {
                    SetState(MarioState::IDLE);
                    nextPos.y = targetSurfaceY + marioHalfHeight;
                    m_CurrentFloorY = targetSurfaceY; // 【修正】
                }
                // 防呆掉落邏輯保留不變
                // 離開梯子且沒有踩到新地板 -> 掉落
                // 【防呆】：加上 footTile != TileType::FLOOR，避免他剛往下爬還穿梭在「出發樓層」內部時，被誤判為離開梯子而墜落
                else if (footTile != TileType::LADDER && footTile != TileType::BROKEN_LADDER &&
                         belowTile != TileType::LADDER && belowTile != TileType::BROKEN_LADDER &&
                         footTile != TileType::FLOOR) {
                    SetState(MarioState::FALLING);
                         }
            }
        }
    }

    // 如果在空中，套用重力與垂直移動
    if (m_CurrentState == MarioState::JUMPING || m_CurrentState == MarioState::FALLING) {

        m_VelocityY -= m_Gravity * dtSec;
        // 空氣阻力限制：任何物體都不該掉得比這個速度更快，防止子彈穿紙效應
        if (m_VelocityY < -1500.0f) {
            m_VelocityY = -1500.0f;
        }
        nextPos.y += m_VelocityY * dtSec;

        // TODO: (如果你原本有寫水平跳躍移動，例如 nextPos.x += ... 記得加在這裡)

        // 往下掉落時才檢查是否踩到地板
        // 往下掉落時才檢查是否踩到地板
        if (m_VelocityY < 0.0f && map != nullptr) {
            float marioHalfHeight = GetSize().y / 2.0f;
            float currentFootY = nextPos.y - marioHalfHeight;
            float oldFootY     = prevPos.y - marioHalfHeight;

            bool hitFloor = false;
            float surfaceY = 0.0f;
            float checkY = oldFootY;

            // 【關鍵修改】：每隔 1.0f 像素往下細切掃描！
            // 這樣不管你的瑪利歐掉多快，或是地板多薄，絕對不可能「漏看」任何一層地板
            while (checkY >= currentFootY) {
                if (map->GetTileAtPosition(nextPos.x, checkY) == TileType::FLOOR) {
                    float tempSurfaceY = map->GetGridSurfaceY(checkY);

                    // ==========================================
                    // 【核心修正：大金剛專屬限制】
                    // 如果這層地板的高度，比我起跳前踩的樓層還要高
                    // (加 5.0f 是為了容許在地板上微小的高低差)，就直接無視它！
                    // ==========================================
                    if (tempSurfaceY > m_CurrentFloorY + 5.0f) {
                        checkY -= 1.0f;
                        continue; // 忽略這個地板，繼續往下掃描
                    }
                    hitFloor = true;
                    surfaceY = map->GetGridSurfaceY(checkY);
                    break;
                }
                checkY -= 1.0f; // 步長設為 1.0f，最為精準
            }

            // 防呆機制
            if (!hitFloor && map->GetTileAtPosition(nextPos.x, currentFootY) == TileType::FLOOR) {
                float tempSurfaceY = map->GetGridSurfaceY(currentFootY);
                if (tempSurfaceY <= m_CurrentFloorY + 5.0f) { // 確保不是更高的樓層
                    hitFloor = true;
                    surfaceY = tempSurfaceY;
                }
            }

            if (hitFloor) {
                if (oldFootY >= surfaceY && currentFootY <= surfaceY) {
                    m_VelocityY = 0.0f;
                    nextPos.y = surfaceY + marioHalfHeight;
                    m_CurrentFloorY = surfaceY; // 【新增】跳躍著陸也要更新樓層！
                    SetState(MarioState::IDLE);
                    m_Direction = m_BackupDirection;
                    m_IsJumping = false;
                }
            }
        }
    }
    // TODO: (如果你原本有寫走路的水平位移 nextPos.x += ...，記得寫在 else if (WALKING) 裡面)

    // ==========================================
    // 2. 邊界與實體碰撞校正
    // ==========================================
    const auto mario_half_size = GetSize() / 2.0f;

    // --- A. 螢幕邊界檢查 ---
    if (m_ScreenHalfWidth > 0 && m_ScreenHalfHeight > 0) {
        float limitX = m_ScreenHalfWidth - mario_half_size.x;
        float limitY = m_ScreenHalfHeight - mario_half_size.y;

        if (nextPos.x > limitX) nextPos.x = limitX;
        if (nextPos.x < -limitX) nextPos.x = -limitX;
        if (nextPos.y > limitY) nextPos.y = limitY;
        if (nextPos.y < -limitY) nextPos.y = -limitY;
    }

    // --- B. Donkey Kong 碰撞檢查 ---
    if (m_DonkeyKongSize.x > 0 && m_DonkeyKongSize.y > 0) {
        const auto dk_half_size = m_DonkeyKongSize / 2.0f;

        bool collideX = std::abs(nextPos.x - m_DonkeyKongPos.x) < (mario_half_size.x + dk_half_size.x);
        bool collideY = std::abs(nextPos.y - m_DonkeyKongPos.y) < (mario_half_size.y + dk_half_size.y);

        if (collideX && collideY) {
            nextPos = m_LastPosition; // 發生重疊，直接退回上一幀的安全位置
        }
    }

    // ==========================================
    // 3. 寫回最終座標
    // ==========================================
    m_LastPosition = prevPos;   // 紀錄真實的上一幀位置
    m_Position = nextPos;       // 同步成員變數
    SetPosition(m_Position);    // 更新底層 Transform

    // ==========================================
    // 4. 視覺與動畫圖層管理 (恢復你的完美 Switch)
    // ==========================================

    // 先把所有人隱藏
    m_Idle->SetVisible(false);
    m_Walk->SetVisible(false);
    m_Climb->SetVisible(false);
    m_Jump->SetVisible(false);
    m_Hammer->SetVisible(false);
    m_Dead->SetVisible(false);
    m_DiedFinal->SetVisible(false);
    if (m_Fall) m_Fall->SetVisible(false);
    if (m_HammerIdle) m_HammerIdle->SetVisible(false);
    if (m_Win) m_Win->SetVisible(false);

    // 停止不該播放的動畫
    if (m_CurrentState != MarioState::WALKING) m_Walk->Stop();
    if (m_CurrentState != MarioState::CLIMBING) m_Climb->Stop();
    if (m_CurrentState != MarioState::DEAD) m_Dead->Stop();

    // 根據面向統一處理縮放邏輯
    MarioDIR renderDir = (m_Direction == MarioDIR::NONE) ? m_BackupDirection : m_Direction;
    float scaleX = (renderDir == MarioDIR::LEFT) ? marioScale : -1.0f * marioScale;

    // 根據狀態設定可見圖層與播放控制
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
            if (m_Fall) {
                m_Fall->SetVisible(true);
                m_Fall->SetScale({scaleX, marioScale});
            } else {
                // 如果還沒設定 FALL 圖片，先用 Jump 代替避免隱形
                m_Jump->SetVisible(true);
                m_Jump->SetScale({scaleX, marioScale});
            }
            break;
        case MarioState::HAMMERING:
            m_Hammer->SetVisible(true);
            m_Hammer->Play();
            m_HammerTimer -= deltaTime;
            if (m_HammerTimer <= 0) {
                LOG_DEBUG("HAMMER EXPIRED");
                SetState(MarioState::IDLE);
            }
            m_Hammer->SetScale({scaleX, marioScale});
            break;
        case MarioState::HAMMER_IDLE:
            if (m_HammerIdle) {
                m_HammerIdle->SetVisible(true);
                m_HammerIdle->SetScale({scaleX, marioScale});
            }
            break;
        case MarioState::DEAD:
            // 原本用 += 1.0f，現在換成真實時間 deltaTime(毫秒)，所以判斷的閾值要等比例放大 (例如 *10)
            m_DeadTimer += deltaTime;
            if (m_DeadTimer < 300.0f) {
                m_Dead->SetVisible(true);
                m_Dead->Stop();
            }
            else if (m_DeadTimer < 1000.0f) {
                m_Dead->SetVisible(true);
                if (!m_Dead->IsPlaying()) {
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
            if (m_Win) {
                m_Win->SetVisible(true);
                m_Win->SetScale({scaleX, marioScale});
            } else {
                m_Idle->SetVisible(true);
                m_Idle->SetScale({scaleX, marioScale});
            }
            break;
    }
}

void Mario::SetDonkeyKongBounds(const glm::vec2& dkPos, const glm::vec2& dkSize) {
    m_DonkeyKongPos = dkPos;
    m_DonkeyKongSize = dkSize;
}

bool Mario::CanClimbDown(const std::shared_ptr<Map>& map) const {
    if (map == nullptr) return false;

    float marioHalfWidth = GetSize().x / 4.0f;
    float footY = m_Position.y - (GetSize().y / 2.0f);

    // 往下掃描一個深度區間 (例如 10 到 50 像素)，確保穿透 30 像素厚的地板
    for (float yOffset = 10.0f; yOffset <= 50.0f; yOffset += 5.0f) {
        float checkY = footY - yOffset;

        TileType center = map->GetTileAtPosition(m_Position.x, checkY);
        TileType left   = map->GetTileAtPosition(m_Position.x - marioHalfWidth, checkY);
        TileType right  = map->GetTileAtPosition(m_Position.x + marioHalfWidth, checkY);

        if (center == TileType::LADDER || center == TileType::BROKEN_LADDER ||
            left   == TileType::LADDER || left   == TileType::BROKEN_LADDER ||
            right  == TileType::LADDER || right  == TileType::BROKEN_LADDER) {

            LOG_DEBUG("[Physics] 透視眼探測到下方有梯子，Offset: {}", yOffset);
            return true;
            }
    }
    return false;
}