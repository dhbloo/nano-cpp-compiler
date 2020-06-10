
int main(int, char*[]);
int f(int (), int (int));
int g(int (*)(), int (*)(int));
int h(), i(int);

int main(int argc, char* argv[]) {
    int (*a)(int (), int (int)) = (int (*)(int func1(), int(*)(int)))f;
    int (*b)(int (*)(), int (*)(int)) = (int (*)(int (*func1)(), int (*)(int)))g;
    int c(int x);

    return a(h, i) + b(h, i);
}

int f(int a(), int b(int)) {
    return a() + b(a());
}

int g(int (*a)(), int (*b)(int)) {
    return a() + b(a());
}