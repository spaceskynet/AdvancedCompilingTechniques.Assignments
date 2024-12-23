extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int a = 10 - 13 * 2, b = 3;
int main() {
   PRINT(a);
   int a = (3 + 4) * b; 
   PRINT(a);
   a = GET();
   PRINT(a);
}