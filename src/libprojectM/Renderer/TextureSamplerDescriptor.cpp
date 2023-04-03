#include "TextureSamplerDescriptor.hpp"

#include "TextureManager.hpp"

#include <utility>

TextureSamplerDescriptor::TextureSamplerDescriptor(const std::shared_ptr<class Texture>& texture,
                                                   const std::shared_ptr<class Sampler>& sampler,
                                                   std::string samplerName,
                                                   std::string sizeName)
    : m_texture(texture)
    , m_sampler(sampler)
    , m_samplerName(std::move(samplerName))
    , m_sizeName(std::move(sizeName))
{
}

auto TextureSamplerDescriptor::Empty() const -> bool
{
    return m_texture.expired() || m_sampler.expired();
}

void TextureSamplerDescriptor::Bind(GLint unit, const Shader& shader) const
{
    auto texture = m_texture.lock();
    auto sampler = m_sampler.lock();
    if (texture && sampler)
    {
        texture->Bind(unit, sampler);

        shader.SetUniformInt(std::string("sampler_" + m_samplerName).c_str(), unit);
        // Might be setting this more than once if the texture is used with different wrap/filter modes, but this rarely happens.
        shader.SetUniformFloat4(std::string("texsize_" + m_sizeName).c_str(), {texture->Width(),
                                                                               texture->Height(),
                                                                               1.0f / static_cast<float>(texture->Width()),
                                                                               1.0f / static_cast<float>(texture->Height())});
    }
}

void TextureSamplerDescriptor::Unbind(GLint unit)
{
    auto texture = m_texture.lock();
    if (texture)
    {
        texture->Unbind(unit);
    }
    Sampler::Unbind(unit);
}

auto TextureSamplerDescriptor::Texture() const -> std::shared_ptr<class Texture>
{
    return m_texture.lock();
}

void TextureSamplerDescriptor::Texture(std::weak_ptr<class Texture> texture)
{
    m_texture = texture;
}

auto TextureSamplerDescriptor::Sampler() const -> std::shared_ptr<class Sampler>
{
    return m_sampler.lock();
}

auto TextureSamplerDescriptor::SamplerDeclaration() const -> std::string
{
    auto texture = m_texture.lock();
    auto sampler = m_sampler.lock();
    if (!texture || !sampler)
    {
        return {};
    }

    std::string declaration = "uniform ";
    if (texture->Type() == GL_TEXTURE_3D)
    {
        declaration.append("sampler3D sampler_");
    }
    else
    {
        declaration.append("sampler2D sampler_");
    }
    declaration.append(m_samplerName);
    declaration.append(";\n");

    return declaration;
}

auto TextureSamplerDescriptor::TexSizeDeclaration() const -> std::string
{
    auto texture = m_texture.lock();
    auto sampler = m_sampler.lock();
    if (!texture || !sampler)
    {
        return {};
    }

    std::string declaration;
    if (!m_sizeName.empty())
    {
        declaration.append("uniform float4 texsize_");
        declaration.append(m_sizeName);
        declaration.append(";\n");
    }

    return declaration;
}

void TextureSamplerDescriptor::TryUpdate(TextureManager& textureManager)
{
    if (!Empty())
    {
        return;
    }

    auto desc = textureManager.GetTexture(m_samplerName);
    if (desc.Empty())
    {
        // Only try once, then give up.
        m_updateFailed = true;
        return;
    }

    m_texture = desc.m_texture;
    m_sampler = desc.m_sampler;
}