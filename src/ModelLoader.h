#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector3.hpp>
#include <SFML/System/Vector2.hpp>

#include <assimp/scene.h>

struct Vertex
{
    sf::Vector3f position;
    sf::Vector3f normal;
    sf::Vector2f texCoords;
};
struct Texture
{
    sf::Texture texture;
    std::string type; // e.g. "diffuse", "specular", etc.
    std::string path;
};
struct Mesh
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // Index into Model::textures. -1 if no texture.
    int textureIndex = -1;
};

struct Model
{
    std::vector<Mesh> meshes;
    // Textures owned by the model. Meshes reference by index.
    std::vector<sf::Texture> textures;
};

class ModelLoader
{
public:
    // Load a model from disk. Returns std::nullopt on failure.
    // textures are loaded relative to the model file directory.
    std::optional<Model> LoadModel(const std::string& filepath);

private:
    void ProcessNode(const aiNode* node, const aiScene* scene, Model& outModel, const std::string& directory);
    Mesh ProcessMesh(const aiMesh* mesh, const aiScene* scene, Model& outModel, const std::string& directory);
    // Accept scene to allow loading embedded textures ("*0" style)
    int LoadMaterialTexture(aiMaterial* material, aiTextureType type, Model& outModel, const std::string& directory, const aiScene* scene);

    // Simple cache, maps texture file path -> texture index in Model::textures
    std::unordered_map<std::string, int> m_textureCache;
    std::string directory;
};