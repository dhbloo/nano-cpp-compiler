int a = 5;
int b;
int arr[10];
int arr2[10][10];

struct S
{
    int  x, y;
    char c[10 * 10];
};

int g();

float f(int i)
{
    int x = 0, *px = 0;
    x = 1 + 2 + 3;
    x += i + 1;
    px  = &x;
    *px = a;

    return x;
}

int g()
{
    S s;

    s.y         = b;
    s.x         = s.y + 1;
    s.c[a % 99] = 2;
    return s.x;
}

int h(int x, int y)
{
    arr2[a][b] = x;

    int *pa = arr;
    pa[a]   = y;
    return pa[y];
}

int abs(int x)
{
    return x >= 0 ? x : 0.0f - x;
}

int main()
{
    b      = a + 1;
    arr[3] = 5;
    arr[a] = a;

    if (b > 10) {
        return b;
        b++;
    }
    else {
        ++a;
    }

    a = 10;
    while (a > 0) {
        --a;
    }

    b = 0;
    do {
        b = b * 2 + 1;
    } while (b <= a + 10);

    for (int i = 0; i < 100; ++i) {
        a += i++;
        if (a > 1000 || i > 50)
            break;
        else if (i & 1)
            continue;
        else if (a > 500 && i == 32)
            a = a - 5;

        b = a + b;
    }

    a = b % 10;
    switch (a) {
    case 1:
        a = 2;
        break;
    case 3:
    case 4:
        a = 5;
        break;

    default:
        a = 0;
        break;
    }

    f(100);
    arr2[1][2] = g() + h(b, arr[a]);

    return a + b;
}