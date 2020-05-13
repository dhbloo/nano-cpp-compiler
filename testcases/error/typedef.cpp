typedef int I;
typedef float I;
typedef I G(I);
typedef unsigned int UI, FI(int), (*PFI)(int);

class I;
enum I;

typedef int Q;

int main() {
    if (Q x = 10; x == 10) {
        return 1;
    } else if (Q{15} > 5)
        return 2;
}