
int main()
{
    int *x = new int, *a = new int(1 + 2 + 3);
    int *y = new int[10], *b = new (int[5]);
    int(*z)[20] = new int[10][20], (*c)[6] = new (int[5][6]);

    z[0][0] += y[0] * *x;

    delete x;
    delete[] y;
    delete[] z;
    delete[] b;
    delete[] c;
}