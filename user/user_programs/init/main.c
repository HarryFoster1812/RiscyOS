#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

char hello[] = "Hello, World!";

int main() {

//  write(0, &hello, sizeof(hello))
	// Fork
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		execv("/shell", NULL);
		exit(1);
	}             // Parent
	while (1) {
		wait(NULL);
	}

	return 0;
}
