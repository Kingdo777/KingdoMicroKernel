.section .rodata
hw_str: .string "HelloKMK"
.size hw_str, .-hw_str

.section .text
.global _start;
.type _start, %function;
_start: 
	adr x0, hw_str
	ldr x1, [x0]
	b 	.
.size _start, .- _start