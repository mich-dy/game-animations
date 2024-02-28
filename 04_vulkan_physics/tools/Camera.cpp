#define _USE_MATH_DEFINES
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

void Camera::updateCamera(VkRenderData& renderData, const float deltaTime) {
  if (deltaTime == 0.0f) {
    return;
  }

  float azimRad = glm::radians(renderData.rdViewAzimuth);
  float elevRad = glm::radians(renderData.rdViewElevation);

  float sinAzim = glm::sin(azimRad);
  float cosAzim = glm::cos(azimRad);
  float sinElev = glm::sin(elevRad);
  float cosElev = glm::cos(elevRad);

  /* update view direction */
  mViewDirection = glm::normalize(glm::vec3(
     sinAzim * cosElev, sinElev, -cosAzim * cosElev));

  /* calculate right and up direction */
  mRightDirection = glm::normalize(glm::cross(mViewDirection, mWorldUpVector));
  mUpDirection = glm::normalize(glm::cross(mRightDirection, mViewDirection));

  /* update camera position depending on desired movement */
  renderData.rdCameraWorldPosition +=
    renderData.rdMoveForward * deltaTime * mViewDirection
    + renderData.rdMoveRight * deltaTime * mRightDirection
    + renderData.rdMoveUp * deltaTime * mUpDirection;
}


glm::mat4 Camera::getViewMatrix(VkRenderData &renderData) {
  return glm::lookAt(renderData.rdCameraWorldPosition,
    renderData.rdCameraWorldPosition + mViewDirection, mUpDirection);
}
