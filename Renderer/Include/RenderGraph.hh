#pragma once

#include "RendererMarco.hh"

#include "DirectedGraph.hh"
#include "RenderPass.hh"

#include <memory>
#include <string>

namespace SilverBell
{
    namespace Renderer
    {
        class RENDERER_API FRenderGraph
        {
        public:
            FRenderGraph(const std::string& iName);
            ~FRenderGraph() = default;

            template<typename PassParameters, typename SetupFunctor, typename ExecutionFunctor>
            void AddPass(const std::string& iPassName, SetupFunctor&& Setup, ExecutionFunctor&& Execution)
            {
                const auto PassIndex = InternalGraph->AddNode();
                NameToIndex[iPassName] = PassIndex;

                auto RenderPass = std::make_unique<ARenderPass>();

                NodeDataMap[PassIndex] = NodeData{ .Name = iPassName, .Pass = std::move(RenderPass)};
            }

            uint32_t GetPassIndex(const std::string& PassName) const;

            [[nodiscard]] ARenderPass* GetPass(const std::string& PassName) const;

            uint32_t AddEdge(const std::string& Src, const std::string& Dst);
             
        private:

            struct EdgeData
            {
                std::string SrcField;
                std::string DstField;
            };

            struct NodeData
            {
                std::string Name;
                std::unique_ptr<ARenderPass> Pass;
            };

            // 用于渲染图的整体分析和拓扑排序
            std::unique_ptr<Algorithm::DirectedGraph> InternalGraph;

            std::unordered_map<std::string, uint32_t> NameToIndex;
            std::unordered_map<uint32_t, EdgeData> EdgeDataMap;
            std::unordered_map<uint32_t, NodeData> NodeDataMap;

        };
    }

    
}

