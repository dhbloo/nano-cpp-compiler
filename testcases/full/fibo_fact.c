void write(int x);

int fibo(int x)
{
    if (x <= 2)
        return 1;
    return fibo(x - 1) + fibo(x - 2);
}

int fact(int n) 
{
    int ret = 1;
    for (int i = 2; i <= n; i++) {
        ret *= i;
    }
    return ret;
}

int main()
{
    write(fibo(10));
    write(fact(5));
    return 0;
}