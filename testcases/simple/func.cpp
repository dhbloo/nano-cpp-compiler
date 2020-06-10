class A
{
public:
    int   x, y;
    float z;

    virtual int  getX() const { return x; }
    void         setY(const int &newy) { y = newy; }
    friend float getz(const A &a) { return a.z; }
    static void  f();
};

struct B : A
{
    struct C
    {
        int a;
        friend class A;
    } c;
    int a;
    // virtual int getX() const { return a; }
    int          getX() const = 0;
    virtual void f();
};

enum E { CON1, CON2, CON3 = CON2 * 2 } b;
typedef int *INTP;
int          g = 10;
int          a[5 + 5];
const char   str[5 * 2] = "123456789";

int f()
{
    int  a = 1, b = 2;
    INTP d = &b;
    long c = 3, &e = c;
    B    t;
    B::C r;
    t.x   = 10;
    t.c.a = 20;
    r     = t.c;
    t.setY(20);
    d = &t.x;

    *d = a >> 1 | b << t.getX();
    return g * a + b * e / *d - t.y * r.a;
}

int max(int x, int y)
{
    return x > y ? x : y;
}

void p(const int &x, const char str[])
{
    if (g + x > 0)
        p(x - 1, "hello");
    else
        a[x]++;

    switch (b) {
    case CON1: break;
    default: g--; return;
    }
}