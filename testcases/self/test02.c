extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int a = 10 + 13 * 2, b;
int main() {
   int c = 4 % 6, d = 8;
   b = GET();
   // b += 1;
   PRINT(a + b - c * d);
}