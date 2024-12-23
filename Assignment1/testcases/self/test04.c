extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   int b = 10, c = GET();
   a = 10;
   if (a != 10) { // if stmt & binop
     b = 20;
   } else if (c > 1) {
     b = 0;
   } else {
     b = -(1 * 2); // unaryop
   }
   PRINT(a*b);
}
