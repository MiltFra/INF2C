#=========================================================================
# XOR Cipher Cracking
#=========================================================================
# Finds the secret key for a given encrypted text with a given hint.
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

input_text_file_name:         .asciiz  "input_xor_crack.txt"
hint_file_name:                .asciiz  "hint.txt"
newline:                      .asciiz  "\n"
        
#-------------------------------------------------------------------------
# Global variables in memory
#-------------------------------------------------------------------------
# 
input_text:                   .space 10001       # Maximum size of input_text_file + NULL
.align 4                                         # The next field will be aligned
hint:                         .space 101         # Maximum size of key_file + NULL
.align 4                                         # The next field will be aligned
decrypt_text:                 .space 10001       # same as input
.align 4
key_text:                     .space 10
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


# opening file for reading (hint)

        li   $v0, 13                    # system call for open file
        la   $a0, hint_file_name        # hint file name
        li   $a1, 0                     # flag for reading
        li   $a2, 0                     # mode is ignored
        syscall                         # open a file
        
        move $s0, $v0                   # save the file descriptor 

        # reading from file just opened

        move $t0, $0                    # idx = 0

READ_LOOP1:                             # do {
        li   $v0, 14                    # system call for reading from file
        move $a0, $s0                   # file descriptor
                                        # hint[idx] = c_input
        la   $a1, hint($t0)             # address of buffer from which to read
        li   $a2,  1                    # read 1 char
        syscall                         # c_input = fgetc(key_file);
        blez $v0, END_LOOP1             # if(feof(key_file)) { break }
        lb   $t1, hint($t0)          
        addi $v0, $0, 10                # newline \n
        beq  $t1, $v0, END_LOOP1        # if(c_input == '\n')
        addi $t0, $t0, 1                # idx += 1
        j    READ_LOOP1
END_LOOP1:
        sb   $0,  hint($t0)             # hint[idx] = '\0'

        # Close the file 

        li   $v0, 16                    # system call for close file
        move $a0, $s0                   # file descriptor to close
        syscall                         # fclose(key_file)

#------------------------------------------------------------------
# End of reading file block.
#------------------------------------------------------------------
        li $t7,0                        # key = 0;
key_loop:
        li $t0,256
        beq $t7,$t0,key_not_found       # key == 0x100?
        li $t6,0                        # i = 0
decrypt_loop:
        lb $t5,input_text($t6)          # c = input_text[i]
        beqz $t5,decrypt_loop_end
        li $t0,10
        beq $t5,$t0,decrypt_is_whitespace
        li $t0,32
        beq $t5,$t0,decrypt_is_whitespace
        xor $t5,$t5,$t7
decrypt_is_whitespace:
        sb $t5,decrypt_text($t6)
        addi $t6,$t6,1
        j decrypt_loop
decrypt_loop_end:
        sb $t5,decrypt_text($t6)        # setting final null character
        la $t6,decrypt_text             # p = decrypt_text
compare_loop:
        lb $t0,0($t6)
        beqz $t0,compare_loop_end
        add $a0,$t6,$zero
        la $a1,hint
        jal compare_substring
        bnez $v0,key_found
skip_word_loop:
        lb $t0,0($t6)
        li $t1,10
        beq $t0,$t1,skip_word_loop_end
        li $t1,32
        beq $t0,$t1,skip_word_loop_end
        addi $t6,$t6,1
        j skip_word_loop
skip_word_loop_end:
        addi $t6,$t6,1                  # p++
        j compare_loop
compare_loop_end:
        addi $t7,$t7,1
        j key_loop
key_not_found:
        li $v0,11                      # syscall for exit with value
        li $a0,'-'
        syscall
        li $a0,'1'
        syscall
        li $a0,'\n'
        syscall
        j main_end
key_found:
        li $t0,9
        sb $zero,key_text($t0)
        li $t0,8                        # i = 8
        li $t1,10
        sb $t1,key_text($t0)
print_loop:
        beqz $t0,print_loop_end
        addi $t0,$t0,-1
        li $t1,1
        and $t1,$t1,$t7
        addi $t1,$t1,48
        sb $t1,key_text($t0)
        srl $t7,$t7,1
        j print_loop
print_loop_end:
        li $v0,4                        # syscall for print string
        la $a0,key_text
        syscall
        j main_end


compare_substring:                      # $a0 = p, $a1 = q, $v0 = return value
        lb $t0,0($a0)
        lb $t1,0($a1)
        beqz $t0,compare_substring_end
        li $t2,10
        bne $t0,$t2,compare_substring_not_eol 
        li $t0,32
compare_substring_not_eol:
        bne $t0,$t1,compare_substring_end
        addi $a0,$a0,1
        addi $a1,$a1,1
        j compare_substring
compare_substring_end:
        seq $v0, $t1, $zero
        li $t2,32
        seq $t3, $t0, $t2
        seq $t4, $t0, $zero
        or $t3, $t3, $t4
        and $v0,$v0,$t3
        jr $ra


#------------------------------------------------------------------
# Exit, DO NOT MODIFY THIS BLOCK
#------------------------------------------------------------------
main_end:      
        li   $v0, 10          # exit()
        syscall

#----------------------------------------------------------------
# END OF CODE
#----------------------------------------------------------------
