#include "Image.h"

#include <stb/stb_image.h>
#include <stdexcept>

Image::Image(const std::string &path) : 
	m_Data(stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha), stbi_image_free)
{
    if (m_Data == nullptr)
    {
        throw std::runtime_error("Could not load image from " + path);
    }
}

int Image::GetWidth() const
{
    return m_Width;
}

int Image::GetHeight() const
{
    return m_Height;
}

unsigned char *Image::GetPixels() const
{
    return m_Data.get();
}

Texture2DCreateInfo Image::GetTextureCreateDesc() const
{
    // TODO: Make this use the num channels that we actually retrieved and adjust
    // texture format if needed.
    // Standard texture format is expecting RGBA
    constexpr unsigned NumChannels = 4;
    return Texture2DCreateInfo{
        static_cast<uint32_t>(m_Width),
        static_cast<uint32_t>(m_Height), 
        std::span<const unsigned char>(m_Data.get(), m_Width * m_Height * NumChannels)
    };
}
