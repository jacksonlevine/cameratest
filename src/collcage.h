#ifndef COLLCAGE_H
#define COLLCAGE_H

#include <vector>
#include <glm/glm.hpp>
#include <functional>
#include <array>

class BoundingBox
{
public:
    BoundingBox(const glm::vec2 &m_min_corner, const glm::vec2 &m_max_corner, const glm::vec2 &collision_normal);
    BoundingBox(const glm::vec2 &center, const glm::vec2 &collision_normal);
    void set_center(const glm::vec2 &center);
    void set_center(const glm::vec2 &center, float xextent);
    bool intersects(const BoundingBox &other) const;
    double get_penetration(const BoundingBox &other) const;
    glm::vec2 collision_normal;
    glm::vec2 center;

private:
    glm::vec2 m_min_corner;
    glm::vec2 m_max_corner;
};

enum Side {
    LEFT,
    RIGHT,
    FRONT,
    BACK
};

class LilCollisionCage {
public:
    inline static std::vector<glm::ivec2> positions = {
        glm::ivec2(-1,0),
        glm::ivec2(1,0),
        glm::ivec2(0,1),
        glm::ivec2(0,-1)
    };

    inline static std::vector<glm::vec2> normals = {
        glm::vec2(1,0),
        glm::vec2(-1,0),
        glm::vec2(0,-1),
        glm::vec2(0,1)
    };

    glm::ivec2 position;
    std::array<BoundingBox, 4> boxes;
    std::array<float, 4> penetration;

    std::vector<Side> colliding;
    std::vector<Side> solid;
    
    LilCollisionCage(std::function<bool(int, int)> solidPredicate);
    std::function<bool(int,int)> solidPredicate;

    void update_position(glm::vec2 &pos);
    void update_solidity();
    void update_colliding(BoundingBox &user);
    void update_readings(glm::vec2 &pos);

};

#endif