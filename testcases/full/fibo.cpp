void write(int x);

int fibo(int x)
{
    if (x <= 2)
        return 1;
    return fibo(x - 1) + fibo(x - 2);
}

int fib(int num)
{
    int x = 0, y = 1, z = 0;
    for (int i = 0; i < num; i++) {
        z = x + y;
        x = y;
        y = z;
    }
    return x;
}

int main()
{
    write(fibo(10));
    write(fib(10));
    return 0;
}