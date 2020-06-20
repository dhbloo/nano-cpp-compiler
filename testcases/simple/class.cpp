struct A {
    A() : x(1), y(1) {}
    int sum() { return x + y; }

    int x;
    int y;
};

int main() {
    A a;
    return 0;
}