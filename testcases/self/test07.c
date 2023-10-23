extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);


int main() {
   PRINT(sizeof(int));
   PRINT(sizeof(int *));
   PRINT(sizeof(char));
   PRINT(sizeof(123 * 123));
}
