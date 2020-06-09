int    a;
char   b;
float  c;
double d;

const int x(1), y(1 + 2), z = 2 + 3;
long      i, j, k;

unsigned int      aa;
signed char       bb;
long int          cc;
short int         dd;
unsigned long int ee;
signed short int  ff;

int   t = 1, &rt = t, w(1 + 2), &rw(w);
float u = 2.0f;
char  v = 'r';

int   arr[10], arr2[5][6], arr3[2][3][4];
int   iarr[4] = {1, 2, 3, 4};
int   iarr2[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
void *varr[3];

int *  pa, **ppa, *paa[5], &ra = t;
char **ppb, *&prb = *ppb;

void f(int);
void (*p1)(int) = &f;
void (*p2)(int) = f;

const int ci = 10, *pc = &ci, *const cpc = pc, **ppc;
int ii, *p, *const cp                    = &ii;

int (*(*fx[2])())[3];
char *(*foo)(char *);
char *(*(*fuu[5])(char *))[10];
char *const (*(*const bar)[5])(int);
char *const (*(**const (**zz[1])[2])[3])[4];

// int **&* error1;
// void error2[10];
// void* error3[10][];
