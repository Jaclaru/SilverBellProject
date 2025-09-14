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
        std::vector<uint16_t> Indices;
    };

    const BaseMesh TestTriangleMesh =
    {
        .Positions = {
                            Math::Vec3(-0.5f, -0.5f, 0.0f),
                            Math::Vec3(0.5f, -0.5f, 0.0f),
                            Math::Vec3(0.5f, 0.5f, 0.0f),
                            Math::Vec3(-0.5f, 0.5f, 0.0f)
                        },

        .Color =    {
                            Math::Vec3(1.0f, 0.0f, 0.0f),
                            Math::Vec3(0.0f, 1.0f, 0.0f),
                            Math::Vec3(0.0f, 0.0f, 1.0f),
                            Math::Vec3(1.0f, 1.0f, 1.0f)
                        },

        .TexCoord = {
                            Math::Vec2(1.0f, 0.0f),
                            Math::Vec2(0.0f, 0.0f),
                            Math::Vec2(0.0f, 1.0f),
                            Math::Vec2(1.0f, 1.0f)
                        }
    };

    const MeshIndices TestTriangleMeshIndices =
    {
        .Indices = { 0, 1, 2, 2, 3, 0 }
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