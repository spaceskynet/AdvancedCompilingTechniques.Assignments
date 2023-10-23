extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a = -100;
   for (int a = 0; a < 10; a += 1) {
      PRINT(++a);
   }

   PRINT(a);
}
