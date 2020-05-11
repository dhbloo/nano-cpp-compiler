
long int fibo(long int x)
{
    if (x == 0)
        return 0;
    else if (x == 1)
        return 1;
    return fibo(x - 1) + fibo(x - 2);
}

long fib(long num)
{
    long x = 0, y = 1, z = 0;
    for (long int i = 0; i < num; i++) {
        z = x + y;
        x = y;
        y = z;
    }
    return x;
}