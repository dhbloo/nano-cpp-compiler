class A
{
    int   x, y;
    float z;

    void print() const;
    int ret(int a) { return x + y + a; }

    operator float() { return z; }
};

//class B : public A {};