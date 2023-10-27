extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int cnt[100] = {0};

int main() {
   for (;;);
   int c = GET();
   int a[GET() * ++c];
   int b[233] = {0};
   int d[3] = {0, 1, 2};
   int e[4] = {0, 1};
   int *n = b;
   c = *n++;
   c = *(n++);
   c = *++n;
   c = *(++n);
   c = ++*n;
   n += 1;
   n = n - 1;
   cnt[0] = *n;
   while(1) {
      if(1) break;
   }
   while(0);
   if (1) {}
   int **x = &n;
}
