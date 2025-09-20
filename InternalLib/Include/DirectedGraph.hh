#pragma once

#include "InternalLibMarco.hh"
#include <spdlog/logger.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SilverBell::Algorithm
{
    /*
     * 简易实现一个有向图
     * 参考自 Nvidia Falcor https://github.com/NVIDIAGameWorks/Falcor
     */
    class INTERNALLIB_API DirectedGraph
    {
    public:
        static constexpr uint32_t InvalidID = static_cast<uint32_t>(-1);

        class Node;
        class Edge;

        uint32_t AddNode()
        {
            NodeMap[CurrentNodeId] = Node();
            return CurrentNodeId++;
        }

        std::unordered_set<uint32_t> RemoveNode(const uint32_t NodeID)
        {
            if (NodeMap.find(NodeID) == NodeMap.end())
            {
                spdlog::warn("节点ID不存在！");
                return {};
            }

            // 找到所有相关的边
            std::unordered_set<uint32_t> RemovedEdges;
            // 找到该节点的入向边的源节点，并获取该源节点需要删除的出向边存入RemovedEdges
            for (auto& EdgeId : NodeMap[NodeID].IncomingEdges)
                FindEdgesToRemove<false>(NodeMap[EdgeMap[EdgeId].SrcNodeID].OutgoingEdges, NodeID, RemovedEdges);
            for (auto& EdgeId : NodeMap[NodeID].OutgoingEdges)
                // 找到该节点的出向边的目标节点，并获取该目标节点需要删除的入向边存入RemovedEdges
                FindEdgesToRemove<true>(NodeMap[EdgeMap[EdgeId].DstNodeID].IncomingEdges, NodeID, RemovedEdges);

            // 删除所有相关的边
            for (auto& EdgeId : RemovedEdges)
                RemoveEdge(EdgeId);
            // 删除节点
            NodeMap.erase(NodeID);

            return RemovedEdges;
        }

        // 添加一条边连接两个节点
        uint32_t AddEdge(uint32_t SrcNode, uint32_t DstNode)
        {
            if (!NodeMap.contains(SrcNode))
            {
                spdlog::warn("无法添加一条边到有向图中，源节点ID不存在！");
                return InvalidID;
            }

            if (!NodeMap.contains(DstNode))
            {
                spdlog::warn("无法添加一条边到有向图中，目标节点ID不存在！");
                return InvalidID;
            }

            NodeMap[SrcNode].OutgoingEdges.push_back(CurrentEdgeId);
            NodeMap[DstNode].IncomingEdges.push_back(CurrentEdgeId);

            EdgeMap[CurrentEdgeId] = Edge(SrcNode, DstNode);
            return CurrentEdgeId++;
        }

        void RemoveEdge(uint32_t EdgeId)
        {
            if (!EdgeMap.contains(EdgeId))
            {
                spdlog::warn("无法添加一条边到有向图中，边的ID不存在！");
                return;
            }

            const auto& Edge = EdgeMap[EdgeId];
            // 移除节点中对边引用
            RemoveEdgeFromNode<true>(EdgeId, NodeMap[Edge.GetDestNode()]);
            RemoveEdgeFromNode<false>(EdgeId, NodeMap[Edge.GetSourceNode()]);

            EdgeMap.erase(EdgeId);
        }

        // 有向节点
        class Node
        {
        public:
            Node() = default;

            // 获取入度
            __FORCEINLINE uint32_t GetOutgoingEdgeCount() const { return static_cast<uint32_t>(OutgoingEdges.size()); }
            // 获取出度
            __FORCEINLINE uint32_t GetIncomingEdgeCount() const { return static_cast<uint32_t>(IncomingEdges.size()); }

            // 获取指定入向边
            __FORCEINLINE uint32_t GetIncomingEdge(const uint32_t Idx) const { return IncomingEdges[Idx]; }
            // 获取指定出向边
            __FORCEINLINE uint32_t GetOutgoingEdge(const uint32_t Idx) const { return OutgoingEdges[Idx]; }

        private:
            friend DirectedGraph;
            // 入向边
            std::vector<uint32_t> IncomingEdges;
            // 出向边
            std::vector<uint32_t> OutgoingEdges;
        };

        // 有向边
        class Edge
        {
        public:
            Edge() = default;
            __FORCEINLINE uint32_t GetSourceNode() const { return SrcNodeID; }
            __FORCEINLINE uint32_t GetDestNode() const { return DstNodeID; }

        private:
            Edge(uint32_t Src, uint32_t Dst) : SrcNodeID(Src), DstNodeID(Dst) {}

            uint32_t SrcNodeID = InvalidID;
            uint32_t DstNodeID = InvalidID;

            friend DirectedGraph;
        };

        __FORCEINLINE bool DoesNodeExist(uint32_t NodeId) const { return NodeMap.contains(NodeId); }

        __FORCEINLINE bool DoesEdgeExist(uint32_t EdgeId) const { return EdgeMap.contains(EdgeId); }

        [[nodiscard]] const Node* GetNode(uint32_t NodeId) const
        {
            if (DoesNodeExist(NodeId) == false)
            {
                spdlog::warn("DirectGraph::GetNode() 节点ID不存在！");
                return nullptr;
            }
            return &NodeMap.at(NodeId);
        }

        [[nodiscard]] const Edge* GetEdge(uint32_t EdgeId) const
        {
            if (DoesEdgeExist(EdgeId) == false)
            {
                spdlog::warn("DirectGraph::GetEdge() 边ID不存在！");
                return nullptr;
            }
            return &EdgeMap.at(EdgeId);
        }

        [[maybe_unused]] uint32_t GetCurrentNodeId() const { return CurrentNodeId; }
        [[maybe_unused]] uint32_t GetCurrentEdgeId() const { return CurrentEdgeId; }

    private:
        template<bool RemoveSrc>
        void FindEdgesToRemove(std::vector<uint32_t>& Edges, uint32_t NodeIDToRemove, std::unordered_set<uint32_t>& RemovedEdges)
        {
            for (uint32_t EdgeID : Edges)
            {
                const auto& CRefEdge = EdgeMap[EdgeID];
                if (auto& OtherNodeID = RemoveSrc ? CRefEdge.SrcNodeID : CRefEdge.DstNodeID; OtherNodeID == NodeIDToRemove)
                {
                    RemovedEdges.insert(EdgeID);
                }
            }
        }

        template<bool RemoveInput>
        void RemoveEdgeFromNode(uint32_t EdgeId, Node& iNode)
        {
            auto& Vec = RemoveInput ? iNode.IncomingEdges : iNode.OutgoingEdges;
            for (auto Iter = Vec.begin(); Iter != Vec.end(); Iter++)
            {
                if (*Iter == EdgeId)
                {
                    Vec.erase(Iter);
                    return;
                }
            }
            throw std::runtime_error("Edge not found in node");
        }

        std::unordered_map<uint32_t, Node> NodeMap;
        std::unordered_map<uint32_t, Edge> EdgeMap;

        uint32_t CurrentNodeId = 0;
        uint32_t CurrentEdgeId = 0;
    };
}
