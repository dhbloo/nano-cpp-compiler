float sqrt(float x);
class Vec3f
{
public:
    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float xx) : x(xx), y(xx), z(xx) {}
    Vec3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
    Vec3f operator+(const Vec3f &v) const { return Vec3f {x + v.x, y + v.y, z + v.z}; }
    Vec3f operator-(const Vec3f &v) const { return Vec3f {x - v.x, y - v.y, z - v.z}; }
    Vec3f operator*(const float &r) const { return Vec3f {x * r, y * r, z * r}; }
    float operator*(const Vec3f &v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3f cross(const Vec3f &v) const
    {
        return Vec3f {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    float        norm() const { return x * x + y * y + z * z; }
    float        length() const { return sqrt(norm()); }
    const float &operator[](unsigned int i) const { return (&x)[i]; }
    float &      operator[](unsigned int i) { return (&x)[i]; }
    Vec3f &      normalize()
    {
        float n = norm();
        if (n > 0) {
            float factor = 1 / sqrt(n);
            x *= factor, y *= factor, z *= factor;
        }

        return *this;
    }
    float x, y, z;
};

class Matrix44f
{
public:
    float x[4][4];

    Matrix44f() { x[0][0] = x[1][1] = x[2][2] = x[3][3] = 1; }

    Matrix44f(float a,
              float b,
              float c,
              float d,
              float e,
              float f,
              float g,
              float h,
              float i,
              float j,
              float k,
              float l,
              float m,
              float n,
              float o,
              float p)
    {
        x[0][0] = a;
        x[0][1] = b;
        x[0][2] = c;
        x[0][3] = d;
        x[1][0] = e;
        x[1][1] = f;
        x[1][2] = g;
        x[1][3] = h;
        x[2][0] = i;
        x[2][1] = j;
        x[2][2] = k;
        x[2][3] = l;
        x[3][0] = m;
        x[3][1] = n;
        x[3][2] = o;
        x[3][3] = p;
    }

    const float *operator[](unsigned int i) const { return x[i]; }
    float *      operator[](unsigned int i) { return x[i]; }

    // Multiply the current matrix with another matrix (rhs)
    Matrix44f operator*(const Matrix44f &v) const
    {
        Matrix44f tmp;
        multiply(*this, v, tmp);

        return tmp;
    }

    static void multiply(const Matrix44f &a, const Matrix44f &b, Matrix44f &c)
    {
        for (unsigned int i = 0; i < 4; ++i) {
            for (unsigned int j = 0; j < 4; ++j) {
                c[i][j] =
                    a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
            }
        }
    }

    // return a transposed copy of the current matrix as a new matrix
    Matrix44f transposed() const
    {
        Matrix44f t;
        for (unsigned int i = 0; i < 4; ++i) {
            for (unsigned int j = 0; j < 4; ++j) {
                t[i][j] = x[j][i];
            }
        }
        return t;
    }

    // transpose itself
    Matrix44f &transpose()
    {
        Matrix44f tmp(x[0][0],
                      x[1][0],
                      x[2][0],
                      x[3][0],
                      x[0][1],
                      x[1][1],
                      x[2][1],
                      x[3][1],
                      x[0][2],
                      x[1][2],
                      x[2][2],
                      x[3][2],
                      x[0][3],
                      x[1][3],
                      x[2][3],
                      x[3][3]);
        *this = tmp;

        return *this;
    }
    void multVecMatrix(const Vec3f &src, Vec3f &dst) const
    {
        float a, b, c, w;

        a = src[0] * x[0][0] + src[1] * x[1][0] + src[2] * x[2][0] + x[3][0];
        b = src[0] * x[0][1] + src[1] * x[1][1] + src[2] * x[2][1] + x[3][1];
        c = src[0] * x[0][2] + src[1] * x[1][2] + src[2] * x[2][2] + x[3][2];
        w = src[0] * x[0][3] + src[1] * x[1][3] + src[2] * x[2][3] + x[3][3];

        dst.x = a / w;
        dst.y = b / w;
        dst.z = c / w;
    }
    void multDirMatrix(const Vec3f &src, Vec3f &dst) const
    {
        float a, b, c;

        a = src[0] * x[0][0] + src[1] * x[1][0] + src[2] * x[2][0];
        b = src[0] * x[0][1] + src[1] * x[1][1] + src[2] * x[2][1];
        c = src[0] * x[0][2] + src[1] * x[1][2] + src[2] * x[2][2];

        dst.x = a;
        dst.y = b;
        dst.z = c;
    }
    Matrix44f inverse()
    {
        int       i, j, k;
        Matrix44f s;
        Matrix44f t(*this);

        // Forward elimination
        for (i = 0; i < 3; i++) {
            int pivot = i;

            float pivotsize = t[i][i];

            if (pivotsize < 0)
                pivotsize = -pivotsize;

            for (j = i + 1; j < 4; j++) {
                float tmp = t[j][i];

                if (tmp < 0)
                    tmp = -tmp;

                if (tmp > pivotsize) {
                    pivot     = j;
                    pivotsize = tmp;
                }
            }

            if (pivotsize == 0) {
                // Cannot invert singular matrix
                return Matrix44f{};
            }

            if (pivot != i) {
                for (j = 0; j < 4; j++) {
                    float tmp;

                    tmp         = t[i][j];
                    t[i][j]     = t[pivot][j];
                    t[pivot][j] = tmp;

                    tmp         = s[i][j];
                    s[i][j]     = s[pivot][j];
                    s[pivot][j] = tmp;
                }
            }

            for (j = i + 1; j < 4; j++) {
                float f = t[j][i] / t[i][i];

                for (k = 0; k < 4; k++) {
                    t[j][k] -= f * t[i][k];
                    s[j][k] -= f * s[i][k];
                }
            }
        }

        // Backward substitution
        for (i = 3; i >= 0; --i) {
            float f;

            if ((f = t[i][i]) == 0) {
                // Cannot invert singular matrix
                return Matrix44f{};
            }

            for (j = 0; j < 4; j++) {
                t[i][j] /= f;
                s[i][j] /= f;
            }

            for (j = 0; j < i; j++) {
                f = t[j][i];

                for (k = 0; k < 4; k++) {
                    t[j][k] -= f * t[i][k];
                    s[j][k] -= f * s[i][k];
                }
            }
        }

        return s;
    }

    // set current matrix to its inverse
    const Matrix44f &invert()
    {
        *this = inverse();
        return *this;
    }
};

int main(int argc, char **argv)
{
    Matrix44f a, b, c;
    c = a * b;

    Matrix44f d(0.707107,
                0,
                -0.707107,
                0,
                -0.331295,
                0.883452,
                -0.331295,
                0,
                0.624695,
                0.468521,
                0.624695,
                0,
                4.000574,
                3.00043,
                4.000574,
                1);
    d.invert();

    return 0;
}