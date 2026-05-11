#include <stdio.h>
#include <unistd.h>

int main() {
	char a[] = "Hello, World";
	syscall(SYS_write, 0, &a, sizeof(a));
	return 0;
}
