// extern Function declarations
extern int GET();
extern void* MALLOC(int);
extern void FREE(void*);
extern void PRINT(int);

// Function declarations
int add(int, int);

int arr[5];

int main() {
    int x, y, z;
    char c;
    void* ptr;
    int *parr;
    
    x = 10;
    y = sizeof(c);
    c = 'A';

    PRINT(c);
    // Basic arithmetic
    z = x / y + y ^ x;
    PRINT(z);

    // Function call
    int result = z ^ c ? add(x, y) : add(x, -y);
    PRINT(result);

    // if statement
    if (x > y) {
        PRINT(-2333);
    } else {
        PRINT(+114514);
    }

    // while loop
    while (x > 0) {
        PRINT(x);
        x--;
    }

    // for loop
    for (int i = 0, j = 1; i < 5; i++) {
        PRINT(i * j--);
    }

    // Pointer operations & sizeof
    ptr = MALLOC(sizeof(int));
    if (ptr != 0) { // NULL -> 0
        *(int*)ptr = 42;
        PRINT(*(int*)ptr);
        FREE(ptr);
    }
    parr = arr + 3;
    *parr = 123;

    // Print using external function
    PRINT(arr[3]);

    return 0;
}

int add(int a, int b) {
    return a + b;
}
