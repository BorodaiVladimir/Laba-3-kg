#include "model.h"
#include <fstream>
#include <sstream>
#include <iostream>

Model::Model(const char* filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;

    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;

        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts.push_back(v);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int itrash, idx;
            iss >> trash;
            while (iss >> idx >> trash >> itrash >> trash >> itrash) {
                idx--;
                f.push_back(idx);
            }
            faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << " f# " << faces.size() << std::endl;
}

Model::~Model() {
}

bool Model::load_texture(const char* filename) {
    has_texture = texture.read_tga_file(filename);
    if (has_texture) {
        texture.flip_vertically();
        std::cerr << "Texture loaded: " << filename << " ("
            << texture.get_width() << "x" << texture.get_height() << ")" << std::endl;
    }
    return has_texture;
}

TGAColor Model::get_texture_color(float u, float v) {
    if (!has_texture) return TGAColor(128, 128, 128, 255);

    int tex_x = static_cast<int>(u * texture.get_width());
    int tex_y = static_cast<int>(v * texture.get_height());

    tex_x = std::max(0, std::min(texture.get_width() - 1, tex_x));
    tex_y = std::max(0, std::min(texture.get_height() - 1, tex_y));

    return texture.get(tex_x, tex_y);
}

Vec3f Model::vert(int i) {
    return verts[i];
}

std::vector<int> Model::face(int idx) {
    return faces[idx];
}

int Model::nverts() {
    return verts.size();
}

int Model::nfaces() {
    return faces.size();
}