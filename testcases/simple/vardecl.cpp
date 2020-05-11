int a;
char b;
float c;
double d;

int x, y, z;
long i, j, k;

unsigned int aa;
signed char bb;
long int cc;
short int dd;
unsigned long int ee;
signed short int ff;

int t = 1, &rt = t;
float u = 2.0f;
char v = 'r';

int arr[10], arr2[5][6], arr3[2][3][4];

int *pa, **ppa, *paa[5], &ra = t;
char ** ppb, *&prb;

void f(int);
void (*p1)(int) = &f;
void (*p2)(int) = f;

const int ci = 10, *pc = &ci, *const cpc = pc, **ppc;
int ii, *p, *const cp = &ii;

int (*(*x[2])())[3];
char* (*foo)(char*);
char* (*(*foo[5])(char*))[10];
char * const (*(* const bar)[5])(int);