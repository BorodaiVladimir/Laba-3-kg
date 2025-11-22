#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<Vec3f> verts;
    std::vector<std::vector<int>> faces;

    TGAImage texture;
    bool has_texture = false;

public:
    Model(const char* filename);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int i);
    std::vector<int> face(int idx);

    bool load_texture(const char* filename);
    TGAColor get_texture_color(float u, float v);
    bool has_texture_loaded() const { return has_texture; }

    TGAImage get_texture() const { return texture; }
};

#endif //__MODEL_H__