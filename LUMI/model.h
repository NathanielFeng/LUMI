#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>

#include "mesh.h"
#include "shader.h"
using namespace std;


class Model
{
public:
    vector<Texture> textures_loaded;	// 存储到目前为止加载的所有纹理，进行优化以确保纹理不会加载多次。
    vector<Mesh>    meshes;
    string          directory;
    bool            gammaCorrection;


    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }


    virtual void draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].draw(shader);
    }

private:
    // 使用ASSIMP从文件载入模型，并将结果存入vector<mesh>中
    void loadModel(string const& path);


    // 递归处理每个节点中的每个mesh
    void processNode(aiNode* node, const aiScene* scene);


    Mesh processMesh(aiMesh* mesh, const aiScene* scene);


    // 检查定制类型的材质纹理，如果材质没有load将其load
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};