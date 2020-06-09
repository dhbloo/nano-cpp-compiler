struct A
{
    int        x, y, *i;
    float      z;
    static int a;

    A(int a = 0)
    {
        x = a;
        y = 0;
        z = 0.f;
    }
    ~A() { x = y = 0; }

    void print() const;
    int  ret(int a, int b) { return x + y + a + b; }
    // int  ret(int a) const { return x + y + a; }

    A operator+(const A &other) const
    {
        A a;
        a.x = x + other.x;
        a.y = y + other.y;
        a.z = z + other.z;
        return a;
    }
    bool operator==(const A &other)
    {
        return x == other.x && y == other.y && z == other.z;
    }
    operator float() { return z; }
};

class B : public A
{
public:
    struct D : A
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
    friend A;
    friend const D &get_d(const B &b) { return b.d; }
};

class C : private B::D
{
    float  z = 1.0f;
    double w;

public:
    C(const A *pa, int px) : B::D(1), a(pa) { x = px + 1; }
    ~C() { x = 0; }
    const B &get_b() const { return (const B &)*this; }

private:
    const A *a;
    int      x;
};

struct B::D::E
{
    A       a;
    B::D    d;
    B::D::F f;

    class H
    {};
};

B::D::E::H h;

B::operator float() const
{
    return 2.5f;
}

int     A::a = 10;
A       a, *pa, **ppa;
B       b, ab[10], ab[10][20];
C       c;
B::D    d;
B::D::E f;
B::D::F f;

struct
{
    typedef int D;
} _struct;

struct
{
    typedef int* D;
} _struct2;

class A xa, &pxa = xa;
int A::*mpa, B::D::*mpdb, A::**mppa = &mpa, *A::*pmpa = &A::i;