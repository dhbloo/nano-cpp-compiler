class A
{
    int   x, y;
    float z;

    void print() const;
    int ret(int a) { return x + y + a; }

    operator float() { return z; }
};

class B : public A {};

class C : private B {
private:
    const A* a;
    int x;

public:
    /*C(const A* pa, int px) : a(pa) {
        x = px + 1;
    }*/
    ~C() {
        x = 0;
    }
    const B& get_b() const {
        return (const B&)*this;
    }
};

A a, *pa, **ppa;
B b, *ab[10], **ab[10][20];
C c;
