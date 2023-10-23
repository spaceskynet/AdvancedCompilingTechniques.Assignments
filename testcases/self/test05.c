extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int a = 1, b;
int main() {
   b = GET();
   // b += 1;
   PRINT(++a + b++);
   PRINT(a-- + --b);
   PRINT(a);
   PRINT(b);
}