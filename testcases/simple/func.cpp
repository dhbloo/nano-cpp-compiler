class A
{
public:
    int   x, y;
    float z;

    int  getX() const { return x; }
    void setY(const int &newy) { y = newy; }
    friend float getz(const A &a) { return a.z; }
};

class B : public A
{
    int a;
};

enum E { CON1, CON2 } b;
typedef int *INTP;
int          g = 10;
int          a[10];

int f()
{
    int  a = 1, b = 2;
    INTP d = &b;
    long c = 3, &e = c;
    A    t;
    t.x = 10;
    t.setY(20);

    *d = a >> 1 | b << t.getX();
    return g * a + b * e / *d - t.y;
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