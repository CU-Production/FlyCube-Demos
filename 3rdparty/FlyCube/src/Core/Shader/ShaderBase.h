#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <ShaderReflection/ShaderReflection.h>
#include <map>

class ShaderBase : public Shader
{
public:
    ShaderBase(const ShaderDesc& desc, ShaderBlobType blob_type);
    ShaderType GetType() const override;
    const std::vector<uint8_t>& GetBlob() const override;
    uint64_t GetId(const std::string& entry_point) const override;
    const BindKey& GetBindKey(const std::string& name) const override;
    const std::vector<ResourceBindingDesc>& GetResourceBindings() const override;
    const ResourceBindingDesc& GetResourceBinding(const BindKey& bind_key) const override;
    const std::vector<InputLayoutDesc>& GetInputLayouts() const override;
    uint32_t GetInputLayoutLocation(const std::string& semantic_name) const override;
    const std::vector<BindKey>& GetBindings() const override;
    const std::shared_ptr<ShaderReflection>& GetReflection() const override;

protected:
    ShaderType m_shader_type;
    ShaderBlobType m_blob_type;
    std::vector<uint8_t> m_blob;
    std::map<std::string, uint64_t> m_ids;
    std::vector<ResourceBindingDesc> m_bindings;
    std::vector<BindKey> m_binding_keys;
    std::map<BindKey, size_t> m_mapping;
    std::map<std::string, BindKey> m_bind_keys;
    std::vector<InputLayoutDesc> m_input_layout_descs;
    std::map<std::string, uint32_t> m_locations;
    std::shared_ptr<ShaderReflection> m_reflection;
};
