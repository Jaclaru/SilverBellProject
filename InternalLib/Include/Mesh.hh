#pragma once

#include "Math.hh"

#include <ylt/reflection/member_value.hpp>

namespace SilverBell::Assets
{
    struct BaseMesh
    {
        std::vector<Math::Vec3> Positions;
        std::vector<Math::Vec3> Color;
        std::vector<Math::Vec2> TexCoord;
    };

    struct Mesh
    {
        std::vector<Math::Vec3> Positions;
        std::vector<Math::Vec3> Normals;
        std::vector<Math::Vec2> Texcoords;

        std::vector<uint32_t> Indices;  // 每3个为一组
    };

    struct MeshIndices
    {
        std::vector<uint32_t> Indices;
    };

    struct MVPMatrix
    {
        Math::Mat4  Model;
        Math::Mat4  View;
        Math::Mat4  Projection;
    };
    YLT_REFL(MVPMatrix, Model, View, Projection);

    //constexpr inline auto MemberCount = ylt::reflection::members_count_v<MVPMatrix>;
    inline MVPMatrix TestTriangleMeshUniformBufferObject =
    {
        .Model = Math::Mat4::Identity(),
        .View = Math::Mat4::Identity(),
        .Projection = Math::Mat4::Identity()
    };

}