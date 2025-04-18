.section .text

.global early_put32;
.type early_put32, %function;
early_put32:
	str w1, [x0]
	ret
.size early_put32, .- early_put32

.global early_get32;
.type early_get32, %function;
early_get32:
	ldr w0, [x0]
	ret
.size early_get32, .- early_get32

.global delay;
.type delay, %function;
delay:
	subs x0, x0, #1
	bne delay
	ret
.size delay, .- delay