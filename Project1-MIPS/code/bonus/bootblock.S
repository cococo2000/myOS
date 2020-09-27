.text
.global main

main:
	# 1) call BIOS print string "It's bootblock!"
	dla $a0, msg
	ld $t0, printstr
	jal $t0

	# 2) call BIOS read kernel in SD card
	ld $a0, kernel
	li $a1, 0x200
	# li $a2, 0x200
	lw $a2, kernel_size
	dsll $a2, $a2, 9
	ld $t0, read_sd_card
	ld $ra, kernel_main
	jr $t0

	# 3) jump to kernel
	ld $t0, kernel_main
	jal $t0

# 4) while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .ascii "It's a bootloader...\n"

# 1. PMON read SD card function address
# read_sd_card();
read_sd_card: .dword 0xffffffff8f0d5e10

# 2. PMON print string function address
# printstr(char *string)
printstr: .dword 0xffffffff8f0d5534

# 3. PMON print char function address
# printch(char ch)
printch: .dword 0xffffffff8f0d5570

# 4. kernel address (move kernel to here ~)
kernel : .dword 0xffffffffa0800000

# 5. kernel main address (jmp here to start kernel main!)
kernel_main : .dword 0xffffffffa0800000

kernel_size : .dword 0xffffffffa08001fc