#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <cmath>
#include <iostream>
#include <vector>
#include <cassert>

template <class t> struct Vec2 {
    union {
        struct { t u, v; };
        struct { t x, y; };
        t raw[2];
    };
    Vec2() : u(0), v(0) {}
    Vec2(t _u, t _v) : u(_u), v(_v) {}
    inline Vec2<t> operator +(const Vec2<t>& V) const { return Vec2<t>(u + V.u, v + V.v); }
    inline Vec2<t> operator -(const Vec2<t>& V) const { return Vec2<t>(u - V.u, v - V.v); }
    inline Vec2<t> operator *(float f)          const { return Vec2<t>(u * f, v * f); }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<t>& v);
};

template <class t> struct Vec3 {
    union {
        struct { t x, y, z; };
        struct { t ivert, iuv, inorm; };
        t raw[3];
    };
    Vec3() : x(0), y(0), z(0) {}
    Vec3(t _x, t _y, t _z) : x(_x), y(_y), z(_z) {}
    inline Vec3<t> operator ^(const Vec3<t>& v) const { return Vec3<t>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    inline Vec3<t> operator +(const Vec3<t>& v) const { return Vec3<t>(x + v.x, y + v.y, z + v.z); }
    inline Vec3<t> operator -(const Vec3<t>& v) const { return Vec3<t>(x - v.x, y - v.y, z - v.z); }
    inline Vec3<t> operator *(float f)          const { return Vec3<t>(x * f, y * f, z * f); }
    inline t       operator *(const Vec3<t>& v) const { return x * v.x + y * v.y + z * v.z; }
    float norm() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3<t>& normalize(t l = 1) { *this = (*this) * (l / norm()); return *this; }
    template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<t>& v);
};

template <class t> struct Vec4 {
    union {
        struct { t x, y, z, w; };
        t raw[4];
    };
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(t _x, t _y, t _z, t _w) : x(_x), y(_y), z(_z), w(_w) {}
};

class Matrix {
    std::vector<std::vector<float>> m;
    int rows, cols;

public:
    Matrix(int r = 4, int c = 4) : rows(r), cols(c) {
        m.resize(rows);
        for (int i = 0; i < rows; i++) {
            m[i].resize(cols, 0);
        }
    }

    static Matrix identity(int dimensions = 4) {
        Matrix E(dimensions, dimensions);
        for (int i = 0; i < dimensions; i++) {
            E[i][i] = 1;
        }
        return E;
    }

    int nrows() const { return rows; }
    int ncols() const { return cols; }

    std::vector<float>& operator[](const int i) {
        assert(i >= 0 && i < rows);
        return m[i];
    }

    const std::vector<float>& operator[](const int i) const {
        assert(i >= 0 && i < rows);
        return m[i];
    }

    Matrix operator*(const Matrix& a) const {
        Matrix result(rows, a.cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < a.cols; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < cols; k++) {
                    result.m[i][j] += m[i][k] * a.m[k][j];
                }
            }
        }
        return result;
    }

    Matrix transpose() const {
        Matrix result(cols, rows);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[j][i] = m[i][j];
            }
        }
        return result;
    }

    Matrix invert() const {
        assert(rows == 4 && cols == 4);
        Matrix result = identity();
        Matrix src = *this;

        for (int i = 0; i < 4; i++) {
            float diag = src[i][i];
            if (diag == 0) return identity();

            for (int j = 0; j < 4; j++) {
                src[i][j] /= diag;
                result[i][j] /= diag;
            }

            for (int k = 0; k < 4; k++) {
                if (k != i) {
                    float factor = src[k][i];
                    for (int j = 0; j < 4; j++) {
                        src[k][j] -= factor * src[i][j];
                        result[k][j] -= factor * result[i][j];
                    }
                }
            }
        }
        return result;
    }

    Matrix invert_transpose() const {
        return invert().transpose();
    }

    Vec4<float> operator*(const Vec4<float>& v) const {
        Vec4<float> result;
        for (int i = 0; i < rows; i++) {
            result.raw[i] = 0;
            for (int j = 0; j < cols; j++) {
                result.raw[i] += m[i][j] * v.raw[j];
            }
        }
        return result;
    }

    Vec3<float> operator*(const Vec3<float>& v) const {
        Vec4<float> v4(v.x, v.y, v.z, 1.0f);
        Vec4<float> result = (*this) * v4;
        if (result.w != 0.0f) {
            return Vec3<float>(result.x / result.w, result.y / result.w, result.z / result.w);
        }
        return Vec3<float>(result.x, result.y, result.z);
    }

    friend std::ostream& operator<<(std::ostream& s, Matrix& m) {
        for (int i = 0; i < m.nrows(); i++) {
            for (int j = 0; j < m.ncols(); j++) {
                s << m[i][j] << " ";
            }
            s << "\n";
        }
        return s;
    }
};

typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;
typedef Vec4<float> Vec4f;

template <class t> std::ostream& operator<<(std::ostream& s, Vec2<t>& v) {
    s << "(" << v.x << ", " << v.y << ")\n";
    return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec3<t>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
    return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec4<t>& v) {
    s << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")\n";
    return s;
}

#endif //__GEOMETRY_H__