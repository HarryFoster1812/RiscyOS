/* All-static C program without any standard library */

/* ---------- Enums ---------- */
enum Color { RED, GREEN, BLUE } favorite_color = GREEN;

/* ---------- Structs ---------- */
struct Point {
    int x;
    int y;
} origin = {0, 0};

/* ---------- Unions ---------- */
union Data {
    int i;
    float f;
    char c;
} data_union;

/* ---------- Function Pointer ---------- */
int add(int a, int b) {
    return a + b;
}
int (*static_func_ptr)(int, int) = add;

/* ---------- Static variables of all fundamental types ---------- */
char static_char = 'A';
signed char static_schar = -5;
unsigned char static_uchar = 200;

short static_short = -1000;
unsigned short static_ushort = 60000;

int static_int = 42;
unsigned int static_uint = 4000000000U;

long static_long = -100000L;
unsigned long static_ulong = 100000UL;

long long static_llong = -10000000000LL;
unsigned long long static_ullong = 10000000000ULL;

float static_float = 3.14f;
double static_double = 3.14159265359;
long double static_ldouble = 3.141592653589793238L;

/* ---------- Static pointer ---------- */
int *static_ptr = &static_int;

/* ---------- Static array ---------- */
int static_array[5] = {1, 2, 3, 4, 5};

/* ---------- Void function ---------- */
void do_nothing(void) {
    /* does nothing, just exists */
}

/* ---------- Main ---------- */
int main_static(void) {
    /* Operations to "use" all static variables */
    int sum = 0;
    sum += static_char;
    sum += static_schar;
    sum += static_uchar;

    sum += static_short;
    sum += static_ushort;

    sum += static_int;
    sum += static_uint;

    sum += static_long;
    sum += static_ulong;

    sum += static_llong;
    sum += static_ullong;

    /* array usage */
    for(int i = 0; i < 5; i++) {
        sum += static_array[i];
    }

    /* pointer usage */
    *static_ptr = *static_ptr + 1;

    /* struct usage */
    origin.x += 1;
    origin.y += 1;

    /* union usage */
    data_union.i = 123;
    data_union.f = 3.14f;
    data_union.c = 'Z';

    /* enum usage */
    if(favorite_color == GREEN) {
        favorite_color = BLUE;
    }

    /* function pointer usage */
    int r = static_func_ptr(5, 7);
    sum += r;

    /* void function usage */
    do_nothing();

    return sum; /* return value avoids unused-variable warnings */
}

/* entry point for compiler */
int main(void) {
    return main_static();
}
