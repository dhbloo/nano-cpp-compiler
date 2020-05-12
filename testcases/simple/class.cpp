struct A
{
    int   x, y;
    float z;

    A(int a = 0)
    {
        x = a;
        y = 0;
        z = 0.f;
    }
    ~A() { x = y = 0; }

    void print() const;
    int  ret(int a, int b) { return x + y + a + b; }

    A operator+(const A &other) const
    {
        A a;
        a.x = x + other.x;
        a.y = y + other.y;
        a.z = z + other.z;
        return a;
    }
    bool operator==(const A &other) { return x == other.x && y == other.y && z == other.z; }
         operator float() { return z; }
};

class B : public A
{
public:
    struct D
    {
        struct E;
        struct F
        {};
        int a;
        D(int a) : a(a) {}
    };
    B();
    virtual ~B();
    operator float() const;
    operator int() { return d.a; };

private:
    D d;
};

const class C : private B::D
{
    float  z;
    double w;

public:
    C(const A *pa, int px) : B::D(1),  a(pa) { x = px + 1; }
    ~C() { x = 0; }
    const B &get_b() const { return (const B &)*this; }

private:
    const A *a;
    int      x;
};

class B::D::E
{
    A a;
    B::D d;
    B::D::F f;
};

A       a, *pa, **ppa;
B       b, ab[10], ab[10][20];
C       c;
B::D    d;
B::D::E f;
B::D::F f;

class A xa, &pxa = xa;