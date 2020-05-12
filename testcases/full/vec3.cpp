class Vec3f
{
public:
    float x, y, z;
    Vec3f(float i = 0.0f) : x(i), y(i), z(i) {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    ~Vec3f();

    void  clear() { x = y = z = 0; }
    Vec3f operator*(float k) { return Vec3f{x * k, y * k, z * k}; }
    bool  operator==(const Vec3f &v) { return x == v.x && y == v.y && z == v.z; }
          operator int() const { return (int)(x + y + z); }
};

int main(int argc, char *argv[])
{
    Vec3f a(1.f), b(8.f);
    int   i = 0;
    while (!(a == b)) {
        b = b * 0.5f;
    }
    return i;
}