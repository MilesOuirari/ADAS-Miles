#ifndef HMI_CAMERA_HPP
#define HMI_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace adas::hmi {

class Camera {
public:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    
    // Follow settings
    float distance = 6.0f;
    float height = 2.5f;
    float smoothFactor = 0.1f; // Lerp factor

    Camera() : position(0, 5, 10), target(0, 0, 0), up(0, 1, 0) {}

    void updateFollow(const glm::vec3& carPos, float carYaw) {
        // Calculate desired position based on car yaw
        // In our system: -Z is forward?
        // Let's assume input carYaw is 0 when facing -Z.
        
        float offsetX = distance * std::sin(carYaw);
        float offsetZ = distance * std::cos(carYaw);
        
        glm::vec3 desiredPos = carPos + glm::vec3(offsetX, height, offsetZ);
        
        // Lerp position
        position = glm::mix(position, desiredPos, smoothFactor);
        
        // Look slightly ahead of the car
        glm::vec3 desiredTarget = carPos + glm::vec3(-std::sin(carYaw)*5.0f, 0.0f, -std::cos(carYaw)*5.0f);
        target = glm::mix(target, desiredTarget, smoothFactor);
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, target, up);
    }
    
    glm::mat4 getProjectionMatrix(float aspect) {
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    }
};

} // namespace adas::hmi

#endif // HMI_CAMERA_HPP
