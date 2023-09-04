#pragma once
#include <vector>
#include <string>

#include "shader.h"
using namespace std;


class Skybox
{
public:
    Skybox(const string& path);

    void draw(const Shader& shader);


private:
    unsigned int loadCubemap(const string& path);


private:
    unsigned int VAO, VBO;
    unsigned int textureID;
};