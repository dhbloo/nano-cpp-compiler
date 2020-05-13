float sqrt(float x);
struct Vec3f
{
    float x, y, z;
    Vec3f(float i = 0.0f) : x(i), y(i), z(i) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    virtual ~Vec3f();
    void   clear() { x = y = z = 0; }
    float &operator[](int i)
    {
        switch (i) {  // comment: 0 <= i <= 2
        case 0: return x;
        case 1: return y;
        default: return z;
        }
    }
    Vec3f operator+(Vec3f v) const { return Vec3f {x + v.x, y + v.y, z + v.z}; }
          operator int() const { return (int)(sqrt(x * x + y * y + z * z)); }
};
class Mat33f
{
    Vec3f *r[3];
    void (*p)(int, Vec3f **);
    Vec3f (*(*fx[2])(long int x))[3][4];
public:
    Mat33f(Vec3f &a) { r[0] = r[1] = r[2] = &a; }
    Vec3f &     operator[](int i) { return *r[i]; }
    const char *name() { return "Matrix"; }
};
/*  main function  */
int main(int argc, char *argv[])
{
    Vec3f  a(3.f), b(5.f, 7.f, 9.f), c = 1.0f;
    Mat33f m(a);
    int    i;
    for (i = 0; i < 100 && (int)a < (int)b; i++) {
        if (m[0][i % 3] > 5.0f)
            m[i % 3][0] *= 0.1f;
        else
            a = a + c;
    }
    return i;
}