extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int a = 10;
int main() {
   PRINT(a);
   a = GET();
   PRINT(a);
}