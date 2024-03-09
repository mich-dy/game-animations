#include "WaterModel.h"
#include "Logger.h"

void WaterModel::setPosition(const glm::vec3 pos) {
  mPosition = pos;
}

glm::vec3 WaterModel::getPosition() const {
  return mPosition;
}

VkMesh WaterModel::getVertexData() {
  if (mVertexData.vertices.size() == 0) {
    init();
  }
  return mVertexData;
}

void WaterModel::init() {
  mVertexData.vertices.resize(6);

  /* top face only for now */
  mVertexData.vertices[0].position = glm::vec3( 5.0f,  0.0f,  5.0f);
  mVertexData.vertices[1].position = glm::vec3(-5.0f,  0.0f, -5.0f);
  mVertexData.vertices[2].position = glm::vec3(-5.0f,  0.0f,  5.0f);
  mVertexData.vertices[3].position = glm::vec3( 5.0f,  0.0f,  5.0f);
  mVertexData.vertices[4].position = glm::vec3( 5.0f,  0.0f, -5.0f);
  mVertexData.vertices[5].position = glm::vec3(-5.0f,  0.0f, -5.0f);

  mVertexData.vertices[0].color = glm::vec3(0.7f, 0.7f, 1.0f);
  mVertexData.vertices[1].color = glm::vec3(0.7f, 0.7f, 1.0f);
  mVertexData.vertices[2].color = glm::vec3(0.7f, 0.7f, 1.0f);
  mVertexData.vertices[3].color = glm::vec3(0.7f, 0.7f, 1.0f);
  mVertexData.vertices[4].color = glm::vec3(0.7f, 0.7f, 1.0f);
  mVertexData.vertices[5].color = glm::vec3(0.7f, 0.7f, 1.0f);

  mVertexData.vertices[0].uv = glm::vec2(0.0, 1.0);
  mVertexData.vertices[1].uv = glm::vec2(1.0, 0.0);
  mVertexData.vertices[2].uv = glm::vec2(0.0, 0.0);
  mVertexData.vertices[3].uv = glm::vec2(0.0, 1.0);
  mVertexData.vertices[4].uv = glm::vec2(1.0, 1.0);
  mVertexData.vertices[5].uv = glm::vec2(1.0, 0.0);

  Logger::log(1, "%s: WaterModel - loaded %d vertices\n", __FUNCTION__, mVertexData.vertices.size());
}
