#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
Model* model = NULL;
const int width = 800;
const int height = 800;

//Z-буфер
float* zBuffer = nullptr;

void line(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color) {
    bool steep = false;
    if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
        steep = true;
    }
    if (p0.x > p1.x) {
        std::swap(p0, p1);
    }

    for (int x = p0.x; x <= p1.x; x++) {
        float t = (x - p0.x) / (float)(p1.x - p0.x);
        int y = p0.y * (1. - t) + p1.y * t;
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
    }
}

//барицентрические координаты
Vec3f barycentric(Vec2i A, Vec2i B, Vec2i C, Vec2i P) {
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
    return Vec3f(-1, 1, 1); //случай вырожденного треугольника = возвращаем отрицательные координаты
}

//Растеризация
void triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin.raw[j] = std::max(0.f, std::min(bboxmin.raw[j], pts[i].raw[j]));
            bboxmax.raw[j] = std::min(clamp.raw[j], std::max(bboxmax.raw[j], pts[i].raw[j]));
        }
    }

    Vec2i P;
    //Проходим по всем пикселям в ограничивающей рамке
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            //барицентрические координаты
            Vec3f bc_screen = barycentric(Vec2i(pts[0].x, pts[0].y),
                Vec2i(pts[1].x, pts[1].y),
                Vec2i(pts[2].x, pts[2].y),
                P);
            //Если точка внутри треугольника
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            //Интерполируем z-координату
            float z = 0;
            for (int i = 0; i < 3; i++) z += pts[i].z * bc_screen.raw[i];

            //Проверяем z-буфер
            int idx = P.x + P.y * width;
            if (zbuffer[idx] < z) {
                zbuffer[idx] = z;
                image.set(P.x, P.y, color);
            }
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

    //Инициализация z-буфера
    zBuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zBuffer[i] = -std::numeric_limits<float>::max();
    }

    TGAImage image(width, height, TGAImage::RGB);
    Vec3f light_dir(0, 0, -1);

    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];

        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            //Преобразуем в экранные координаты и сохраняем z-координату
            screen_coords[j] = Vec3f((v.x + 1.) * width / 2., (v.y + 1.) * height / 2., v.z);
            world_coords[j] = v;
        }

        //Вычисляем нормаль и интенсивность
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;

        if (intensity > 0) {
            TGAColor color = TGAColor(intensity * 255, intensity * 255, intensity * 255, 255);
            triangle(screen_coords, zBuffer, image, color);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");

    delete model;
    delete[] zBuffer;

    return 0;
}