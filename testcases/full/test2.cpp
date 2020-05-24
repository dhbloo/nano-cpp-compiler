// ConsoleApplication1.cpp : 定义控制台应用程序的入口点。
//

//#include "stdafx.h"


int a, b, c;
//int 1x;
//int x+1;
int x=3+a;
char cChar;
float m, n;
double cArray[100];
int b[10][10];
int d[10][10];

int test(int a);

typedef struct
{
	int a;
	int b;
}_TestStruct;
/*
//int array[10];
double *ff,gg;
*/
double *f, g;
int fibo(int a)
{
 	_TestStruct tt;
	if (a == 1 || a == 2)
		return 1;
	return fibo(a - 1) + fibo(a - 2);
	//just Test
	if (a > 10)
	return fibo(a - 1, a - 2);
		//
	if (b > 100)
		return fibo(a - 1) + fibo(a - 2);
	else
	{
		b--;
	}
	/*just a Test*/
	f = (double*)cArray;
	for (int j = 0; j < 100; j++)
	{
		g = cArray[j];
		g = *(f + j);
	}
}
//just a test
void testf()
{
	return cArray[0];
}


int _tmain(int argc, /*_TCHAR*/ char* argv[])
{
	int m=10, n, i;
	m = read();
        cChar='a';
        cChar='\\';
        cChar='\n'; 
        cChar = 1;
	g = *(&(d[0][0]));
	i = 1;
	while (i <= m)
	{
		n = fibo(i);
		//write(n);
		i = i + 1;
	}
        switch(a)
        {
         case 1: m++;break;
         case 2:m--;break;
         default: i=i-1; 
	}
	return 1;

}

