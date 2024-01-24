#include "collcage.h"

double BoundingBox::get_penetration(const BoundingBox &other) const
{
    if (!intersects(other))
    {
        return 0.0;
    }

    double x_penetration = std::min(m_max_corner.x - other.m_min_corner.x, other.m_max_corner.x - m_min_corner.x);
    double y_penetration = std::min(m_max_corner.y - other.m_min_corner.y, other.m_max_corner.y - m_min_corner.y);

    return std::min({x_penetration, y_penetration});
}

BoundingBox::BoundingBox(const glm::vec2 &m_min_corner, const glm::vec2 &m_max_corner, const glm::vec2 &collision_normal)
    : m_min_corner(m_min_corner), m_max_corner(m_max_corner), collision_normal(collision_normal), center(glm::mix(m_max_corner, m_min_corner, 0.5f))
{
}

BoundingBox::BoundingBox(const glm::vec2 &center, const glm::vec2 &collision_normal)
    : m_min_corner(center + glm::vec2(-0.5, -0.5)), m_max_corner(center + glm::vec2(0.5, 0.5)), collision_normal(collision_normal), center(center)
{
}

void BoundingBox::set_center(const glm::vec2 &center)
{
    this->m_min_corner = center + glm::vec2(-0.5, -0.5);
    this->m_max_corner = center + glm::vec2(0.5, 0.5);
    this->center = center;
}

void BoundingBox::set_center(const glm::vec2 &center, float xextent)
{
    this->m_min_corner = center + glm::vec2(-xextent, -xextent);
    this->m_max_corner = center + glm::vec2(xextent, xextent);
    this->center = center;
}

bool BoundingBox::intersects(const BoundingBox &other) const
{
    return !(m_max_corner.x < other.m_min_corner.x || m_min_corner.x > other.m_max_corner.x ||
             m_max_corner.y < other.m_min_corner.y || m_min_corner.y > other.m_max_corner.y);
}


void LilCollisionCage::update_position(glm::vec2 &pos)
{
    this->position = glm::ivec2(static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)));
    for (int i = 0; i < 4; ++i)
    {
        this->boxes[i].set_center(LilCollisionCage::positions[i] + this->position);
    }
}
void LilCollisionCage::update_solidity()
{
    this->solid.clear();
    
    for (int i = 0; i < 4; ++i)
    {
        glm::vec2 spot = this->boxes[i].center;

        Side side = static_cast<Side>(i);
        if (solidPredicate(spot.x, spot.y))
        {
            if(std::find(this->solid.begin(), this->solid.end(), side) == solid.end())
            {
                
                this->solid.push_back(side);
            }
                
        }
        else
        {
            this->solid.erase(std::remove(solid.begin(), solid.end(), side), solid.end());
        }
    }
}
void LilCollisionCage::update_colliding(BoundingBox &user)
{
    //RESET
    this->colliding.clear(); //Clear colliding and penetration
    for(int i = 0; i < 4; ++i)
    {
        this->penetration[i] = 0.0;
    }

    //RE-ASSESS
    for(Side& side : this->solid) //Look through solid boxes
    {
       

        if(user.intersects(this->boxes[side]))
        {
            if(std::find(colliding.begin(), colliding.end(), side) == colliding.end())
            {
                
                
                this->colliding.push_back(side);//Add to colliding
            }
            
            //std::cout << "side id: " << static_cast<int>(side) << std::endl;
            this->penetration[side] = user.get_penetration(boxes[side]); //Set penetration amount
        }
    }
    //std::cout << "colliding size: " <<  this->colliding.size() << std::endl;
}

LilCollisionCage::LilCollisionCage(std::function<bool(int, int)> solidPredicate)
    : solidPredicate(solidPredicate), boxes{
    BoundingBox(LilCollisionCage::positions[0], LilCollisionCage::normals[0]),
    BoundingBox(LilCollisionCage::positions[1], LilCollisionCage::normals[1]),
    BoundingBox(LilCollisionCage::positions[2], LilCollisionCage::normals[2]),
    BoundingBox(LilCollisionCage::positions[3], LilCollisionCage::normals[3]),

    }
{

    penetration = {
        0.0f, 0.0f, 0.0f, 0.0f
    };

}

void LilCollisionCage::update_readings(glm::vec2 &pos)
{
    this->update_position(pos);
    this->update_solidity();
    //this->debug_display();
}
