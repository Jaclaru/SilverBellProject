#include "RenderGraph.hh"

#include <spdlog/spdlog.h>

#include <algorithm>

using namespace SilverBell::Renderer;

namespace 
{
    using StringPair = std::pair<std::string, std::string>;

    // 解析字段名
    StringPair ParseFieldName(std::string_view Fullname)
    {
        StringPair StrPair;
        size_t DotPos = Fullname.find_last_of('.');
        if (DotPos == std::string::npos)
        {
            spdlog::warn("非标准字段字符串！");
            StrPair.first = Fullname;
        }
        else
        {
            StrPair.first = Fullname.substr(0, DotPos);
            StrPair.second = Fullname.substr(DotPos + 1);
        }
        return StrPair;
    }

    // 判断渲染通道的输入输出字段是否存在
    [[maybe_unused]]
    bool CheckRenderPassIoExist(ARenderPass* Pass, const std::string& FieldName, bool bIsInput)
    {
        const auto& ResourceDescSet = bIsInput ? Pass->Inputs : Pass->Outputs;
        return std::ranges::any_of(ResourceDescSet, [&FieldName](const auto& Desc)
        {
            return Desc.Field == FieldName;
        });
    }

}

FRenderGraph::FRenderGraph(const std::string& iName)
{
    InternalGraph = std::make_unique<Algorithm::DirectedGraph>();
}

uint32_t FRenderGraph::GetPassIndex(const std::string& PassName) const
{
    auto It = NameToIndex.find(PassName);
    if (It == NameToIndex.end())
    {
        spdlog::warn("无法通过名称获取渲染通道索引，名称：{}不存在！", PassName);
        return Algorithm::DirectedGraph::InvalidID;
    }
    return It->second;
}

ARenderPass* FRenderGraph::GetPass(const std::string& PassName) const
{
    uint32_t PassIndex = GetPassIndex(PassName);
    if (PassIndex == Algorithm::DirectedGraph::InvalidID)
        return nullptr;

    return NodeDataMap.at(PassIndex).Pass.get();
}

uint32_t FRenderGraph::AddEdge(const std::string& Src, const std::string& Dst)
{
    EdgeData NewEdge;
    StringPair SrcPair = ParseFieldName(Src);
    StringPair DstPair = ParseFieldName(Dst);

    NewEdge.SrcField = SrcPair.second;
    NewEdge.DstField = DstPair.second;

    uint32_t SrcNodeID = GetPassIndex(SrcPair.first);
    uint32_t DstNodeID = GetPassIndex(DstPair.first);
    if (SrcNodeID == Algorithm::DirectedGraph::InvalidID 
        || DstNodeID == Algorithm::DirectedGraph::InvalidID)
    {
        spdlog::warn("无法添加一条边到渲染图中，源节点或目标节点ID不存在！");
        return Algorithm::DirectedGraph::InvalidID;
    }

    // 检查RenderPass对应的字段是否存在
    ARenderPass* SrcPass = NodeDataMap[SrcNodeID].Pass.get();
    ARenderPass* DstPass = NodeDataMap[DstNodeID].Pass.get();
    if (!CheckRenderPassIoExist(SrcPass, NewEdge.SrcField, false) || 
        !CheckRenderPassIoExist(DstPass, NewEdge.DstField, true))
    {
        spdlog::warn("无法添加一条边到渲染图中，源节点或目标节点字段不存在！");
        return Algorithm::DirectedGraph::InvalidID;
    }

    uint32_t EdgeID = InternalGraph->AddEdge(SrcNodeID, DstNodeID);
    // 记录边的数据
    EdgeDataMap[EdgeID] = std::move(NewEdge);

    return EdgeID;
}



