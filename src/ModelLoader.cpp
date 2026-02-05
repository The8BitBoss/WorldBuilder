#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <SFML/Graphics/Image.hpp>

#include <filesystem>
#include <iostream>

std::optional<Model> ModelLoader::LoadModel(const std::string& filepath)
{
    // Resolve absolute path and print diagnostics
    std::filesystem::path requestedPath(filepath);
    std::filesystem::path resolvedPath = std::filesystem::absolute(requestedPath);
    std::cout << "ModelLoader: requested='" << filepath << "' resolved='" << resolvedPath.u8string() << "'\n";
    std::cout << "ModelLoader: current_working_dir='" << std::filesystem::current_path().u8string() << "'\n";
    if (!std::filesystem::exists(resolvedPath))
    {
        std::cerr << "ModelLoader: file does not exist at resolved path: " << resolvedPath.u8string() << '\n';
        // Continue to let Assimp try (it may search relative paths), but this log helps diagnose copies.
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        resolvedPath.u8string(),
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace |
        aiProcess_EmbedTextures);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "Assimp error loading model '" << filepath << "': " << importer.GetErrorString() << '\n';
        return std::nullopt;
    }

    Model model;
    m_textureCache.clear();

    std::filesystem::path p(filepath);
    const std::string directory = p.parent_path().string();

    ProcessNode(scene->mRootNode, scene, model, directory);
    return model;
}

void ModelLoader::ProcessNode(const aiNode* node, const aiScene* scene, Model& outModel, const std::string& directory)
{
    if (!node) return;

    // Process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        outModel.meshes.push_back(ProcessMesh(mesh, scene, outModel, directory));
    }

    // Then process children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene, outModel, directory);
    }
}

Mesh ModelLoader::ProcessMesh(const aiMesh* mesh, const aiScene* scene, Model& outModel, const std::string& directory)
{
    Mesh result;
    result.name = mesh->mName.C_Str();

    // Vertices
    result.vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex{};
        // position
        vertex.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // normals
        if (mesh->HasNormals())
        {
            vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }
        else
        {
            vertex.normal = { 0.f, 0.f, 0.f };
        }

        // texture coordinates (only the first set)
        if (mesh->mTextureCoords && mesh->mTextureCoords[0])
        {
            vertex.texCoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }
        else
        {
            vertex.texCoords = { 0.f, 0.f };
        }

        result.vertices.push_back(vertex);
    }

    // Indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    // Material / texture
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // Try diffuse texture first
        int texIndex = LoadMaterialTexture(material, aiTextureType_DIFFUSE, outModel, directory, scene);
        if (texIndex == -1)
        {
            // fallback: use base color or other types if desired
            texIndex = LoadMaterialTexture(material, aiTextureType_BASE_COLOR, outModel, directory, scene);
        }
        result.textureIndex = texIndex;
    }

    return result;
}

int ModelLoader::LoadMaterialTexture(aiMaterial* material, aiTextureType type, Model& outModel, const std::string& directory, const aiScene* scene)
{
    const unsigned int count = material->GetTextureCount(type);
    for (unsigned int i = 0; i < count; ++i)
    {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string texPath = str.C_Str();

        std::cout << "Texture path from material: " << texPath << std::endl;

        // Build cache key: use embedded id (e.g. "*0") as-is, otherwise absolute/relative path
        std::string cacheKey;
        const aiTexture* embeddedTex = nullptr;
        if (!texPath.empty() && texPath[0] == '*')
        {
            cacheKey = texPath;
            embeddedTex = scene->GetEmbeddedTexture(texPath.c_str());
            if (!embeddedTex) {
				std::cerr << "Warning: texture path '" << texPath << "' indicates embedded texture but it was not found in the scene.\n";
				continue; // skip this texture
            }
        }
        else
        {
                // External file: try model directory + texPath
            std::filesystem::path fullPath = directory.empty() ? std::filesystem::path(texPath) : std::filesystem::path(directory) / texPath;
            cacheKey = fullPath.u8string();
            // normalize slashes (optional)
        }

        // Check cache
        auto it = m_textureCache.find(cacheKey);
        if (it != m_textureCache.end())
            return it->second;

        // Load texture
        sf::Texture texture;
        bool loaded = false;

        if (embeddedTex != nullptr)
        {
            // Embedded texture
            if (embeddedTex->mHeight == 0)
            {
                // Compressed (PNG/JPEG/etc.) stored in memory, size = mWidth, data = pcData
                const std::size_t size = static_cast<std::size_t>(embeddedTex->mWidth);
                const void* data = embeddedTex->pcData;
                if (data && size > 0)
                {
                    loaded = texture.loadFromMemory(data, size);
                    if (!loaded)
                        std::cerr << "Failed to load embedded compressed texture: " << texPath << '\n';
                }
                else
                {
                    std::cerr << "Embedded compressed texture has invalid data: " << texPath << '\n';
                }
            }
            else
            {
                // Uncompressed RGBA image: pcData points to raw pixel bytes (RGBA)
                const unsigned int w = static_cast<unsigned int>(embeddedTex->mWidth);
                const unsigned int h = static_cast<unsigned int>(embeddedTex->mHeight);
                const unsigned char* pixels = reinterpret_cast<const unsigned char*>(embeddedTex->pcData);
                if (pixels)
                {
                    sf::Image img(sf::Vector2u{w, h}, sf::Color::Transparent);
                    // Each pixel is 4 bytes (RGBA)
                    const std::size_t rowBytes = static_cast<std::size_t>(w) * 4;
                    for (unsigned int y = 0; y < h; ++y)
                    {
                        const unsigned char* row = pixels + static_cast<std::size_t>(y) * rowBytes;
                        for (unsigned int x = 0; x < w; ++x)
                        {
                            const std::size_t idx = static_cast<std::size_t>(x) * 4;
                            std::int8_t r = row[idx + 0];
                            std::int8_t g = row[idx + 1];
                            std::int8_t b = row[idx + 2];
                            std::int8_t a = row[idx + 3];
                            img.setPixel({x, y}, sf::Color(r, g, b, a));
                        }
                    }
                    loaded = texture.loadFromImage(img);
                    if (!loaded)
                        std::cerr << "Failed to load embedded raw texture: " << texPath << '\n';
                }
                else
                {
                    std::cerr << "Embedded raw texture has null data: " << texPath << '\n';
                }
            }
        }
        else
        {
            // External image on disk
            std::filesystem::path fullPath = cacheKey.empty() ? std::filesystem::path(texPath) : std::filesystem::path(cacheKey);
            std::string fullPathStr = fullPath.u8string();

            // Try full path first
            if (texture.loadFromFile(fullPathStr))
            {
                loaded = true;
            }
            else
            {
                // Try just the filename (some exporters only write filename)
                std::filesystem::path justFile = std::filesystem::path(texPath).filename();
                if (!justFile.empty() && texture.loadFromFile(justFile.u8string()))
                {
                    loaded = true;
                    fullPathStr = justFile.u8string();
                    cacheKey = fullPathStr; // cache by the resolved path
                }
                else
                {
                    std::cerr << "Failed to load texture from file: " << fullPathStr << " (and tried filename: " << texPath << ")\n";
                }
            }
        }

        if (!loaded)
            continue; // try next texture slot if available

        // Store texture and update cache
        outModel.textures.push_back(std::move(texture));
        int newIndex = static_cast<int>(outModel.textures.size() - 1);
        m_textureCache.emplace(cacheKey, newIndex);
        return newIndex;
    }

    return -1;
}