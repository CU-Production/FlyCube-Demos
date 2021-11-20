#pragma once
#include "Pipeline/VKPipeline.h"
#include <Instance/BaseTypes.h>
#include <vulkan/vulkan.hpp>
#include <RenderPass/VKRenderPass.h>
#include <ShaderReflection/ShaderReflection.h>

class VKDevice;

class VKRayTracingPipeline : public VKPipeline
{
public:
    VKRayTracingPipeline(VKDevice& device, const RayTracingPipelineDesc& desc);
    PipelineType GetPipelineType() const override;
    std::vector<uint8_t> GetRayTracingShaderGroupHandles(uint32_t first_group, uint32_t group_count) const override;

private:
    RayTracingPipelineDesc m_desc;
};
