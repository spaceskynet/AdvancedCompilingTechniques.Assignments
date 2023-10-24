extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);


int main() {
   int a = 0;
   for (int i = 0; ; ++i) {
      PRINT(i);
      if (a >= 100) break;
      a += i;
      if (i % 3) {
         PRINT(a);
      }
      else continue;
   }
   PRINT(a);
   return 0;
}
