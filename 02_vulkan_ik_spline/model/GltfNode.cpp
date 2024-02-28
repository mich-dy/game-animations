#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GltfNode.h"
#include "Logger.h"

GltfNode::~GltfNode() {
  Logger::log(2, "%s: removing node number %i (%s)\n", __FUNCTION__, mNodeNum, mNodeName.c_str());
}

std::shared_ptr<GltfNode> GltfNode::createRoot(int rootNodeNum) {
  std::shared_ptr<GltfNode> mParentNode = std::make_shared<GltfNode>();
  mParentNode->mNodeNum = rootNodeNum;
  return mParentNode;
}

void GltfNode::addChilds(std::vector<int> childNodes) {
  for (const int childNode : childNodes) {
    std::shared_ptr<GltfNode> child = std::make_shared<GltfNode>();
    child->mNodeNum = childNode;
    child->mParentNode = shared_from_this();

    mChildNodes.push_back(child);
  }
}

std::shared_ptr<GltfNode> GltfNode::getParentNode() {
  std::shared_ptr<GltfNode> pNode = mParentNode.lock();
  if (pNode) {
    return pNode;
  }
  return nullptr;
}

std::vector<std::shared_ptr<GltfNode>> GltfNode::getChilds() {
  return mChildNodes;
}

void GltfNode::updateNodeAndChildMatrices() {
  /* update this matrix */
  calculateNodeMatrix();

  /* call recursive for child update */
  for (auto &node : mChildNodes) {
    if (node) {
      node->updateNodeAndChildMatrices();
    }
  }
}

int GltfNode::getNodeNum() {
  return mNodeNum;
}

void GltfNode::setNodeName(std::string name) {
  mNodeName = name;
}

std::string GltfNode::getNodeName() {
  return mNodeName;
}

void GltfNode::setScale(glm::vec3 scale) {
  mScale = scale;
  mBlendScale = scale;
}

void GltfNode::setTranslation(glm::vec3 translation) {
  mTranslation = translation;
  mBlendTranslation = translation;
}

void GltfNode::setRotation(glm::quat rotation) {
  mRotation = rotation;
  mBlendRotation = rotation;
}

void GltfNode::blendScale(glm::vec3 scale, float blendFactor) {
  float factor = std::clamp(blendFactor, 0.0f, 1.0f);
  mBlendScale = scale * factor + mScale * (1.0f - factor);
}

void GltfNode::blendTranslation(glm::vec3 translation, float blendFactor) {
  float factor = std::clamp(blendFactor, 0.0f, 1.0f);
  mBlendTranslation = translation * factor + mTranslation * (1.0f - factor) ;
}

void GltfNode::blendRotation(glm::quat rotation, float blendFactor) {
  float factor = std::clamp(blendFactor, 0.0f, 1.0f);
  mBlendRotation = glm::slerp(mRotation, rotation, factor);
}

void GltfNode::calculateLocalTRSMatrix() {
  glm::mat4 sMatrix = glm::scale(glm::mat4(1.0f), mBlendScale);
  glm::mat4 rMatrix = glm::mat4_cast(mBlendRotation);
  glm::mat4 tMatrix = glm::translate(glm::mat4(1.0f), mBlendTranslation);

  mLocalTRSMatrix = tMatrix * rMatrix * sMatrix;
}

void GltfNode::calculateNodeMatrix() {
  calculateLocalTRSMatrix();

  /* default is identity matrix */
  glm::mat4 parentNodeMatrix = glm::mat4(1.0f);

  std::shared_ptr<GltfNode> pNode = mParentNode.lock();
  if (pNode) {
    parentNodeMatrix = pNode->getNodeMatrix();
  }

  mNodeMatrix = parentNodeMatrix * mLocalTRSMatrix;
}

glm::mat4 GltfNode::getNodeMatrix() {
  return mNodeMatrix;
}

glm::quat GltfNode::getLocalRotation() {
  return mBlendRotation;
}

glm::quat GltfNode::getGlobalRotation() {
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;

  if (!glm::decompose(mNodeMatrix, scale, orientation, translation, skew, perspective)) {
    Logger::log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__,
      mNodeNum);
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  }

  return glm::inverse(orientation);
}

glm::vec3 GltfNode::getGlobalPosition() {
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;

  if (!glm::decompose(mNodeMatrix, scale, orientation, translation, skew, perspective)) {
    Logger::log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__,
      mNodeNum);
    return glm::vec3(0.0f, 0.0f, 0.0f);
  }

  return translation;
}

void GltfNode::printTree() {
  Logger::log(1, "%s: ---- tree ----\n", __FUNCTION__);
  Logger::log(1, "%s: parent : %i (%s)\n", __FUNCTION__, mNodeNum, mNodeName.c_str());
  for (const auto& childNode : mChildNodes) {
    GltfNode::printNodes(childNode, 1);
  }
  Logger::log(1, "%s: -- end tree --\n", __FUNCTION__);
}

void GltfNode::printNodes(std::shared_ptr<GltfNode> node, int indent) {
  std::string indendString = "";
  for (int i = 0; i < indent; ++i) {
    indendString += " ";
  }
  indendString += "-";
  Logger::log(1, "%s: %s child : %i (%s)\n", __FUNCTION__,
    indendString.c_str(), node->mNodeNum, node->mNodeName.c_str());

  for (const auto& childNode : node->mChildNodes) {
    GltfNode::printNodes(childNode, indent + 1);
  }
}
