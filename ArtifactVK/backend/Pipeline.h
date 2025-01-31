#pragma once
#include "ShaderModule.h"

class Pipeline
{
  public:
    Pipeline(ShaderModule vertex, ShaderModule fragment);

  private:
    ShaderModule m_Vertex;
    ShaderModule m_Fragment;
};
