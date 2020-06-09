enum A {
    INT = 1,
    LONG = 2,
    FLOAT = 3,
    DOUBLE
} a;

enum {
    RED, GREEN, BLUE
} rgb = BLUE;

A b;
const enum A c = FLOAT;

void f() {
    b = LONG;
    rgb = GREEN;
}