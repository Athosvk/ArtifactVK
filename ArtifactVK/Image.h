#pragma once
#include <memory>
#include <string>

typedef unsigned char stbi_uc;

class Image
{
public:
    Image(const std::string &path);

	int GetWidth() const;
	int GetHeight() const;
    unsigned char *GetPixels() const;
  private:
    std::unique_ptr<stbi_uc, void(*)(void*)> m_Data;
	int m_Width;
	int m_Height;
	int m_Channels;
};

