struct A
{
    int   x, y;
    float z;

    A();
    virtual ~A();

    void print() const;
    int  ret(int a, int b) { return x + y + a + b; }

    A operator+(const A &other)
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
    class D {
        int a;
    };
private:
    D d;
};

const class C : private ::B
{
    float  z;
    double w;

public:
    /*C(const A* pa, int px) : a(pa) {
        x = px + 1;
    }*/
    ~C() { x = 0; }
    const B &get_b() const { return (const B &)*this; }

private:
    const A *a;
    int      x;
};

A a, *pa, **ppa;
B b, ab[10], ab[10][20];
C c;

class A xa, &pxa = xa;