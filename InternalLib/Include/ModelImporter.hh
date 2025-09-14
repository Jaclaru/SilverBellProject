#pragma once

#include <string_view>
#include <optional>

#include "InternalLibMarco.hh"

#include "Mesh.hh"

namespace SilverBell
{
    struct Model
    {
        Assets::BaseMesh MeshData;
        Assets::MeshIndices MeshIndices;
    };

    class INTERNALLIB_API FModelImporter
    {
    public:
        FModelImporter() = delete;
        ~FModelImporter() = delete;

        static Model* ImporterModel(std::string_view FilePath);

    };

}
