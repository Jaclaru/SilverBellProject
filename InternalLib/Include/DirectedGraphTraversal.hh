#pragma once

#include "DirectedGraph.hh"

#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace SilverBell::Algorithm
{
    // 有向图遍历基类
    class IDirectedGraphTraversal
    {
    public:
        enum class Flags
        {
            None = 0x0,
            Reverse = 0x1,
            IgnoreVisited = 0x2,
        };

        IDirectedGraphTraversal(const DirectedGraph& iGraph, Flags iFlag) : Graph(iGraph), Flag(iFlag) {}

    protected:
        virtual ~IDirectedGraphTraversal() {}

        bool Reset(uint32_t RootNodeID)
        {
            if (Graph.DoesNodeExist(RootNodeID) == false)
                return false;

            if (static_cast<uint32_t>(Flag) & static_cast<uint32_t>(Flags::IgnoreVisited))
            {
                Visited.assign(Graph.GetCurrentNodeId(), false);
            }

            return true;
        }

        const DirectedGraph& Graph;
        Flags Flag;
        std::vector<bool> Visited;
    };

    // 定义枚举类的位运算符 来自Falcor
    FALCOR_ENUM_CLASS_OPERATORS(IDirectedGraphTraversal::Flags);

    template<typename Args>
        requires requires { typename Args::Container; }&& requires { Args::GetTop(std::declval<typename Args::Container>()); }
    class TDirectedGraphTraversal : public IDirectedGraphTraversal
    {
    public:
        TDirectedGraphTraversal(const DirectedGraph& iGraph, uint32_t RootNodeID, Flags iFlag = Flags::None)
            : IDirectedGraphTraversal(iGraph, iFlag)
        {
            Reset(RootNodeID);
        }
        ~TDirectedGraphTraversal() = default;

        uint32_t Traverse()
        {
            if (NodeList.empty())
            {
                return DirectedGraph::InvalidID;
            }

            uint32_t CurNodeID = Args::GetTop(NodeList);
            if (is_set(Flag, Flags::IgnoreVisited))
            {
                while (Visited[CurNodeID])
                {
                    NodeList.pop();
                    if (NodeList.empty())
                    {
                        return DirectedGraph::InvalidID;
                    }
                    CurNodeID = Args::GetTop(NodeList);
                }

                Visited[CurNodeID] = true;
            }
            NodeList.pop();

            // 获取所有子节点
            const DirectedGraph::Node* pNode = Graph.GetNode(CurNodeID);
            bool Reverse = is_set(Flag, Flags::Reverse);
            uint32_t EdgeCount = Reverse ? pNode->GetIncomingEdgeCount() : pNode->GetOutgoingEdgeCount();

            for (uint32_t Idx = 0; Idx < EdgeCount; Idx++)
            {
                uint32_t E = Reverse ? pNode->GetIncomingEdge(Idx) : pNode->GetOutgoingEdge(Idx);
                const DirectedGraph::Edge* pEdge = Graph.GetEdge(E);
                uint32_t ChildID = Reverse ? pEdge->GetSourceNode() : pEdge->GetDestNode();
                NodeList.push(ChildID);
            }

            return CurNodeID;
        }

        bool Reset(uint32_t RootNodeID)
        {
            bool bSuccess = IDirectedGraphTraversal::Reset(RootNodeID);
            NodeList = decltype(NodeList)();
            if (bSuccess)
                NodeList.push(RootNodeID);
            return bSuccess;
        }

    private:
        typename Args::Container NodeList;
    };

    // 深度优先遍历
    struct DfsArgs
    {
        using Container = std::stack<uint32_t>;
        static const std::string& GetName() { return "DFS"; }
        static const uint32_t& GetTop(const Container& C) { return C.top(); }
    };
    using DirectedGraphDfsTraversal = TDirectedGraphTraversal<DfsArgs>;

    // 广度优先遍历
    struct BfsArgs
    {
        using Container = std::queue<uint32_t>;
        static const std::string GetName() { return "BFS"; }
        static const uint32_t& GetTop(const Container& C) { return C.front(); }
    };
    using DirectedGraphBfsTraversal = TDirectedGraphTraversal<BfsArgs>;

    // 有向图环检测
    class DirectedGraphLoopDetector
    {
    public:
        static bool HasLoop(const DirectedGraph& Graph, uint32_t RootNodeID)
        {
            DirectedGraphDfsTraversal Dfs(Graph, RootNodeID);
            // 跳过第一个节点，因为它是根节点
            uint32_t CurID = Dfs.Traverse();
            while (CurID != DirectedGraph::InvalidID)
            {
                CurID = Dfs.Traverse();
                if (CurID == RootNodeID)
                    return true;
            }

            return false;
        }
    };

    // 有向图拓扑排序
    class DirectedGraphTopologicalSort
    {
    public:
        static std::vector<uint32_t> sort(const DirectedGraph& iGraph)
        {
            DirectedGraphTopologicalSort TSort(iGraph);
            for (uint32_t Idx = 0; Idx < TSort.Graph.GetCurrentNodeId(); Idx++)
            {
                if (TSort.Visited[Idx] == false && TSort.Graph.GetNode(Idx))
                {
                    TSort.SortInternal(Idx);
                }
            }

            std::vector<uint32_t> Result;
            Result.reserve(TSort.Stack.size());
            while (TSort.Stack.empty() == false)
            {
                Result.push_back(TSort.Stack.top());
                TSort.Stack.pop();
            }
            return Result;
        }

    private:
        DirectedGraphTopologicalSort(const DirectedGraph& RefGraph) : Graph(RefGraph), Visited(RefGraph.GetCurrentNodeId(), false) {}

        void SortInternal(uint32_t NodeID)
        {
            Visited[NodeID] = true;
            const DirectedGraph::Node* pNode = Graph.GetNode(NodeID);
            assert(pNode != nullptr);
            for (uint32_t EdgeID = 0; EdgeID < pNode->GetOutgoingEdgeCount(); EdgeID++)
            {
                uint32_t nextNode = Graph.GetEdge(pNode->GetOutgoingEdge(EdgeID))->GetDestNode();
                if (!Visited[nextNode])
                {
                    SortInternal(nextNode);
                }
            }

            Stack.push(NodeID);
        }

        const DirectedGraph& Graph;
        std::stack<uint32_t> Stack;
        std::vector<bool> Visited;
    };

    // 有向图路径检测
    namespace DirectedGraphPathDetector
    {
        __FORCEINLINE bool HasPath(const DirectedGraph& Graph, uint32_t From, uint32_t To)
        {
            DirectedGraphDfsTraversal DFS(Graph, From, DirectedGraphDfsTraversal::Flags::IgnoreVisited);
            uint32_t CurNodeID = DFS.Traverse();
            CurNodeID = DFS.Traverse();
            while (CurNodeID != DirectedGraph::InvalidID)
            {
                if (CurNodeID == To)
                    return true;
                CurNodeID = DFS.Traverse();
            }
            return false;
        }

        __FORCEINLINE bool HasCycle(const DirectedGraph& Graph, uint32_t RootID)
        {
            return HasPath(Graph, RootID, RootID);
        }
    }
}
