
// Simple Skeletal Mesh Loader & Renderer
// Author: Yi Kangrui <yikangrui@pku.edu.cn>

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "gl_env.h"

#include "texture_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#define SCENE_RESOURCE_SHADER_DIFFUSE_CHANNEL 0

#define SCENE_RESOURCE_BONE_PER_VERTEX 4

namespace SkeletalMesh {
    typedef std::map<std::string, glm::fmat4> SkeletonModifier;

    struct ParametricVertex {
        float position[3];
        float texcoord[2];
        float normal[3];
        unsigned int boneId[SCENE_RESOURCE_BONE_PER_VERTEX];
        float boneWeight[SCENE_RESOURCE_BONE_PER_VERTEX];

        ParametricVertex() { memset(this, 0, sizeof(ParametricVertex)); }

        ParametricVertex(aiVector3D _p, aiVector2D _tc, aiVector3D _n) {
            memcpy(position, &_p, sizeof(position));
            memcpy(texcoord, &_tc, sizeof(texcoord));
            memcpy(normal, &_n, sizeof(normal));
            memset(boneId, 0, sizeof(boneId));
            memset(boneWeight, 0, sizeof(boneWeight));
        }

        bool addBone(unsigned int _id, float _weight) {
            if (_weight < 1e-6) return false;
            int minWeightIndex = 0;
            for (int i = 1; i < SCENE_RESOURCE_BONE_PER_VERTEX; i++) {
                if (boneWeight[i] < boneWeight[minWeightIndex])
                    minWeightIndex = i;
            }
            if (boneWeight[minWeightIndex] < _weight) {
                boneId[minWeightIndex] = _id;
                boneWeight[minWeightIndex] = _weight;
                return true;
            }
            return false;
        }
    };

    struct MeshEntry {
        unsigned int facetCornerNum;
        unsigned int indexOffset;
        unsigned int vertexOffset;
        unsigned int materialIndex;
    };

    struct Material {
        const TextureImage::Texture *diffuse;

        Material()
                : diffuse(&TextureImage::Texture::error) {}

        bool setDiffuse(std::string _name, std::string _filename = std::string()) {
            return (diffuse = &TextureImage::Texture::loadTexture(_name, _filename))
                   != &TextureImage::Texture::error;
        }
    };

    struct Bone {
        aiMatrix4x4 localTransf;

        //Bone() : localTransf() {}
        Bone(const aiMatrix4x4 &_m) : localTransf(_m) {}
    };

    class Scene {

    public:
        typedef std::map<std::string, Scene *> Name2Scene;
        typedef std::vector<glm::fmat4> SkeletonTransf;
        typedef std::map<std::string, unsigned int> Name2Bone;
        static Name2Scene allScene;
        static Scene error;

    private:
        bool available;
        std::string name;
        std::string filename;
        Assimp::Importer importer;
        const aiScene *scene;
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        std::vector<MeshEntry> meshEntry;
        std::vector<Material> material;
        std::vector<Bone> skeleton;
        Name2Bone nameBoneMap;

        // Forbid calling any constructor outside
        Scene(const Scene &_copy)
                : Scene() {}

        Scene() {
            available = false;
            vao = 0;
            vbo = 0;
            ebo = 0;
        }

        virtual ~Scene() { clear(); }

    public:
        void clear() {
            available = false;
            name = std::string();
            filename = std::string();
            // importer..
            scene = NULL;
            glDeleteVertexArrays(1, &vao);
            vao = 0;
            glDeleteBuffers(1, &vbo);
            vbo = 0;
            glDeleteBuffers(1, &ebo);
            ebo = 0;
            meshEntry.clear();
            material.clear();
            skeleton.clear();
            nameBoneMap.clear();
        }

        static std::string testAllSuffix(std::string no_suffix_name) {
            const int support_suffix_num = 3;
            const std::string support_suffix[support_suffix_num] = {
                    ".obj",
                    ".dae",
                    ".fbx"
            };

            for (int i = 0; i < support_suffix_num; i++) {
                std::string try_filename = no_suffix_name + support_suffix[i];
                FILE *ftest = fopen(try_filename.c_str(), "r");
                if (ftest) {
                    fclose(ftest);
                    return try_filename;
                }
            }
            return std::string();
        }

        static Scene &loadScene(std::string _name, std::string _filename = std::string()) {
            if (_filename.empty() || _filename == "") {
                _filename = testAllSuffix(_name);
                if (_filename.empty()) return error;
            }
            FILE *fi = fopen(_filename.c_str(), "r");
            if (fi == NULL) return error;
            fclose(fi);

            std::pair<Name2Scene::iterator, bool> insertion =
                    allScene.insert(Name2Scene::value_type(_name, new Scene()));
            Scene &target = *(insertion.first->second);
            if (!insertion.second) {
                if (target.filename == _filename && target.available) {
                    return target;
                } else {
                    target.clear();
                }
            }

            target.name = _name;
            target.filename = _filename;

            target.scene = target.importer.ReadFile(_filename,
                                                    aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                    aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
            if (!target.scene) return error;

            std::vector<ParametricVertex> vertexAssembly;
            std::vector<unsigned int> indexAssembly;

            int nTotalMeshes = target.scene->mNumMeshes;
            target.meshEntry.resize(nTotalMeshes);

            int nTotalVertices = 0;
            int nTotalIndices = 0;
            for (int i = 0; i < nTotalMeshes; i++) {
                const aiMesh *curMesh = target.scene->mMeshes[i];
                int nMeshVertices = curMesh->mNumVertices;
                int nMeshBones = curMesh->mNumBones;
                int nMeshFaces = curMesh->mNumFaces;

                target.meshEntry[i].facetCornerNum = nMeshFaces * 3;
                target.meshEntry[i].indexOffset = nTotalIndices;
                target.meshEntry[i].vertexOffset = nTotalVertices;
                target.meshEntry[i].materialIndex = curMesh->mMaterialIndex;

                nTotalVertices += nMeshVertices;
                nTotalIndices += nMeshFaces * 3;

                for (int j = 0; j < nMeshVertices; j++) {
                    aiVector2D curTexcoord(.0f, .0f);
                    if (curMesh->HasTextureCoords(0))
                        curTexcoord = aiVector2D(curMesh->mTextureCoords[0][j].x, curMesh->mTextureCoords[0][j].y);
                    vertexAssembly.emplace_back(curMesh->mVertices[j], curTexcoord, curMesh->mNormals[j]);
                }
                for (int j = 0; j < nMeshBones; j++) {
                    std::string boneName = curMesh->mBones[j]->mName.data;
                    std::pair<std::map<std::string, unsigned int>::iterator, bool> insertResult;
                    insertResult = target.nameBoneMap.insert(std::make_pair(boneName, target.skeleton.size()));
                    if (insertResult.second) {
                        target.skeleton.emplace_back(curMesh->mBones[j]->mOffsetMatrix);
                        int nBoneVertexWeight = curMesh->mBones[j]->mNumWeights;
                        for (int k = 0; k < nBoneVertexWeight; k++) {
                            int vertexId = target.meshEntry[i].vertexOffset + curMesh->mBones[j]->mWeights[k].mVertexId;
                            float weight = curMesh->mBones[j]->mWeights[k].mWeight;
                            vertexAssembly[vertexId].addBone(insertResult.first->second, weight);
                        }
                    }
                }
                for (int j = 0; j < nMeshFaces; j++) {
                    for (int k = 0; k < 3; k++)
                        indexAssembly.push_back(curMesh->mFaces[j].mIndices[k]);
                }
            }

            std::string filepath_prefix;
            {
                size_t slashpos = _filename.rfind('/');
                size_t conslashpos = _filename.rfind('\\');
                if (conslashpos != std::string::npos) {
                    if (slashpos == std::string::npos || slashpos < conslashpos)
                        slashpos = conslashpos;
                }
                if (slashpos != std::string::npos) {
                    filepath_prefix = _filename.substr(0, slashpos + 1);
                }
            }
            int nTotalMaterials = target.scene->mNumMaterials;
            target.material.resize(nTotalMaterials);
            for (int i = 0; i < nTotalMaterials; i++) {
                const aiMaterial *curMaterial = target.scene->mMaterials[i];

                if (curMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                    aiString ai_filepath;
                    if (curMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &ai_filepath, NULL, NULL, NULL, NULL, NULL) ==
                        AI_SUCCESS) {
                        std::string filepath(filepath_prefix + ai_filepath.data);
                        std::string dirpath, filename;
                        size_t slashpos = filepath.rfind('/');
                        size_t conslashpos = filepath.rfind('\\');
                        if (conslashpos != std::string::npos) {
                            if (slashpos == std::string::npos || slashpos < conslashpos)
                                slashpos = conslashpos;
                        }
                        if (slashpos != std::string::npos) {
                            dirpath = filepath.substr(0, slashpos + 1);
                            filename = filepath.substr(slashpos + 1, std::string::npos);
                        } else {
                            dirpath = std::string();
                            filename = filepath;
                        }
                        if (!target.material[i].setDiffuse(filename, dirpath + filename))
                            std::cout << "Error loading diffuse " << filepath << std::endl;
                    }
                }
            }

            glGenVertexArrays(1, &target.vao);
            glBindVertexArray(target.vao);

            glGenBuffers(1, &target.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, target.vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(ParametricVertex) * vertexAssembly.size(), vertexAssembly.data(),
                         GL_STATIC_DRAW);

            glGenBuffers(1, &target.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, target.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexAssembly.size(), indexAssembly.data(),
                         GL_STATIC_DRAW);

            glBindVertexArray(0);

            target.available = true;
            return target;
        }

        static bool unloadScene(std::string _name) {
            return allScene.erase(_name) != 0;
        }

        static Scene &getScene(const std::string &_name) {
            Name2Scene::iterator find_result = allScene.find(_name);
            if (find_result == allScene.end()) return error;
            return *(find_result->second);
        }

        void recursivelyGetTransf(SkeletonTransf &skTransf, SkeletonModifier &modifier, aiNode *node,
                                  aiMatrix4x4 parentTransf, const aiMatrix4x4 &invTransf) const {
            aiMatrix4x4 globalTransf = parentTransf * node->mTransformation;
            Name2Bone::const_iterator boneFound = nameBoneMap.find(std::string(node->mName.data));
            if (boneFound != nameBoneMap.end()) {
                SkeletonModifier::const_iterator boneModFound = modifier.find(std::string(node->mName.data));
                aiMatrix4x4 boneMod;
                if (boneModFound != modifier.end()) {
                    auto trans = glm::transpose(boneModFound->second);
                    memcpy(&boneMod, &trans, 16 * sizeof(float));
                    globalTransf *= boneMod;
                }
                aiMatrix4x4 finalMtrx = invTransf * globalTransf * skeleton[boneFound->second].localTransf;
                memcpy(&skTransf[boneFound->second], &finalMtrx.Transpose(), sizeof(skTransf[boneFound->second]));
            }
            for (int i = 0; i < node->mNumChildren; i++) {
                recursivelyGetTransf(skTransf, modifier, node->mChildren[i], globalTransf, invTransf);
            }
        }

        bool getSkeletonTransform(SkeletonTransf &transf, SkeletonModifier &modifier) const {
            if (!available) return false;

            transf.resize(skeleton.size());

            aiMatrix4x4 identityMtrx, invTransf = scene->mRootNode->mTransformation;
            invTransf.Inverse();
            recursivelyGetTransf(transf, modifier, scene->mRootNode, identityMtrx, invTransf);
            return !transf.empty();
        }

        bool setShaderInput(GLuint program,
                            std::string posiName, std::string texcName, std::string normName,
                            std::string bnidName, std::string bnwtName) {
            if (!available) return false;

            ParametricVertex example;

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            {
                GLint posiLoc = glGetAttribLocation(program, posiName.c_str());
                if (posiLoc >= 0) {
                    glEnableVertexAttribArray(posiLoc);
                    glVertexAttribPointer(posiLoc, 3, GL_FLOAT, GL_FALSE, sizeof(ParametricVertex),
                                          (const void *) ((char *) example.position - (char *) &example));
                }
            }
            {
                GLint texcLoc = glGetAttribLocation(program, texcName.c_str());
                if (texcLoc >= 0) {
                    glEnableVertexAttribArray(texcLoc);
                    glVertexAttribPointer(texcLoc, 2, GL_FLOAT, GL_FALSE, sizeof(ParametricVertex),
                                          (const void *) ((char *) example.texcoord - (char *) &example));
                }
            }
            {
                GLint normLoc = glGetAttribLocation(program, normName.c_str());
                if (normLoc >= 0) {
                    glEnableVertexAttribArray(normLoc);
                    glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE, sizeof(ParametricVertex),
                                          (const void *) ((char *) example.normal - (char *) &example));
                }
            }
            {
                GLint bnidLoc = glGetAttribLocation(program, bnidName.c_str());
                if (bnidLoc >= 0) {
                    glEnableVertexAttribArray(bnidLoc);
                    glVertexAttribIPointer(bnidLoc, SCENE_RESOURCE_BONE_PER_VERTEX, GL_INT, sizeof(ParametricVertex),
                                           (const void *) ((char *) example.boneId - (char *) &example));
                }
            }
            {
                GLint bnwtLoc = glGetAttribLocation(program, bnwtName.c_str());
                if (bnwtLoc >= 0) {
                    glEnableVertexAttribArray(bnwtLoc);
                    glVertexAttribPointer(bnwtLoc, SCENE_RESOURCE_BONE_PER_VERTEX, GL_FLOAT, GL_FALSE,
                                          sizeof(ParametricVertex),
                                          (const void *) ((char *) example.boneWeight - (char *) &example));
                }
            }

            glBindVertexArray(0);

            return true;
        }

        void render() const {
            if (!available) return;
            glBindVertexArray(vao);
            for (int i = 0; i < meshEntry.size(); i++) {
                if (!material[meshEntry[i].materialIndex].diffuse->bind(
                        SCENE_RESOURCE_SHADER_DIFFUSE_CHANNEL))
                    glBindTexture(GL_TEXTURE_2D, 0);

                glDrawElementsBaseVertex(GL_TRIANGLES,
                                         meshEntry[i].facetCornerNum,
                                         GL_UNSIGNED_INT,
                                         (void *) (sizeof(unsigned int) * meshEntry[i].indexOffset),
                                         meshEntry[i].vertexOffset);
            }
            glBindVertexArray(0);
        }
    };

    Scene::Name2Scene Scene::allScene;
    Scene Scene::error;
}