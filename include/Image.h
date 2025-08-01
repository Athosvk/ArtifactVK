#pragma once
#include <memory>
#include <string>

#include "backend/Texture.h"

typedef unsigned char stbi_uc;

class Image
{
public:
    Image(const std::string &path);

	int GetWidth() const;
	int GetHeight() const;
    unsigned char *GetPixels() const;
    Texture2DCreateInfo GetTextureCreateDesc() const;
  private:
    std::unique_ptr<stbi_uc, void(*)(void*)> m_Data;
	int m_Width;
	int m_Height;
	int m_Channels;
};

