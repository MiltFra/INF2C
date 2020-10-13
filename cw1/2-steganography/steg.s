#=========================================================================
# Steganography
#=========================================================================
# Retrive a secret message from a given text.
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

input_text_file_name:         .asciiz  "input_steg.txt"
newline:                      .asciiz  "\n"
        
#-------------------------------------------------------------------------
# Global variables in memory
#-------------------------------------------------------------------------
# 
input_text:                   .space 10001       # Maximum size of input_text_file + NULL
.align 4                                         # The next field will be aligned

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

# opening file for reading

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


#------------------------------------------------------------------
# End of reading file block.
#------------------------------------------------------------------

        la $t5,input_text               # char pointer
        li $a1,0                        # line counter
        li $a2,1                        # newline flag
        li $v0,11                       # system call for printing a character
line_loop:
        li $t1,0                        # number of words read
        lb $a0,0($t5)
        beq $a0,$zero,main_end
word_loop:
        beq $t1,$a1,found_word          # read enough words for the line
        addi $t1,$t1,1
        jal skip_word
        li $t7,32
        beq $a0,$t7,word_loop           # $a0 is unchanged from skip_word
                                        # if this does not get run, we are at the end
                                        # of the line without having skipped enough
                                        # words
        li $a0,10                       # loading \n
        syscall                         # printing newline
        li $a2,1
        j word_loop_end                     # we're not printing a word this time
found_word:
        jal print_word
        li $t7,10
        beq $t7,$a0,word_loop_end       # don't do any skipping if we're at the end of the line
        jal skip_rest_of_line           # may just be '\n'
word_loop_end:
        addi $a1,$a1,1                  # done reading one more line
        addi $t5,$t5,1                  # skip '\n' as well
        j line_loop

skip_word:
        lb $a0,0($t5)                   # reading character from input
        addi $t5,$t5,1                  # skip letter
        li $t7,10                       # '\n'
        beq $a0,$t7,skip_word_end
        li $t7,32                       # ' '
        beq $a0,$t7,skip_word_end
        j skip_word
skip_word_end:
        jr $ra	                        

skip_rest_of_line:
        lb $a0,0($t5)
        li $t7,10
        beq $a0,$t7,skip_rest_of_line_end # only stop if we find '\n'
        addi $t5,$t5,1                  # 
        j skip_rest_of_line
skip_rest_of_line_end:
        jr $ra

print_word:
        beq $a2,$zero,not_new_line
        li $a2,0
        j print_word_loop
not_new_line:                           # if we're not in a new line, print a space
        li $a0,32
        syscall
print_word_loop:
        lb $a0,0($t5)
        li $t7,10                       # encountered \n
        beq $a0,$t7,print_word_end
        li $t7,32                       # encountered ' '
        beq $a0,$t7,print_word_end
        syscall                         # print *p to stdout 
        addi $t5,$t5,1                  # move p right
        j print_word_loop
print_word_end:
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
