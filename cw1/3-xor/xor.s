#=========================================================================
# XOR Cipher Encryption
#=========================================================================
# Encrypts a given text with a given key.
# 
# Inf2C Computer Systems
# 
# Dmitrii Ustiugov
# 9 Oct 2020
# 
#
#=========================================================================
# DATA SEGMENT
#=========================================================================
.data
#-------------------------------------------------------------------------
# Constant strings
#-------------------------------------------------------------------------

input_text_file_name:         .asciiz  "input_xor.txt"
key_file_name:                .asciiz  "key_xor.txt"
newline:                      .asciiz  "\n"
        
#-------------------------------------------------------------------------
# Global variables in memory
#-------------------------------------------------------------------------
# 
input_text:                   .space 10001       # Maximum size of input_text_file + NULL
.align 4                                         # The next field will be aligned
key:                          .space 33          # Maximum size of key_file + NULL
.align 4                                         # The next field will be aligned
key_bin:                      .space 4           # The key is at most 4 bytes
.align 4

# You can add your data here!

#=========================================================================
# TEXT SEGMENT  
#=========================================================================
.text

#-------------------------------------------------------------------------
# MAIN code block
#-------------------------------------------------------------------------

.globl main                     # Declare main label to be globally visible.
                                # Needed for correct operation with MARS
main:
#-------------------------------------------------------------------------
# Reading file block. DO NOT MODIFY THIS BLOCK
#-------------------------------------------------------------------------

# opening file for reading (text)

        li   $v0, 13                    # system call for open file
        la   $a0, input_text_file_name  # input_text file name
        li   $a1, 0                     # flag for reading
        li   $a2, 0                     # mode is ignored
        syscall                         # open a file
        
        move $s0, $v0                   # save the file descriptor 

        # reading from file just opened

        move $t0, $0                    # idx = 0

READ_LOOP:                              # do {
        li   $v0, 14                    # system call for reading from file
        move $a0, $s0                   # file descriptor
                                        # input_text[idx] = c_input
        la   $a1, input_text($t0)             # address of buffer from which to read
        li   $a2,  1                    # read 1 char
        syscall                         # c_input = fgetc(input_text_file);
        blez $v0, END_LOOP              # if(feof(input_text_file)) { break }
        lb   $t1, input_text($t0)          
        beq  $t1, $0,  END_LOOP        # if(c_input == '\0')
        addi $t0, $t0, 1                # idx += 1
        j    READ_LOOP
END_LOOP:
        sb   $0,  input_text($t0)       # input_text[idx] = '\0'

        # Close the file 

        li   $v0, 16                    # system call for close file
        move $a0, $s0                   # file descriptor to close
        syscall                         # fclose(input_text_file)


# opening file for reading (key)

        li   $v0, 13                    # system call for open file
        la   $a0, key_file_name         # key file name
        li   $a1, 0                     # flag for reading
        li   $a2, 0                     # mode is ignored
        syscall                         # open a file
        
        move $s0, $v0                   # save the file descriptor 

        # reading from file just opened

        move $t0, $0                    # idx = 0

READ_LOOP1:                             # do {
        li   $v0, 14                    # system call for reading from file
        move $a0, $s0                   # file descriptor
                                        # key[idx] = c_input
        la   $a1, key($t0)              # address of buffer from which to read
        li   $a2,  1                    # read 1 char
        syscall                         # c_input = fgetc(key_file);
        blez $v0, END_LOOP1             # if(feof(key_file)) { break }
        lb   $t1, key($t0)          
        addi $v0, $0, 10                # newline \n
        beq  $t1, $v0, END_LOOP1        # if(c_input == '\n')
        addi $t0, $t0, 1                # idx += 1
        j    READ_LOOP1
END_LOOP1:
        sb   $0,  key($t0)              # key[idx] = '\0'

        # Close the file 

        li   $v0, 16                    # system call for close file
        move $a0, $s0                   # file descriptor to close
        syscall                         # fclose(key_file)

#------------------------------------------------------------------
# End of reading file block.
#------------------------------------------------------------------


        li $t0,0                        # num bits
count_loop:
        lb $t1, key($t0)
        beq $t1,$zero,count_loop_end 
        addi $t0, $t0, 1
        j count_loop
count_loop_end:
        srl $t0,$t0,3                   # num bytes = num bits >> 3
        la $t6,key_bin($zero)           # key_begin
        add $t7,$t6,$t0                 # key_end
        li $t1,0                        # loop counter
key_bin_loop:
        li $t2,0                        # byte value
        li $t3,0                        # inner loop counter
        li $t4,8                        # inner loop end
byte_loop:
        sll $t2,$t2,1                   # byte_value <<= 1
        sll $t5,$t1,3                   # idx = i * 8
        add $t5,$t5,$t3                 # idx += j
        lb $t5,key($t5)                 # c = key_text[idx]
        addi $t5,$t5,-48                # c -= '0'
        sgt $t5,$t5,$zero               # b = c > 0
        add $t2,$t2,$t5                 # byte_value += b
        addi $t3,$t3,1                  # j++
        beq $t3,$t4,byte_loop_end       # j == 8?
        j byte_loop
byte_loop_end:
        sb $t2,key_bin($t1)             # key_bin[i] = byte_value
        addi $t1,$t1,1                  # i++
        beq $t1,$t0,key_bin_loop_end    # i == num_bytes?
        j key_bin_loop
key_bin_loop_end:
        la $t0,input_text($zero)        # p = input_text
        add $t1,$t6,$zero               # q = key_begin
        li $v0,11                       # syscall for printing characters
encrypt_loop:
        lb $a0,0($t0)                   # c = *p
        beq $a0,$zero,encrypt_loop_end  # c == '\0'?
        li $t2,32                       
        beq $a0,$t2,print_char          # c == ' '?
        li $t2,10
        beq $a0,$t2,print_char          # c == '\n'?
        lb $t2,0($t1)                   # k = *q
        xor $a0,$a0,$t2                 # c = c ^ k
print_char:
        syscall                         # putchar(c)
        addi $t0,$t0,1                  # p++
        addi $t1,$t1,1                  # q++
        bne $t1,$t7,encrypt_loop
        add $t1,$t6,$zero
        j encrypt_loop
encrypt_loop_end:

#------------------------------------------------------------------
# Exit, DO NOT MODIFY THIS BLOCK
#------------------------------------------------------------------
main_end:      
        li   $v0, 10          # exit()
        syscall

#----------------------------------------------------------------
# END OF CODE
#----------------------------------------------------------------
