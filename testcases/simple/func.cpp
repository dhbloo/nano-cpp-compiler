int g = 10;
int f()
{
    int   a = 1, b = 2, d;
    short c = 3;

    d = a >> 1 | b << 3;
    return g * a % h + b * c / d;
}

int max(int x, int y) {
    return x > y ? x : y;
}

void print(const int &x) {
    if (g + x > 0)
        print(x - 1);
    else
        g++;
}