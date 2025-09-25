#pragma once

#include "RenderPassReflection.hh"

#include <unordered_set>

namespace SilverBell::Renderer
{
    class FRenderGraph;

    // RenderPass抽象基类, 进行RenderPass的Setup和Execution;
    class ARenderPass
    {
    public:
        virtual ~ARenderPass() = default;
        virtual void Initialize() = 0;
        virtual void Build(FRenderGraph& GraphBuilder) = 0;

        std::unordered_set<RenderGraphResourceDesc> Inputs;
        std::unordered_set<RenderGraphResourceDesc> Outputs;
    };

}
