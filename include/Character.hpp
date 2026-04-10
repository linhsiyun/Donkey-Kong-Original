#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <string>
#include <cmath>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

class Character : public Util::GameObject {
public:
    // 建構子：初始化角色，傳入圖片的檔案路徑
    explicit Character(const std::string& ImagePath);

    // 禁用拷貝與移動建構子，以及拷貝與移動賦值運算子
    // 確保角色物件在記憶體中的唯一性，避免資源淺複製或重複釋放的問題
    Character(const Character&) = delete;

    Character(Character&&) = delete;

    Character& operator=(const Character&) = delete;

    Character& operator=(Character&&) = delete;

    // 取得目前角色使用的圖片檔案路徑
    [[nodiscard]] const std::string& GetImagePath() const { return m_ImagePath; }

    // 取得角色目前在畫面上的二維位置座標
    [[nodiscard]] const glm::vec2& GetPosition() const { return m_Transform.translation; }

    // 取得角色的長寬縮放比例
    [[nodiscard]] const glm::vec2& GetScale() const { return m_Transform.scale; }

    // 取得角色目前是否設定為可見狀態
    [[nodiscard]] bool GetVisibility() const { return m_Visible; }

    // 設定角色要顯示的圖片
    void SetImage(const std::string& ImagePath);

    // 設定角色在畫面上的位置座標
    void SetPosition(const glm::vec2& Position) { m_Transform.translation = Position; }

    // 設定角色的長寬縮放比例
    void SetScale(const glm::vec2& Scale) { m_Transform.scale = Scale; }

    // 取得角色縮放後的實際尺寸
    [[nodiscard]] glm::vec2 GetSize() const { return GetObjectSize(); }

    // 判斷此角色是否與另一個角色發生碰撞
    [[nodiscard]] bool IfCollides(const std::shared_ptr<Character>& other) const {
        // AABB (Axis-Aligned Bounding Box) collision detection.
        // This is more robust than a simple distance check because it considers
        // the actual size of the character images.
        const auto self_half_size = GetSize() / 2.0F;
        const auto other_half_size = other->GetSize() / 2.0F;

        // 取得自身與目標角色的中心點座標
        const auto& self_pos = GetPosition();
        const auto& other_pos = other->GetPosition();

        // 若兩者的 X 軸中心點距離小於半寬總和，且 Y 軸中心點距離小於半高總和，代表發生重疊（碰撞）
        return std::abs(self_pos.x - other_pos.x) < (self_half_size.x + other_half_size.x) &&
               std::abs(self_pos.y - other_pos.y) < (self_half_size.y + other_half_size.y);
    }

    // 判斷此角色是否與指定的座標與尺寸發生碰撞 (AABB)
    [[nodiscard]] bool IfCollides(const glm::vec2& otherPos, const glm::vec2& otherSize) const {
        const auto self_half_size = GetSize() / 2.0F;
        const auto other_half_size = otherSize / 2.0F;
        const auto& self_pos = GetPosition();
        return std::abs(self_pos.x - otherPos.x) < (self_half_size.x + other_half_size.x) &&
               std::abs(self_pos.y - otherPos.y) < (self_half_size.y + other_half_size.y);
    }

    // TODO: 可根據需求加入更多方法與屬性以完善遊戲功能

private:
    // 將角色的位置重置回到原點 (0, 0)
    void ResetPosition() { m_Transform.translation = {0, 0}; }

    // 取得角色圖片的實際解晰度/尺寸（寬與高）
    // 透過將 GameObject 內部的 m_Drawable 強制轉型為 Util::Image 來獲取正確的大小
    // 同時考慮到 Scale 縮放
    [[nodiscard]] glm::vec2 GetObjectSize() const {
        return std::dynamic_pointer_cast<Util::Image>(m_Drawable)->GetSize() * glm::abs(m_Transform.scale);
    }

    // 紀錄目前的圖片檔案路徑
    std::string m_ImagePath;
};


#endif //CHARACTER_HPP
