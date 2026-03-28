#ifndef ANIMATED_CHARACTER_HPP
#define ANIMATED_CHARACTER_HPP

#include <vector>
#include <string>

#include "Util/Animation.hpp"
#include "Util/GameObject.hpp"


class AnimatedCharacter : public Util::GameObject {

public:
    // 建構子：初始化動畫角色
    // 傳入一個包含多張圖片路徑的 vector，用來建立連續動畫
    // 參數說明：false (不自動播放), 500 (預設每幀間隔 500ms), false (預設不循環), 0 (從第 0 幀開始)
    explicit AnimatedCharacter(const std::vector<std::string>& AnimationPaths) {
        m_Drawable = std::make_shared<Util::Animation>(AnimationPaths, false, 500, false, 0);
    }

    // 取得目前動畫是否設定為循環播放
    [[nodiscard]] bool IsLooping() const {
        return std::dynamic_pointer_cast<Util::Animation>(m_Drawable)->GetLooping();
    }

    // 取得目前動畫是否正在播放中
    [[nodiscard]] bool IsPlaying() const {
        return std::dynamic_pointer_cast<Util::Animation>(m_Drawable)->GetState() == Util::Animation::State::PLAY;
    }

    // 設定動畫是否要循環播放
    // looping: true 代表循環播放，false 代表播放一次即停止
    void SetLooping(bool looping) {
        auto temp = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
        temp->SetLooping(looping);
    }

    // 設定動畫每一幀的播放間隔時間（單位：毫秒）
    void SetInterval(int interval) {
        auto temp = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
        temp->SetInterval(interval);
    }

    // 開始播放動畫
    void Play() {
        auto temp = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
        temp->Play();
    }

#if 1 // copied from Character.hpp
    // 取得角色目前在畫面上的二維位置座標
    [[nodiscard]] const glm::vec2& GetPosition() const { return m_Transform.translation; }

    // 取得角色目前的縮放比例
    [[nodiscard]] const glm::vec2& GetScale() const { return m_Transform.scale; }

    // 取得角色目前是否設定為可見狀態
    [[nodiscard]] bool GetVisibility() const { return m_Visible; }

    //void SetImage(const std::string& ImagePath);

    // 設定角色在畫面上的位置座標
    void SetPosition(const glm::vec2& Position) { m_Transform.translation = Position; }

    // 設定角色的長寬縮放比例
    void SetScale(const glm::vec2& Scale) { m_Transform.scale = Scale; }

    // 停止播放動畫，並將畫面重置回到第 0 幀（初始狀態）
    void Stop() {
        auto temp = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
        temp->Pause();
        temp->SetCurrentFrame(0);
    }
#endif

    // 判斷動畫是否已經完整播放結束
    // 條件 1：目前的幀數剛好等於最後一幀的索引 (總幀數 - 1)
    // 條件 2：目前的動畫狀態不是 PLAY（代表並非正在播放中）
    [[nodiscard]] bool IfAnimationEnds() const {
        auto animation = std::dynamic_pointer_cast<Util::Animation>(m_Drawable);
        return animation->GetCurrentFrameIndex() == animation->GetFrameCount() - 1 &&
               animation->GetState() != Util::Animation::State::PLAY;
    }

};

#endif //ANIMATED_CHARACTER_HPP
