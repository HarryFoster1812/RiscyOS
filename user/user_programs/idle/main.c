int main(){
    while (1) {
        asm volatile("wfi");  // Wait for Interrupt
    }
		return 0; 
}
