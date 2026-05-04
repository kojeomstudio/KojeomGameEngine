#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Kojeom
{
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

struct Transform
{
    Vec3 position = Vec3(0.0f);
    Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 scale = Vec3(1.0f);

    Mat4 ToMatrix() const
    {
        Mat4 T = glm::translate(Mat4(1.0f), position);
        Mat4 R = glm::toMat4(rotation);
        Mat4 S = glm::scale(Mat4(1.0f), scale);
        return T * R * S;
    }

    static Transform Identity()
    {
        return { Vec3(0.0f), Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(1.0f) };
    }
};

struct CameraData
{
    Mat4 viewMatrix = Mat4(1.0f);
    Mat4 projectionMatrix = Mat4(1.0f);
    Mat4 viewProjectionMatrix = Mat4(1.0f);
    Vec3 position = Vec3(0.0f);
    Vec3 forward = Vec3(0.0f, 0.0f, -1.0f);
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
    Vec3 right = Vec3(1.0f, 0.0f, 0.0f);
    float fov = 60.0f;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    void UpdateMatrices()
    {
        viewMatrix = glm::lookAt(position, position + forward, up);
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
        right = glm::normalize(glm::cross(forward, up));
    }
};
}
