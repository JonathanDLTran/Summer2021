#include <stdio.h>

int main(int argc, const char** argv) {
    int num;
    scanf("%i", &num);
    int x = 1 + num;
    int y = num + num;
    int k = num * 2;
    int z = 4 + num;
    // num = num + num;
    // num = num + num;
    printf("%i\n", k);
    printf("%i\n", x);
    printf("%i\n", y);
    printf("%i\n", z);
    printf("%i\n", num);
    int j = num + 10;
    printf("%i\n", j);
    int l = num + 10;
    printf("%i\n", l);
}
