#include "RenderPassReflection.hh"

#include <unordered_set>

using namespace SilverBell::Renderer;

void test()
{
    RenderGraphResourceDesc desc;
    desc.Field = "Test";
    desc.Type = EResourceType::Buffer;
    desc.Access = EAccessType::ReadWrite;
    RenderGraphResourceDesc desc2;
    desc2.Field = "Test";
    desc2.Type = EResourceType::Buffer;
    desc2.Access = EAccessType::ReadWrite;
    if (desc == desc2)
    {
        printf("Equal\n");
    }

    std::unordered_set<RenderGraphResourceDesc> descSet;
    descSet.insert(desc);
    
}