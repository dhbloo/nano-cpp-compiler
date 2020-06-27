int A[10][10];
int B[10][10];
int C[10][10];

void write(int x);
void putchar(int c);

int main()
{
    for (int i = 0; i < 10; i++) {
        A[i][i] = B[i][i] = 1;
    }

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            write(A[i][j]);
        }
        putchar('\n');
    }
    putchar('\n');

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            for (int k = 0; k < 10; k++) {
                int s   = 0;
                s       = s + A[i][k] * B[k][j];
                C[i][j] = s;
            }
        }
    }

    int sum = 0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            //sum = sum + C[i][j];
            write(C[i][j]);
        }
        putchar('\n');
    }

    return 0;
}