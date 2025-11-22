#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "shader.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
Model* model = NULL;
const int width = 800;
const int height = 800;

float* zBuffer = nullptr;

Matrix ModelView;
Matrix Projection;
Matrix Viewport;

class Camera {
public:
    Vec3f eye;
    Vec3f center;
    Vec3f up;

    Camera(Vec3f _eye = Vec3f(1, 1, 3), Vec3f _center = Vec3f(0, 0, 0), Vec3f _up = Vec3f(0, 1, 0))
        : eye(_eye), center(_center), up(_up) {
    }

    void lookat() {
        Vec3f z = (eye - center).normalize();
        Vec3f x = (up ^ z).normalize();
        Vec3f y = (z ^ x).normalize();

        Matrix Minv = Matrix::identity();
        Matrix Tr = Matrix::identity();

        for (int i = 0; i < 3; i++) {
            Minv[0][i] = x.raw[i];
            Minv[1][i] = y.raw[i];
            Minv[2][i] = z.raw[i];
            Tr[i][3] = -center.raw[i];
        }

        ModelView = Minv * Tr;
    }
};

void setProjection(float coeff = 0.0f) {
    Projection = Matrix::identity(4);
    Projection[3][2] = coeff;
}

void setViewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity(4);
    Viewport[0][3] = x + w / 2.0f;
    Viewport[1][3] = y + h / 2.0f;
    Viewport[2][3] = 255 / 2.0f;

    Viewport[0][0] = w / 2.0f;
    Viewport[1][1] = h / 2.0f;
    Viewport[2][2] = 255 / 2.0f;
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i].x = C.raw[i] - A.raw[i];
        s[i].y = B.raw[i] - A.raw[i];
        s[i].z = A.raw[i] - P.raw[i];
    }

    Vec3f u = s[0] ^ s[1];
    if (std::abs(u.z) > 1e-2) {
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }
    return Vec3f(-1, 1, 1);
}

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, float* zbuffer) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    Vec2f screen_coords[3];
    for (int i = 0; i < 3; i++) {
        if (pts[i].w != 0.0f) {
            screen_coords[i] = Vec2f(pts[i].x / pts[i].w, pts[i].y / pts[i].w);
        }
        else {
            screen_coords[i] = Vec2f(pts[i].x, pts[i].y);
        }

        for (int j = 0; j < 2; j++) {
            bboxmin.raw[j] = std::min(bboxmin.raw[j], screen_coords[i].raw[j]);
            bboxmax.raw[j] = std::max(bboxmax.raw[j], screen_coords[i].raw[j]);
        }
    }

    Vec2f clamp(static_cast<float>(image.get_width() - 1), static_cast<float>(image.get_height() - 1));
    bboxmin.x = std::max(0.0f, std::min(clamp.x, bboxmin.x));
    bboxmin.y = std::max(0.0f, std::min(clamp.y, bboxmin.y));
    bboxmax.x = std::max(0.0f, std::min(clamp.x, bboxmax.x));
    bboxmax.y = std::max(0.0f, std::min(clamp.y, bboxmax.y));

    Vec2i P;
    TGAColor color;
    for (P.x = static_cast<int>(bboxmin.x); P.x <= static_cast<int>(bboxmax.x); P.x++) {
        for (P.y = static_cast<int>(bboxmin.y); P.y <= static_cast<int>(bboxmax.y); P.y++) {
            Vec3f bc_screen = barycentric(screen_coords[0], screen_coords[1], screen_coords[2],
                Vec2f(static_cast<float>(P.x), static_cast<float>(P.y)));

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            float z = 0;
            float w = 0;
            for (int i = 0; i < 3; i++) {
                z += pts[i].z * bc_screen.raw[i];
                w += pts[i].w * bc_screen.raw[i];
            }

            int idx = P.x + P.y * width;
            if (zbuffer[idx] < z) {
                zbuffer[idx] = z;

                if (!shader.fragment(bc_screen, color)) {
                    image.set(P.x, P.y, color);
                }
            }
        }
    }
}

void render(Model* model, IShader& shader, TGAImage& image, float* zbuffer) {
    for (int i = 0; i < model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j = 0; j < 3; j++) {
            screen_coords[j] = shader.vertex(i, j);
        }
        triangle(screen_coords, shader, image, zbuffer);
    }
}

void render_texture_background(TGAImage& image, Model* model) {
    if (!model->has_texture_loaded()) {
        for (int i = 0; i < image.get_width(); i++) {
            for (int j = 0; j < image.get_height(); j++) {
                image.set(i, j, TGAColor(50, 50, 50, 255));
            }
        }
        return;
    }

    TGAImage texture = model->get_texture();

    for (int i = 0; i < image.get_width(); i++) {
        for (int j = 0; j < image.get_height(); j++) {
            float u = static_cast<float>(i) / image.get_width();
            float v = static_cast<float>(j) / image.get_height();

            TGAColor tex_color = model->get_texture_color(u, v);

            unsigned char gray = static_cast<unsigned char>(
                0.299f * tex_color.r + 0.587f * tex_color.g + 0.114f * tex_color.b
                );

            image.set(i, j, TGAColor(gray, gray, gray, 255));
        }
    }
}

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("african_head.obj");
    }

    if (!model->load_texture("texture.tga")) {
        std::cerr << "Failed to load background texture" << std::endl;
    }

    Camera camera(Vec3f(1, 1, 3), Vec3f(0, 0, 0), Vec3f(0, 1, 0));
    camera.lookat();

    setProjection(-1.0f / (camera.eye - camera.center).norm());
    setViewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    zBuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zBuffer[i] = -std::numeric_limits<float>::max();
    }

    TGAImage image(width, height, TGAImage::RGB);

    std::cerr << "Rendering background texture..." << std::endl;
    render_texture_background(image, model);

    std::cerr << "Rendering 3D model..." << std::endl;
    PhongShader shader(model);
    shader.uniform_M = Viewport * Projection * ModelView;
    shader.uniform_MIT = (Viewport * Projection * ModelView).invert_transpose();
    shader.uniform_light_dir = Vec3f(1, 0, -1).normalize();
    shader.uniform_eye_pos = camera.eye;

    render(model, shader, image, zBuffer);

    image.flip_vertically();
    image.write_tga_file("output.tga");

    std::cerr << "Render complete!" << std::endl;

    delete model;
    delete[] zBuffer;

    return 0;
}