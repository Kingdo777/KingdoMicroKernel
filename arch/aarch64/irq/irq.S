
/**
 * @brief 设置异常向量表
 */
.global set_exception_vector
.type set_exception_vector, %function
set_exception_vector:
	adr	x0, el1_vector
	msr	vbar_el1, x0
	ret
.size set_exception_vector, .- set_exception_vector

/**
 * DAIFSet与DAIFClr: https://www.yuque.com/kingdo-8jdv5/xtd3c8/thqzxi9eaqe0f97m
 */
.global enable_irq
.type enable_irq, %function
enable_irq:
	msr	daifclr, #2
	ret
.size enable_irq, .- enable_irq

.global disable_irq
.type disable_irq, %function
disable_irq:
        msr     daifset, #2
        ret
.size disable_irq, .- disable_irq