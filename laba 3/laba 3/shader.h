#ifndef __SHADER_H__
#define __SHADER_H__

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"

struct IShader {
    virtual ~IShader() {}
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

class PhongShader : public IShader {
public:
    Matrix uniform_M;
    Matrix uniform_MIT;
    Vec3f uniform_light_dir;
    Vec3f uniform_eye_pos;

    Vec3f varying_normal[3];
    Vec3f varying_pos[3];

    Model* model;

    PhongShader(Model* m) : model(m) {}

    virtual Vec4f vertex(int iface, int nthvert) {
        std::vector<int> face = model->face(iface);
        Vec3f vertex = model->vert(face[nthvert]);
        Vec4f gl_Vertex = uniform_M * Vec4f(vertex.x, vertex.y, vertex.z, 1.0f);

        Vec3f v0 = model->vert(face[0]);
        Vec3f v1 = model->vert(face[1]);
        Vec3f v2 = model->vert(face[2]);
        Vec3f normal = ((v2 - v0) ^ (v1 - v0)).normalize();

        varying_normal[nthvert] = normal;
        varying_pos[nthvert] = vertex;

        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {
        Vec3f n = (varying_normal[0] * bar.x +
            varying_normal[1] * bar.y +
            varying_normal[2] * bar.z).normalize();

        Vec3f pos = varying_pos[0] * bar.x + varying_pos[1] * bar.y + varying_pos[2] * bar.z;

        Vec3f light_dir = uniform_light_dir.normalize();
        Vec3f view_dir = (uniform_eye_pos - pos).normalize();
        Vec3f reflect_dir = (n * (n * light_dir * 2.0f) - light_dir).normalize();

        float ambient = 0.1f;
        float diffuse = std::max(0.0f, n * light_dir);
        float specular = std::pow(std::max(0.0f, view_dir * reflect_dir), 32.0f);

        float intensity = ambient + diffuse + specular * 0.5f;
        intensity = std::min(1.0f, intensity);

        unsigned char c = static_cast<unsigned char>(255 * intensity);
        color = TGAColor(c, c, c, 255);

        return false;
    }
};

#endif //__SHADER_H__