#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GltfModel.h"
#include "Logger.h"

bool GltfModel::loadModel(VkRenderData &renderData, VkGltfRenderData &gltfRenderData,
    std::string modelFilename, std::string textureFilename) {
  if (!Texture::loadTexture(renderData, gltfRenderData.rdGltfModelTexture, textureFilename)) {
    Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
    return false;
  }
  Logger::log(1, "%s: glTF model texture '%s' successfully loaded\n", __FUNCTION__,
    modelFilename.c_str());

  mModel = std::make_shared<tinygltf::Model>();

  tinygltf::TinyGLTF gltfLoader;
  std::string loaderErrors;
  std::string loaderWarnings;
  bool result = false;

  result = gltfLoader.LoadASCIIFromFile(mModel.get(), &loaderErrors, &loaderWarnings,
    modelFilename);

  if (!loaderWarnings.empty()) {
    Logger::log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
      loaderWarnings.c_str());
  }

  if (!loaderErrors.empty()) {
    Logger::log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
      loaderErrors.c_str());
  }

  if (!result) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
      modelFilename.c_str());
    return false;
  }

  createVertexBuffers(renderData, gltfRenderData);
  createIndexBuffer(renderData, gltfRenderData);

  /* extract joints, weights, and invers bind matrices*/
  getJointData();
  getWeightData();
  getInvBindMatrices();

  /* build model tree */
  renderData.rdModelNodeCount = mModel->nodes.size();
  int rootNode = mModel->scenes.at(0).nodes.at(0);
  Logger::log(1, "%s: model has %i nodes, root node is %i\n", __FUNCTION__,
    renderData.rdModelNodeCount, rootNode);

  mNodeList.resize(renderData.rdModelNodeCount);

  mRootNode = GltfNode::createRoot(rootNode);
  mNodeList.at(rootNode) = mRootNode;
  getNodeData(mRootNode);
  getNodes(mRootNode);

  /* init skeleton */
  mSkeletonMesh = std::make_shared<VkMesh>();
  mSkeletonMesh->vertices.resize(mModel->nodes.size() * 2);

  mRootNode->printTree();

  /* extract animation data */
  getAnimations();
  renderData.rdAnimClipSize = mAnimClips.size();

  mAdditiveAnimationMask.resize(renderData.rdModelNodeCount);
  mInvertedAdditiveAnimationMask.resize(renderData.rdModelNodeCount);

  std::fill(mAdditiveAnimationMask.begin(), mAdditiveAnimationMask.end(), true);
  mInvertedAdditiveAnimationMask = mAdditiveAnimationMask;
  mInvertedAdditiveAnimationMask.flip();

  for (const auto &clip : mAnimClips) {
    renderData.rdClipNames.push_back(clip->getClipName());
  }

  for (const auto &node : mNodeList) {
    if (node) {
      renderData.rdSkelNodeNames.push_back(node->getNodeName());
    } else {
      renderData.rdSkelNodeNames.push_back("(invalid)");
    }
  }

  renderData.rdGltfTriangleCount = getTriangleCount();

  return true;
}

void GltfModel::getJointData() {
  std::string jointsAccessorAttrib = "JOINTS_0";
  int jointsAccessor = mModel->meshes.at(0).primitives.at(0).attributes.at(jointsAccessorAttrib);
  Logger::log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, jointsAccessor,
    jointsAccessorAttrib.c_str());

  const tinygltf::Accessor &accessor = mModel->accessors.at(jointsAccessor);
  const tinygltf::BufferView &bufferView = mModel->bufferViews.at(accessor.bufferView);
  const tinygltf::Buffer &buffer = mModel->buffers.at(bufferView.buffer);

  int jointVecSize = accessor.count;
  Logger::log(1, "%s: %i short vec4 in JOINTS_0\n", __FUNCTION__, jointVecSize);
  mJointVec.resize(jointVecSize);

  std::memcpy(mJointVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
    bufferView.byteLength);

  mNodeToJoint.resize(mModel->nodes.size());

  const tinygltf::Skin &skin = mModel->skins.at(0);
  for (int i = 0; i < skin.joints.size(); ++i) {
    int destinationNode = skin.joints.at(i);
    mNodeToJoint.at(destinationNode) = i;
    Logger::log(2, "%s: joint %i affects node %i\n", __FUNCTION__, i, destinationNode);
  }
}

void GltfModel::getWeightData() {
  std::string weightsAccessorAttrib = "WEIGHTS_0";
  int weightAccessor = mModel->meshes.at(0).primitives.at(0).attributes.at(weightsAccessorAttrib);
  Logger::log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, weightAccessor,
    weightsAccessorAttrib.c_str());

  const tinygltf::Accessor &accessor = mModel->accessors.at(weightAccessor);
  const tinygltf::BufferView &bufferView = mModel->bufferViews.at(accessor.bufferView);
  const tinygltf::Buffer &buffer = mModel->buffers.at(bufferView.buffer);

  int weightVecSize = accessor.count;
  Logger::log(1, "%s: %i vec4 in WEIGHTS_0\n", __FUNCTION__, weightVecSize);
  mWeightVec.resize(weightVecSize);

  std::memcpy(mWeightVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
    bufferView.byteLength);
}

void GltfModel::getInvBindMatrices() {
  const tinygltf::Skin &skin = mModel->skins.at(0);
  int invBindMatAccessor = skin.inverseBindMatrices;

  const tinygltf::Accessor &accessor = mModel->accessors.at(invBindMatAccessor);
  const tinygltf::BufferView &bufferView = mModel->bufferViews.at(accessor.bufferView);
  const tinygltf::Buffer &buffer = mModel->buffers.at(bufferView.buffer);

  mInverseBindMatrices.resize(skin.joints.size());
  mJointMatrices.resize(skin.joints.size());
  mJointDualQuats.resize(skin.joints.size());

  std::memcpy(mInverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset,
    bufferView.byteLength);
}

void GltfModel::getAnimations() {
  for (const auto& anim : mModel->animations) {
    Logger::log(1, "%s: loading animation '%s' with %i channels\n", __FUNCTION__, anim.name.c_str(), anim.channels.size());
    std::shared_ptr<GltfAnimationClip> clip = std::make_shared<GltfAnimationClip>(anim.name);
    for (const auto& channel : anim.channels) {
      clip->addChannel(mModel, anim, channel);
    }
    mAnimClips.push_back(clip);
  }
}

void GltfModel::playAnimation(int animNum, float speedDivider, float blendFactor,
    replayDirection direction) {
  double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  if (direction == replayDirection::backward) {
    blendAnimationFrame(animNum, mAnimClips.at(animNum)->getClipEndTime() -
      std::fmod(currentTime / 1000.0 * speedDivider,
      mAnimClips.at(animNum)->getClipEndTime()), blendFactor);
  } else {
    blendAnimationFrame(animNum, std::fmod(currentTime / 1000.0 * speedDivider,
      mAnimClips.at(animNum)->getClipEndTime()), blendFactor);
  }
}

void GltfModel::playAnimation(int sourceAnimNumber, int destAnimNumber, float speedDivider,
    float blendFactor, replayDirection direction) {
  double currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

  if (direction == replayDirection::backward) {
    crossBlendAnimationFrame(sourceAnimNumber, destAnimNumber,
      mAnimClips.at(sourceAnimNumber)->getClipEndTime() -
      std::fmod(currentTime / 1000.0 * speedDivider,
      mAnimClips.at(sourceAnimNumber)->getClipEndTime()), blendFactor);
  } else {
    crossBlendAnimationFrame(sourceAnimNumber, destAnimNumber,
      std::fmod(currentTime / 1000.0 * speedDivider,
      mAnimClips.at(sourceAnimNumber)->getClipEndTime()), blendFactor);
  }
}

void GltfModel::blendAnimationFrame(int animNum, float time, float blendFactor) {
  mAnimClips.at(animNum)->blendAnimationFrame(mNodeList, mAdditiveAnimationMask, time,
    blendFactor);
  updateNodeMatrices(mRootNode);
}

void GltfModel::crossBlendAnimationFrame(int sourceAnimNumber, int destAnimNumber, float time,
    float blendFactor) {

  float sourceAnimDuration = mAnimClips.at(sourceAnimNumber)->getClipEndTime();
  float destAnimDuration = mAnimClips.at(destAnimNumber)->getClipEndTime();

  float scaledTime = time * (destAnimDuration / sourceAnimDuration);

  mAnimClips.at(sourceAnimNumber)->setAnimationFrame(mNodeList, mAdditiveAnimationMask, time);
  mAnimClips.at(destAnimNumber)->blendAnimationFrame(mNodeList, mAdditiveAnimationMask,
    scaledTime, blendFactor);

  mAnimClips.at(destAnimNumber)->setAnimationFrame(mNodeList, mInvertedAdditiveAnimationMask,
    scaledTime);
  mAnimClips.at(sourceAnimNumber)->blendAnimationFrame(mNodeList,
    mInvertedAdditiveAnimationMask, time, blendFactor);

  updateNodeMatrices(mRootNode);
}

float GltfModel::getAnimationEndTime(int animNum) {
  return mAnimClips.at(animNum)->getClipEndTime();
}

std::string GltfModel::getClipName(int animNum) {
  return mAnimClips.at(animNum)->getClipName();
}

std::shared_ptr<VkMesh> GltfModel::getSkeleton() {
  mSkeletonMesh->vertices.clear();
  /* start from Armature child */
  getSkeletonPerNode(mRootNode->getChilds().at(0));
  return mSkeletonMesh;
}

void GltfModel::getSkeletonPerNode(std::shared_ptr<GltfNode> treeNode) {
  glm::vec3 parentPos = treeNode->getGlobalPosition();
  VkVertex parentVertex;
  parentVertex.position = parentPos;
  parentVertex.color = glm::vec3(0.0f, 1.0f, 1.0f);

  for (const auto &childNode : treeNode->getChilds()) {
    glm::vec3 childPos = childNode->getGlobalPosition();
    VkVertex childVertex;
    childVertex.position = childPos;
    childVertex.color = glm::vec3(0.0f, 0.0f, 1.0f);
    mSkeletonMesh->vertices.emplace_back(parentVertex);
    mSkeletonMesh->vertices.emplace_back(childVertex);

    getSkeletonPerNode(childNode);
  }
}

void GltfModel::getNodes(std::shared_ptr<GltfNode> treeNode) {
  int nodeNum = treeNode->getNodeNum();
  std::vector<int> childNodes = mModel->nodes.at(nodeNum).children;

  /* remove the child node with skin/mesh metadata */
  auto removeIt = std::remove_if(childNodes.begin(), childNodes.end(),
    [&](int num) { return mModel->nodes.at(num).skin != -1; }
  );
  childNodes.erase(removeIt, childNodes.end());

  treeNode->addChilds(childNodes);

  for (auto &childNode : treeNode->getChilds()) {
    mNodeList.at(childNode->getNodeNum()) = childNode;
    getNodeData(childNode);
    getNodes(childNode);
  }
}

void GltfModel::getNodeData(std::shared_ptr<GltfNode> treeNode) {
  int nodeNum = treeNode->getNodeNum();
  const tinygltf::Node& node = mModel->nodes.at(nodeNum);
  treeNode->setNodeName(node.name);

  if (node.translation.size()) {
    treeNode->setTranslation(glm::make_vec3(node.translation.data()));
  } else {
    treeNode->setTranslation(glm::vec3(0.0f));
  }

  if (node.rotation.size()) {
    treeNode->setRotation(glm::make_quat(node.rotation.data()));
  } else {
    treeNode->setRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
  }

  if (node.scale.size()) {
    treeNode->setScale(glm::make_vec3(node.scale.data()));
  } else {
    treeNode->setScale(glm::vec3(1.0f));
  }

  treeNode->calculateNodeMatrix();

  updateJointMatricesAndQuats(treeNode);
}

void GltfModel::resetNodeData() {
  getNodeData(mRootNode);
  resetNodeData(mRootNode);
}

void GltfModel::resetNodeData(std::shared_ptr<GltfNode> treeNode) {
  for (auto &childNode : treeNode->getChilds()) {
    getNodeData(childNode);
    resetNodeData(childNode);
  }
}

void GltfModel::updateNodeMatrices(std::shared_ptr<GltfNode> treeNode) {
  treeNode->calculateNodeMatrix();
  updateJointMatricesAndQuats(treeNode);

  for (auto& childNode : treeNode->getChilds()) {
    updateNodeMatrices(childNode);
  }
}

void GltfModel::updateAdditiveMask(std::shared_ptr<GltfNode> treeNode, int splitNodeNum) {
  /* break chain here */
  if (treeNode->getNodeNum() == splitNodeNum) {
    return;
  }

  mAdditiveAnimationMask.at(treeNode->getNodeNum()) = false;
  for (auto& childNode : treeNode->getChilds()) {
    updateAdditiveMask(childNode, splitNodeNum);
  }
}

void GltfModel::updateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode) {
  int nodeNum = treeNode->getNodeNum();
  mJointMatrices.at(mNodeToJoint.at(nodeNum)) =
  treeNode->getNodeMatrix() * mInverseBindMatrices.at(mNodeToJoint.at(nodeNum));

  /* extract components from node matrix */
  glm::quat orientation;
  glm::vec3 scale;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::dualquat dq;

  /* create dual quaternion */
  if (glm::decompose(mJointMatrices.at(mNodeToJoint.at(nodeNum)), scale, orientation,
    translation, skew, perspective)) {
    dq[0] = orientation;
    dq[1] = glm::quat(0.0, translation.x, translation.y, translation.z) * orientation * 0.5f;
    mJointDualQuats.at(mNodeToJoint.at(nodeNum)) = glm::mat2x4_cast(dq);
  } else {
    Logger::log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__,
      nodeNum);
  }
}

void GltfModel::createVertexBuffers(VkRenderData &renderData, VkGltfRenderData &gltfRenderData) {
  const tinygltf::Primitive &primitives = mModel->meshes.at(0).primitives.at(0);
  gltfRenderData.rdGltfVertexBufferData.resize(primitives.attributes.size());
  mAttribAccessors.resize(primitives.attributes.size());

  for (const auto& attrib : primitives.attributes) {
    const std::string attribType = attrib.first;
    const int accessorNum = attrib.second;

    const tinygltf::Accessor &accessor = mModel->accessors.at(accessorNum);
    const tinygltf::BufferView &bufferView = mModel->bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer &buffer = mModel->buffers.at(bufferView.buffer);

    if ((attribType.compare("POSITION") != 0) && (attribType.compare("NORMAL") != 0)
        && (attribType.compare("TEXCOORD_0") != 0) && (attribType.compare("JOINTS_0") != 0)
        && (attribType.compare("WEIGHTS_0") != 0)) {
      Logger::log(1, "%s: skipping attribute type %s\n", __FUNCTION__, attribType.c_str());
      continue;
    }

    Logger::log(1, "%s: data for %s uses accessor %i\n", __FUNCTION__, attribType.c_str(),
      accessorNum);
    if (attribType.compare("POSITION") == 0) {
      int numPositionEntries = accessor.count;
      Logger::log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
        numPositionEntries);
    }

    mAttribAccessors.at(attributes.at(attribType)) = accessorNum;

    /* buffers for position, normal, tex coordinates, joints and weights */
    VertexBuffer::init(renderData,
      gltfRenderData.rdGltfVertexBufferData.at(attributes.at(attribType)),
      bufferView.byteLength);
  }
}

void GltfModel::createIndexBuffer(VkRenderData &renderData, VkGltfRenderData &gltfRenderData) {
  /* buffer for vertex indices */
  const tinygltf::Primitive &primitives = mModel->meshes.at(0).primitives.at(0);
  const tinygltf::Accessor &indexAccessor = mModel->accessors.at(primitives.indices);
  const tinygltf::BufferView &indexBufferView = mModel->bufferViews.at(indexAccessor.bufferView);
  const tinygltf::Buffer &indexBuffer = mModel->buffers.at(indexBufferView.buffer);

  IndexBuffer::init(renderData, gltfRenderData.rdGltfIndexBufferData, indexBufferView.byteLength);
}

void GltfModel::uploadVertexBuffers(VkRenderData& renderData, VkGltfRenderData& gltfRenderData) {
  for (int i = 0; i < 5; ++i) {
    const tinygltf::Accessor& accessor = mModel->accessors.at(mAttribAccessors.at(i));
    const tinygltf::BufferView& bufferView = mModel->bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer& buffer = mModel->buffers.at(bufferView.buffer);

    VertexBuffer::uploadData(renderData,
      gltfRenderData.rdGltfVertexBufferData.at(i), buffer, bufferView);
  }
}

void GltfModel::uploadIndexBuffer(VkRenderData& renderData, VkGltfRenderData& gltfRenderData) {
  /* buffer for vertex indices */
  const tinygltf::Primitive& primitives = mModel->meshes.at(0).primitives.at(0);
  const tinygltf::Accessor& indexAccessor = mModel->accessors.at(primitives.indices);
  const tinygltf::BufferView& indexBufferView = mModel->bufferViews.at(indexAccessor.bufferView);
  const tinygltf::Buffer& indexBuffer = mModel->buffers.at(indexBufferView.buffer);

  IndexBuffer::uploadData(renderData, gltfRenderData.rdGltfIndexBufferData,
    indexBuffer, indexBufferView);
}

int GltfModel::getTriangleCount() {
  const tinygltf::Primitive &primitives = mModel->meshes.at(0).primitives.at(0);
  const tinygltf::Accessor &indexAccessor = mModel->accessors.at(primitives.indices);

  unsigned int triangles = 0;
  switch (primitives.mode) {
    case TINYGLTF_MODE_TRIANGLES:
      triangles =  indexAccessor.count / 3;
      break;
    default:
      Logger::log(1, "%s error: unknown draw mode %i\n", __FUNCTION__, primitives.mode);
      break;
  }
  return triangles;
}

int GltfModel::getJointMatrixSize() {
  return mJointMatrices.size();
}

std::vector<glm::mat4> GltfModel::getJointMatrices() {
  return mJointMatrices;
}

int GltfModel::getJointDualQuatsSize() {
  return mJointDualQuats.size();
}

std::vector<glm::mat2x4> GltfModel::getJointDualQuats() {
  return mJointDualQuats;
}

void GltfModel::setSkeletonSplitNode(int nodeNum) {
  std::fill(mAdditiveAnimationMask.begin(), mAdditiveAnimationMask.end(), true);
  updateAdditiveMask(mRootNode, nodeNum);

  mInvertedAdditiveAnimationMask = mAdditiveAnimationMask;
  mInvertedAdditiveAnimationMask.flip();
}

std::string GltfModel::getNodeName(int nodeNum) {
  if (nodeNum >= 0 && nodeNum < (mNodeList.size()) && mNodeList.at(nodeNum)) {
    return mNodeList.at(nodeNum)->getNodeName();
  }
  return "(Invalid)";
}

void GltfModel::setInverseKinematicsNodes(int effectorNodeNum, int ikChainRootNodeNum) {
  if (effectorNodeNum < 0 || effectorNodeNum > (mNodeList.size() - 1)) {
    Logger::log(1, "%s error: effector node %i is out of range\n", __FUNCTION__,
      effectorNodeNum);
    return;
  }

  if (ikChainRootNodeNum < 0 || ikChainRootNodeNum > (mNodeList.size() - 1)) {
    Logger::log(1, "%s error: IK chaine root node %i is out of range\n", __FUNCTION__,
      ikChainRootNodeNum);
    return;
  }

  std::vector<std::shared_ptr<GltfNode>> ikNodes{};
  int currentNodeNum = effectorNodeNum;

  ikNodes.insert(ikNodes.begin(), mNodeList.at(effectorNodeNum));
  while (currentNodeNum != ikChainRootNodeNum) {
    std::shared_ptr<GltfNode> node = mNodeList.at(currentNodeNum);
    if (node) {
      std::shared_ptr<GltfNode> parentNode = node->getParentNode();
      if (parentNode) {
        currentNodeNum = parentNode->getNodeNum();
        ikNodes.push_back(parentNode);
      } else {
        /* force stopping on the root node */
        Logger::log(1, "%s error: reached skeleton root node, stopping\n", __FUNCTION__);
        break;
      }
    }
  }

  mIKSolver.setNodes(ikNodes);
}

void GltfModel::setNumIKIterations(int iterations) {
  mIKSolver.setNumIterations(iterations);
}

void GltfModel::solveIKByCCD(glm::vec3 target)  {
  mIKSolver.solveCCD(target);
  updateNodeMatrices(mIKSolver.getIkChainRootNode());
}

void GltfModel::solveIKByFABRIK(glm::vec3 target)  {
  mIKSolver.solveFABRIK(target);
  updateNodeMatrices(mIKSolver.getIkChainRootNode());
}

void GltfModel::draw(VkRenderData &renderData, VkGltfRenderData& gltfRenderData) {
  /* texture */
  vkCmdBindDescriptorSets(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    renderData.rdGltfPipelineLayout, 0, 1,
    &gltfRenderData.rdGltfModelTexture.texTextureDescriptorSet, 0, nullptr);

  /* vertex buffer */
  VkDeviceSize offset = 0;
  for (int i = 0; i < 5; ++i) {
    vkCmdBindVertexBuffers(renderData.rdCommandBuffer, i, 1,
      &gltfRenderData.rdGltfVertexBufferData.at(i).rdVertexBuffer, &offset);
  }

  /* index buffer */
  vkCmdBindIndexBuffer(renderData.rdCommandBuffer,
    gltfRenderData.rdGltfIndexBufferData.rdIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

  /* pipeline + shader */
  if (renderData.rdGPUDualQuatVertexSkinning == skinningMode::dualQuat) {
    vkCmdBindPipeline(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      renderData.rdGltfGPUDQPipeline);
  } else {
    vkCmdBindPipeline(renderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
     renderData.rdGltfGPUPipeline);
  }
  vkCmdDrawIndexed(renderData.rdCommandBuffer,
    static_cast<uint32_t>(renderData.rdGltfTriangleCount * 3), 1, 0, 0, 0);
}

void GltfModel::cleanup(VkRenderData &renderData, VkGltfRenderData &gltfRenderData) {
  for (int i = 0; i < 5 ; ++i) {
    VertexBuffer::cleanup(renderData, gltfRenderData.rdGltfVertexBufferData.at(i));
  }

  IndexBuffer::cleanup(renderData, gltfRenderData.rdGltfIndexBufferData);

  Texture::cleanup(renderData, gltfRenderData.rdGltfModelTexture);
  mModel.reset();
  mNodeList.clear();
}
