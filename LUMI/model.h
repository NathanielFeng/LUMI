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
    vector<Texture> textures_loaded;	// �洢��ĿǰΪֹ���ص��������������Ż���ȷ����������ض�Ρ�
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
    // ʹ��ASSIMP���ļ�����ģ�ͣ������������vector<mesh>��
    void loadModel(string const& path);


    // �ݹ鴦��ÿ���ڵ��е�ÿ��mesh
    void processNode(aiNode* node, const aiScene* scene);


    Mesh processMesh(aiMesh* mesh, const aiScene* scene);


    // ��鶨�����͵Ĳ��������������û��load����load
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};