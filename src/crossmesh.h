#ifndef CROSSMESH_H
#define CROSSMESH_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

struct Intersection {
    float travel;
    float distanceFromRefPoint;
    float distanceFromCornerPoint;
};

class Plane {
public:
    glm::vec2 point;
    glm::vec2 cornerPoint;
    glm::vec2 normal;
    float width;
    //Returns the travel if the ray intersects, or -1 if not
    float rayIntersects(glm::vec2 rayDir, glm::vec2 rayOrigin);
};

class CrossMesh {
public:
    Plane plane1;
    Plane plane2;
    CrossMesh(glm::vec2 position);
    //Returns the travels of any intersections, in ascending order
    std::vector<Intersection> getIntersections(glm::vec2 rayDir, glm::vec2 rayOrigin);
};

#endif