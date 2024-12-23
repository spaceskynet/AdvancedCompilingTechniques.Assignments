extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int a = 10 + 13 * 2;
int main() {
   int b = GET();
   int a = b > 2 ? -1 : 1; 
   PRINT(a);
}