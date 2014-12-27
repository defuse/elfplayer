#include <stdio.h>

void foo()
{
    return;
}

void bar()
{
    int x;
    x = 5;
    x *= 3;
    x += 6;
    printf("Hello! %d\n", x);
}

int main(int argc, char **argv)
{
    int i = 0;
    for (i = 0; i < 100; i++) {
        if (i % 2 == 0) {
            foo();
        } else {
            bar();
        }
    }
}
