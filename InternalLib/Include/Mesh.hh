#pragma once

#include "Math.hh"

#include <ylt/reflection/member_value.hpp>

namespace SilverBell::Assets
{
    struct BaseMesh
    {
        std::vector<Math::Vec3> Positions;
        std::vector<Math::Vec3> Color;
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
    };

    const MeshIndices TestTriangleMeshIndices =
    {
        .Indices = { 0, 1, 2, 2, 3, 0 }
    };

}