#pragma once

#include "Model.h"

class SphereModel : public Model{
  public:
    SphereModel(const float radius = 1.0f, const float verticalDiv = 10, const float horizontalDiv = 20, const glm::vec3 color = glm::vec3(1.0f)) :
      mRadius(radius), mVertDiv(verticalDiv), mHorDiv(horizontalDiv), mColor(color) {};

  private:
    virtual void init() override;

    float mRadius = 1.0f;
    unsigned int mVertDiv = 10;
    unsigned int mHorDiv = 20;
    glm::vec3 mColor = glm::vec3(1.0f);
};
