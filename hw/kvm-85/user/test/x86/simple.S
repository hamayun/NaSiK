
	.text

	mov $0, %al
	mov $10000, %ebx
1:
	mov %ebx, %ecx
2:	
	loop 2b
	out %al, $0x80
	inc %al
	add $10000, %ebx
	jmp 1b
