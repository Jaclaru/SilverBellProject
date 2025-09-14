#include "ModelImporter.hh"

#define TINYOBJLOADER_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <tinyobjloader/tiny_obj_loader.h>

using namespace SilverBell;

Model* FModelImporter::ImporterModel(std::string_view FilePath)
{
    tinyobj::attrib_t Attributes;
    std::vector<tinyobj::shape_t> Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::string Warn;
    std::string Error;

    std::string FullPath = std::string(PROJECT_ROOT_PATH) + std::string(FilePath);

    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warn, &Error, FullPath.c_str()))
    {
        spdlog::error("模型加载失败: {}, {}", Warn, Error);
        return nullptr;
    }

    uint32_t SizeCount = 0;
    Model* ImportedModel = new Model;
    for (const auto& Shape : Shapes)
    {
        SizeCount += static_cast<uint32_t>(Shape.mesh.indices.size());
    }
   // ImportedModel.MeshIndices.Indices.resize(SizeCount);
    ImportedModel->MeshData.Positions.resize(SizeCount);
    ImportedModel->MeshData.Color.resize(SizeCount);
    ImportedModel->MeshData.TexCoord.resize(SizeCount);

    uint32_t CurrentIdx = 0;
    for (const auto& Shape : Shapes)
    {
        for (size_t Idx = 0; Idx < Shape.mesh.indices.size(); ++Idx)
        {

            const tinyobj::index_t& Vertex = Shape.mesh.indices[Idx];
            ImportedModel->MeshData.Positions[CurrentIdx] = Math::Vec3(
                Attributes.vertices[3 * Vertex.vertex_index + 0],
                Attributes.vertices[3 * Vertex.vertex_index + 1],
                Attributes.vertices[3 * Vertex.vertex_index + 2]
            );
            if (Vertex.texcoord_index >= 0)
            {
                ImportedModel->MeshData.TexCoord[CurrentIdx] = Math::Vec2(
                    Attributes.texcoords[2 * Vertex.texcoord_index + 0],
                    1.0f - Attributes.texcoords[2 * Vertex.texcoord_index + 1]
                );
            }
            else
            {
                ImportedModel->MeshData.TexCoord[CurrentIdx] = Math::Vec2(0.0f, 0.0f);
            }
            ImportedModel->MeshData.Color[CurrentIdx] = Math::Vec3(1.0f, 1.0f, 1.0f);

            ++CurrentIdx;
        }
    }

    return ImportedModel;
}
