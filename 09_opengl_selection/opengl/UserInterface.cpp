#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "UserInterface.h"

void UserInterface::init(OGLRenderData &renderData) {
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(renderData.rdWindow, true);

  const char *glslVersion = "#version 460 core";
  ImGui_ImplOpenGL3_Init(glslVersion);

  ImGui::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mMatrixGenerationValues.resize(mNumMatrixGenerationValues);
  mIKValues.resize(mNumIKValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);
}

void UserInterface::createFrame(OGLRenderData &renderData, ModelSettings &settings) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuiWindowFlags imguiWindowFlags = 0;
  //imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
  //imguiWindowFlags |= ImGuiWindowFlags_NoResize;
  //imguiWindowFlags |= ImGuiWindowFlags_NoMove;

  ImGui::SetNextWindowBgAlpha(0.8f);

  ImGui::Begin("Control", nullptr, imguiWindowFlags);

  static float newFps = 0.0f;
  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    newFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }
  /* make an averge value to avoid jumps */
  mFramesPerSecond = (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * newFps;

  /* clamp manual input on all sliders to min/max */
  ImGuiSliderFlags flags = ImGuiSliderFlags_ClampOnInput;

  static double updateTime = 0.0;

  /* avoid literal double compares */
  if (updateTime < 0.000001) {
    updateTime = ImGui::GetTime();
  }

  static int fpsOffset = 0;
  static int frameTimeOffset = 0;
  static int modelUploadOffset = 0;
  static int matrixGenOffset = 0;
  static int ikOffset = 0;
  static int matrixUploadOffset = 0;
  static int uiGenOffset = 0;
  static int uiDrawOffset = 0;

  while (updateTime < ImGui::GetTime()) {
    mFPSValues.at(fpsOffset) = mFramesPerSecond;
    fpsOffset = ++fpsOffset % mNumFPSValues;

    mFrameTimeValues.at(frameTimeOffset) = renderData.rdFrameTime;
    frameTimeOffset = ++frameTimeOffset % mNumFrameTimeValues;

    mModelUploadValues.at(modelUploadOffset) = renderData.rdUploadToVBOTime;
    modelUploadOffset = ++modelUploadOffset % mNumModelUploadValues;

    mMatrixGenerationValues.at(matrixGenOffset) = renderData.rdMatrixGenerateTime;
    matrixGenOffset = ++matrixGenOffset % mNumMatrixGenerationValues;

    mIKValues.at(ikOffset) = renderData.rdIKTime;
    ikOffset = ++ikOffset % mNumIKValues;

    mMatrixUploadValues.at(matrixUploadOffset) = renderData.rdUploadToUBOTime;
    matrixUploadOffset = ++matrixUploadOffset % mNumMatrixUploadValues;

    mUiGenValues.at(uiGenOffset) = renderData.rdUIGenerateTime;
    uiGenOffset = ++uiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(uiDrawOffset) = renderData.rdUIDrawTime;
    uiDrawOffset = ++uiDrawOffset % mNumUiDrawValues;

    updateTime += 1.0 / 30.0;
  }

  ImGui::BeginGroup();
  ImGui::Text("FPS:");
  ImGui::SameLine();
  ImGui::Text("%s", std::to_string(mFramesPerSecond).c_str());
  ImGui::EndGroup();

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    float averageFPS = 0.0f;
    for (const auto value : mFPSValues) {
      averageFPS += value;
    }
    averageFPS /= static_cast<float>(mNumFPSValues);
    std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) + "\n30s avg: " + std::to_string(averageFPS);
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(), fpsOffset, fpsOverlay.c_str(), 0.0f, FLT_MAX,
      ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdTriangleCount + renderData.rdGltfTriangleCount).c_str());

    std::string windowDims = std::to_string(renderData.rdWidth) + "x" + std::to_string(renderData.rdHeight);
    ImGui::Text("Window Dimensions:");
    ImGui::SameLine();
    ImGui::Text("%s", windowDims.c_str());

    std::string imgWindowPos = std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
    ImGui::Text("ImGui Window Position:");
    ImGui::SameLine();
    ImGui::Text("%s", imgWindowPos.c_str());
  }


  if (ImGui::CollapsingHeader("Timers")) {
    ImGui::BeginGroup();
    ImGui::Text("Frame Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdFrameTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFrameTime = 0.0f;
      for (const auto value : mFrameTimeValues) {
        averageFrameTime += value;
      }
      averageFrameTime /= static_cast<float>(mNumMatrixGenerationValues);
      std::string frameTimeOverlay = "now:     " + std::to_string(renderData.rdFrameTime)
        + " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
      ImGui::Text("Frame Time       ");
      ImGui::SameLine();
      ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(), mFrameTimeValues.size(), frameTimeOffset,
        frameTimeOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("Model Upload Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdUploadToVBOTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageModelUpload = 0.0f;
      for (const auto value : mModelUploadValues) {
        averageModelUpload += value;
      }
      averageModelUpload /= static_cast<float>(mNumModelUploadValues);
      std::string modelUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime)
        + " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
      ImGui::Text("VBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(), mModelUploadValues.size(), modelUploadOffset,
        modelUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("Matrix Generation Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdMatrixGenerateTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatGen = 0.0f;
      for (const auto value : mMatrixGenerationValues) {
        averageMatGen += value;
      }
      averageMatGen /= static_cast<float>(mNumMatrixGenerationValues);
      std::string matrixGenOverlay = "now:     " + std::to_string(renderData.rdMatrixGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
      ImGui::Text("Matrix Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixGenTimes", mMatrixGenerationValues.data(), mMatrixGenerationValues.size(), matrixGenOffset,
        matrixGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("(IK Generation Time)  :");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdIKTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageIKTime = 0.0f;
      for (const auto value : mIKValues) {
        averageIKTime += value;
      }
      averageIKTime /= static_cast<float>(mNumIKValues);
      std::string ikOverlay = "now:     " + std::to_string(renderData.rdIKTime)
        + " ms\n30s avg: " + std::to_string(averageIKTime) + " ms";
      ImGui::Text("(IK Generation)");
      ImGui::SameLine();
      ImGui::PlotLines("##IKTimes", mIKValues.data(), mIKValues.size(), ikOffset,
        ikOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("Matrix Upload Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdUploadToUBOTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixUpload = 0.0f;
      for (const auto value : mMatrixUploadValues) {
        averageMatrixUpload += value;
      }
      averageMatrixUpload /= static_cast<float>(mNumMatrixUploadValues);
      std::string matrixUploadOverlay = "now:     " + std::to_string(renderData.rdUploadToVBOTime)
        + " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
      ImGui::Text("UBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(), mMatrixUploadValues.size(), matrixUploadOffset,
        matrixUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("UI Generation Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdUIGenerateTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiGen = 0.0f;
      for (const auto value : mUiGenValues) {
        averageUiGen += value;
      }
      averageUiGen /= static_cast<float>(mNumUiGenValues);
      std::string uiGenOverlay = "now:     " + std::to_string(renderData.rdUIGenerateTime)
        + " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
      ImGui::Text("UI Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##UIGenTimes", mUiGenValues.data(), mUiGenValues.size(), uiGenOffset,
        uiGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::BeginGroup();
    ImGui::Text("UI Draw Time:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdUIDrawTime).c_str());
    ImGui::SameLine();
    ImGui::Text("ms");
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiDraw = 0.0f;
      for (const auto value : mUiDrawValues) {
        averageUiDraw += value;
      }
      averageUiDraw /= static_cast<float>(mNumUiDrawValues);
      std::string uiDrawOverlay = "now:     " + std::to_string(renderData.rdUIDrawTime)
        + " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
      ImGui::Text("UI Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(), mUiDrawValues.size(), uiDrawOffset,
        uiDrawOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {
    ImGui::Text("Camera Position:");
    ImGui::SameLine();
    ImGui::Text("%s", glm::to_string(renderData.rdCameraWorldPosition).c_str());

    ImGui::Text("View Azimuth:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdViewAzimuth).c_str());

    ImGui::Text("View Elevation:");
    ImGui::SameLine();
    ImGui::Text("%s", std::to_string(renderData.rdViewElevation).c_str());

    ImGui::Text("Field of View");
    ImGui::SameLine();
    ImGui::SliderInt("##FOV", &renderData.rdFieldOfView, 40, 150, "%d", flags);
  }

  if (ImGui::CollapsingHeader("glTF Instances")) {
    ImGui::Text("Model Instances  : %d", renderData.rdNumberOfInstances);

    ImGui::Text("Selected Instance:");
    ImGui::SameLine();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##LEFT", ImGuiDir_Left) &&
        renderData.rdCurrentSelectedInstance > 0) {
      renderData.rdCurrentSelectedInstance--;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("##SELINST", &renderData.rdCurrentSelectedInstance, 1, 0,
      renderData.rdNumberOfInstances - 1, "%3d", flags);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::ArrowButton("##RIGHT", ImGuiDir_Right) &&
        renderData.rdCurrentSelectedInstance < (renderData.rdNumberOfInstances - 1)) {
      renderData.rdCurrentSelectedInstance++;
    }
    ImGui::PopButtonRepeat();

    ImGui::Text("World Pos (X/Z)  :");
    ImGui::SameLine();
    ImGui::SliderFloat2("##WORLDPOS", glm::value_ptr(settings.msWorldPosition),
      -75.0f, 75.0f, "%.1f", flags);

    ImGui::Text("World Rotation   :");
    ImGui::SameLine();
    ImGui::SliderFloat("##WORLDROT", &settings.msWorldRotation.y,
      -180.0f, 180.0f, "%.0f", flags);
  }

  if (ImGui::CollapsingHeader("glTF Model")) {
    ImGui::Checkbox("Draw Model", &settings.msDrawModel);
    ImGui::Checkbox("Draw Skeleton", &settings.msDrawSkeleton);

    ImGui::Text("Vertex Skinning:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Linear",
      settings.msVertexSkinningMode == skinningMode::linear)) {
       settings.msVertexSkinningMode = skinningMode::linear;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Dual Quaternion",
      settings.msVertexSkinningMode == skinningMode::dualQuat)) {
       settings.msVertexSkinningMode = skinningMode::dualQuat;
    }
  }

  if (ImGui::CollapsingHeader("glTF Animation")) {
    ImGui::Checkbox("Play Animation", &settings.msPlayAnimation);

    if (!settings.msPlayAnimation) {
      ImGui::BeginDisabled();
    }

    ImGui::Text("Animation Direction:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Forward",
      settings.msAnimationPlayDirection == replayDirection::forward)) {
       settings.msAnimationPlayDirection = replayDirection::forward;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Backward",
      settings.msAnimationPlayDirection == replayDirection::backward)) {
       settings.msAnimationPlayDirection = replayDirection::backward;
    }

    if (!settings.msPlayAnimation) {
      ImGui::EndDisabled();
    }

    ImGui::Text("Clip   ");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##ClipCombo",
      settings.msClipNames.at(settings.msAnimClip).c_str())) {
      for (int i = 0; i < settings.msClipNames.size(); ++i) {
        const bool isSelected = (settings.msAnimClip == i);
        if (ImGui::Selectable(settings.msClipNames.at(i).c_str(), isSelected)) {
          settings.msAnimClip = i;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (settings.msPlayAnimation) {
      ImGui::Text("Speed  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeed", &settings.msAnimSpeed, 0.0f, 2.0f, "%.3f", flags);
    } else {
      ImGui::Text("Timepos");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipPos", &settings.msAnimTimePosition, 0.0f,
        settings.msAnimEndTime, "%.3f", flags);
    }
  }

  if (ImGui::CollapsingHeader("glTF Animation Blending")) {
    ImGui::Text("Blending Type:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Fade In/Out",
      settings.msBlendingMode == blendMode::fadeinout)) {
       settings.msBlendingMode = blendMode::fadeinout;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Crossfading",
      settings.msBlendingMode == blendMode::crossfade)) {
       settings.msBlendingMode = blendMode::crossfade;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Additive",
      settings.msBlendingMode == blendMode::additive)) {
       settings.msBlendingMode = blendMode::additive;
    }

    if (settings.msBlendingMode == blendMode::fadeinout) {
      ImGui::Text("Blend Factor");
      ImGui::SameLine();
      ImGui::SliderFloat("##BlendFactor", &settings.msAnimBlendFactor, 0.0f, 1.0f, "%.3f",
        flags);
    }

    if (settings.msBlendingMode == blendMode::crossfade ||
        settings.msBlendingMode == blendMode::additive) {
      ImGui::Text("Dest Clip   ");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##DestClipCombo",
        settings.msClipNames.at(settings.msCrossBlendDestAnimClip).c_str())) {
        for (int i = 0; i < settings.msClipNames.size(); ++i) {
          const bool isSelected = (settings.msCrossBlendDestAnimClip == i);
          if (ImGui::Selectable(settings.msClipNames.at(i).c_str(), isSelected)) {
            settings.msCrossBlendDestAnimClip = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      ImGui::Text("Cross Blend ");
      ImGui::SameLine();
      ImGui::SliderFloat("##CrossBlendFactor", &settings.msAnimCrossBlendFactor, 0.0f, 1.0f,
        "%.3f", flags);
    }

    if (settings.msBlendingMode == blendMode::additive) {
      ImGui::Text("Split Node  ");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##SplitNodeCombo",
        settings.msSkelNodeNames.at(settings.msSkelSplitNode).c_str())) {
        for (int i = 0; i < settings.msSkelNodeNames.size(); ++i) {
          if (settings.msSkelNodeNames.at(i).compare("(invalid)") != 0) {
            const bool isSelected = (settings.msSkelSplitNode == i);
            if (ImGui::Selectable(settings.msSkelNodeNames.at(i).c_str(), isSelected)) {
              settings.msSkelSplitNode = i;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
        }
        ImGui::EndCombo();
      }
    }
  }

  if (ImGui::CollapsingHeader("glTF Inverse Kinematic")) {
    ImGui::Text("Inverse Kinematics");
    ImGui::SameLine();
    if (ImGui::RadioButton("Off",
      settings.msIkMode == ikMode::off)) {
       settings.msIkMode = ikMode::off;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("CCD",
      settings.msIkMode == ikMode::ccd)) {
       settings.msIkMode = ikMode::ccd;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("FABRIK",
      settings.msIkMode == ikMode::fabrik)) {
       settings.msIkMode = ikMode::fabrik;
    }

    if (settings.msIkMode == ikMode::ccd ||
        settings.msIkMode == ikMode::fabrik) {
      ImGui::Text("IK Iterations  :");
      ImGui::SameLine();
      ImGui::SliderInt("##IKITER", &settings.msIkIterations, 0, 15, "%d", flags);

      ImGui::Text("Target Position:");
      ImGui::SameLine();
      ImGui::SliderFloat3("##IKTargetPOS", glm::value_ptr(settings.msIkTargetPos), -10.0f,
        10.0f, "%.3f", flags);
      ImGui::Text("Effector Node  :");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##EffectorNodeCombo",
        settings.msSkelNodeNames.at(settings.msIkEffectorNode).c_str())) {
        for (int i = 0; i < settings.msSkelNodeNames.size(); ++i) {
          if (settings.msSkelNodeNames.at(i).compare("(invalid)") != 0) {
            const bool isSelected = (settings.msIkEffectorNode == i);
            if (ImGui::Selectable(settings.msSkelNodeNames.at(i).c_str(), isSelected)) {
              settings.msIkEffectorNode = i;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
        }
        ImGui::EndCombo();
      }

      ImGui::Text("IK Root Node   :");
      ImGui::SameLine();
      if (ImGui::BeginCombo("##RootNodeCombo",
        settings.msSkelNodeNames.at(settings.msIkRootNode).c_str())) {
        for (int i = 0; i < settings.msSkelNodeNames.size(); ++i) {
          if (settings.msSkelNodeNames.at(i).compare("(invalid)") != 0) {
            const bool isSelected = (settings.msIkRootNode == i);
            if (ImGui::Selectable(settings.msSkelNodeNames.at(i).c_str(), isSelected)) {
              settings.msIkRootNode = i;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
        }
        ImGui::EndCombo();
      }
    }
  }

  ImGui::End();
}

void UserInterface::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::cleanup() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
