extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

// 函数声明
int add(int, int);
int gcd(int, int);


int main() {
    int result = add(3, 4);
    PRINT(result);
    PRINT(gcd(3 * 1081, 7 * 1232));
    PRINT(gcd(3 * 5, 3));
    return 0;
}

// 函数定义
int add(int a, int b) {
    return a + b;
}
int gcd(int a, int b) {
    return b > 0 ? gcd(b, a % b) : a;
}