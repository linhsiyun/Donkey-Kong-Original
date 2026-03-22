#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <string>
#include <cmath>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"

class Character : public Util::GameObject {
public:
    explicit Character(const std::string& ImagePath);

    Character(const Character&) = delete;

    Character(Character&&) = delete;

    Character& operator=(const Character&) = delete;

    Character& operator=(Character&&) = delete;

    [[nodiscard]] const std::string& GetImagePath() const { return m_ImagePath; }

    [[nodiscard]] const glm::vec2& GetPosition() const { return m_Transform.translation; }

    [[nodiscard]] bool GetVisibility() const { return m_Visible; }

    void SetImage(const std::string& ImagePath);

    void SetPosition(const glm::vec2& Position) { m_Transform.translation = Position; }

    void SetScale(const glm::vec2& Scale) { m_Transform.scale = Scale; }

    [[nodiscard]] bool IfCollides(const std::shared_ptr<Character>& other) const {
        // AABB (Axis-Aligned Bounding Box) collision detection.
        // This is more robust than a simple distance check because it considers
        // the actual size of the character images.
        const auto self_half_size = GetObjectSize() / 2.0F;
        const auto other_half_size = other->GetObjectSize() / 2.0F;
        const auto& self_pos = GetPosition();
        const auto& other_pos = other->GetPosition();

        return std::abs(self_pos.x - other_pos.x) < (self_half_size.x + other_half_size.x) &&
               std::abs(self_pos.y - other_pos.y) < (self_half_size.y + other_half_size.y);
    }

    // TODO: Add and implement more methods and properties as needed to finish Giraffe Adventure.

private:
    void ResetPosition() { m_Transform.translation = {0, 0}; }

    [[nodiscard]] glm::vec2 GetObjectSize() const {
        return std::dynamic_pointer_cast<Util::Image>(m_Drawable)->GetSize();
    }

    std::string m_ImagePath;
};


#endif //CHARACTER_HPP
