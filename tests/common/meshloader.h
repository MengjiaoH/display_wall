#pragma once
#ifndef _MESH_LOADER_H_
#define _MESH_LOADER_H_

#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/box.h"
#include "ospcommon/range.h"
#include "ospcommon/LinearSpace.h"
#include "ospcommon/AffineSpace.h"

// trying this obj loader https://github.com/syoyo/tinyobjloader
#include "tiny_obj_loader.h"

using namespace ospcommon;

/** \brief structure for a triangular mesh */
class Mesh {
 private:
  //! one geometry contains a continious mesh plus one material index
  struct Geometry {
    std::vector<float> vertex;
    std::vector<float> normal;
    std::vector<float> texcoord;
    std::vector<unsigned int> index;
    int num_faces =  0;
    bool has_normal   = false;
    bool has_texcoord = false;
  };
  struct TinyObjLoader
  {
    std::string                      err;
    tinyobj::attrib_t                attributes; // attributes
    std::vector<tinyobj::shape_t>    shapes; // shapes
    std::vector<tinyobj::material_t> materials; // materials
    void Clear();
  };
  TinyObjLoader tiny;
  /* geometric data */
  vec3f center; // mesh center coordinate in world
  box3f bbox;   // mesh bounding box in world
  affine3f transform;
  /* meta data */
  std::string dpath; // directory path to the mesh folder
  std::string fpath; // directory path to the mesh folder
  std::string fname; // filename of the mesh
  std::vector<Geometry> geometries;
 private:
  void ComputePath(const std::string& str);
 public:
  /** \brief Accessors */
  std::string GetFullPath()
  {
    return fpath;
  }
  vec3f GetBBoxMax()
  {
    return bbox.upper - center;
  }
  vec3f GetBBoxMin()
  {
    return bbox.lower - center;
  }
  vec3f GetCenter() 
  {
    return center;
  }
  float GetDiagonalLength() 
  {
    return length(GetBBoxMax() - GetBBoxMin());
  }
  affine3f GetTransform()
  {
    return transform;
  }
  void SetTransform(const affine3f&);
  /** 
   * \brief Overriding LoadFromFileObj function for TriMesh,
   *  force to triangulate
   */
  bool LoadFromFileObj(const char* filename, bool loadMtl = false);
  /**
   * \brief OSPRay helper     
   */
  void AddToModel(OSPModel model, OSPRenderer renderer, OSPMaterial mtl = nullptr);
};  

#endif//_MESH_LOADER_H_
