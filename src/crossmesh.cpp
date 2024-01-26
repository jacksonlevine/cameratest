#include "crossmesh.h"

float Plane::rayIntersects(glm::vec2 rayDir, glm::vec2 rayOrigin) {
    glm::vec2 POPminusRO = point - rayOrigin;
    float PNdotPOPMRO = glm::dot(normal, POPminusRO);
    float PNdotRD = glm::dot(normal, rayDir);
    float travel = PNdotPOPMRO / PNdotRD;
    if(travel != 0 && travel > 0) {
        return travel;
    } else {
        return -1.0f;
    }
}

CrossMesh::CrossMesh(glm::vec2 position) {
    plane1.point = position;
    plane2.point = position;
    
    plane1.normal = glm::vec2(1.0f/std::sqrt(2.0f), 1.0f/std::sqrt(2.0f));
    
    plane2.normal = glm::vec2(-1.0f/std::sqrt(2.0f), 1.0f/std::sqrt(2.0f));

    plane1.cornerPoint = position + plane2.normal/2.0f;
    plane2.cornerPoint = position + plane1.normal/2.0f;
    
    plane1.width = 1.0f;
    plane2.width = 1.0f;
}

std::vector<Intersection> CrossMesh::getIntersections(glm::vec2 rayDir, glm::vec2 rayOrigin) {
    std::vector<Intersection> intersections;

   
    float int1 = plane1.rayIntersects(rayDir, rayOrigin);
    glm::vec2 point1 = rayOrigin + (rayDir * int1);

    float int2 = plane2.rayIntersects(rayDir, rayOrigin);
    glm::vec2 point2 = rayOrigin + (rayDir * int2);

    Intersection intersect1{
        int1,
        glm::distance(plane1.point, point1),
        glm::distance(plane1.cornerPoint, point1)
    };

    Intersection intersect2{
        int2, 
        glm::distance(plane2.point, point2),
        glm::distance(plane2.cornerPoint, point2)
    };

    if(int1 != -1.0f) {
        if(intersect1.distanceFromRefPoint <= plane1.width/2.0f)
            intersections.push_back(intersect1);
    }
    if(int2 != -1.0f && int2 != int1) {
        if(intersect2.distanceFromRefPoint <= plane2.width/2.0f)
            intersections.push_back(intersect2);
    }
    std::sort(intersections.begin(), intersections.end(), [](Intersection& a, Intersection& b){
        return a.travel < b.travel;
    });
    return intersections;
}