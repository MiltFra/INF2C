#=========================================================================
# Book Cipher Decryption
#=========================================================================
# Decrypts a given encrypted text with a given book.
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

input_text_file_name:         .asciiz  "input_book_cipher.txt"
book_file_name:               .asciiz  "book.txt"
newline:                      .asciiz  "\n"
        
#-------------------------------------------------------------------------
# Global variables in memory
#-------------------------------------------------------------------------
# 
input_text:                   .space 10001       # Maximum size of input_text_file + NULL
.align 4                                         # The next field will be aligned
book:                         .space 10001       # Maximum size of book_file + NULL
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
        la   $a1, input_text($t0)       # address of buffer from which to read
        li   $a2,  1                    # read 1 char
        syscall                         # c_input = fgetc(input_text_file);
        blez $v0, END_LOOP              # if(feof(input_text_file)) { break }
        lb   $t1, input_text($t0)          
        beq  $t1, $0,  END_LOOP         # if(c_input == '\0')
        addi $t0, $t0, 1                # idx += 1
        j    READ_LOOP
END_LOOP:
        sb   $0,  input_text($t0)       # input_text[idx] = '\0'

        # Close the file 

        li   $v0, 16                    # system call for close file
        move $a0, $s0                   # file descriptor to close
        syscall                         # fclose(input_text_file)


# opening file for reading (book)

        li   $v0, 13                    # system call for open file
        la   $a0, book_file_name        # book file name
        li   $a1, 0                     # flag for reading
        li   $a2, 0                     # mode is ignored
        syscall                         # open a file
        
        move $s0, $v0                   # save the file descriptor 

        # reading from file just opened

        move $t0, $0                    # idx = 0

READ_LOOP1:                              # do {
        li   $v0, 14                    # system call for reading from file
        move $a0, $s0                   # file descriptor
                                        # book[idx] = c_input
        la   $a1, book($t0)             # address of buffer from which to read
        li   $a2,  1                    # read 1 char
        syscall                         # c_input = fgetc(book_file);
        blez $v0, END_LOOP1              # if(feof(book_file)) { break }
        lb   $t1, book($t0)          
        beq  $t1, $0,  END_LOOP         # if(c_input == '\0')
        addi $t0, $t0, 1                # idx += 1
        j    READ_LOOP1
END_LOOP1:
        sb   $0,  book($t0)             # book[idx] = '\0'

        # Close the file 

        li   $v0, 16                    # system call for close file
        move $a0, $s0                   # file descriptor to close
        syscall                         # fclose(book_file)

#------------------------------------------------------------------
# End of reading file block.
#------------------------------------------------------------------


# You can add your code here!
        la $t0,input_text               # p = input_text
        li $t4,1                        # new_line = 1
        li $v0,11                       # syscall for putchar
input_loop:
        lb $t5,0($t0)
        beq $t5,$zero,input_loop_end    # *p == '\0'?
        li $t2,0                        # line = 0
        li $t3,0                        # word = 0
        li $t6,32                       # t = ' '
        li $t7,10                       # f = 10
read_line_loop:
        lb $t5,0($t0)                   # c = *p
        beq $t5,$t6,read_line_loop_end  # c == t? (= ' ')
        mul $t2,$t2,$t7                 # line *= f (= 10)
        addi $t5,$t5,-48                # c -= '0'
        add $t2,$t2,$t5                 # line += c
        addi $t0,$t0,1                  # p++
        j read_line_loop
read_line_loop_end:
        addi $t0,$t0,1                  # p++ (Skipping the space)
read_word_loop:
        lb $t5,0($t0)                   # c = *p
        beq $t5,$t7,read_word_loop_end  # c == f? (= '\n')
        mul $t3,$t3,$t7                 # word *= f (= 10)
        addi $t5,$t5,-48                # c -= '0'
        add $t3,$t3,$t5                 # word += c    
        addi $t0,$t0,1                  # p++
        j read_word_loop                
read_word_loop_end:
        addi $t0,$t0,1                  # p++ (Skipping newline)
        la $t1,book($zero)
        addi $t2,$t2,-1                 # make line zero-based
        addi $t3,$t3,-1                 # make word zero-based
        li $t6,10                       # t = '\n'

skip_lines_loop:
        beqz $t2,skip_lines_loop_end    # line == 0?
        lb $t5,0($t1)                   # c = *q
        bne $t5,$t6,skip_lines_not_eol  # c == t? (= '\n')
        addi $t2,$t2,-1                 # line--
skip_lines_not_eol:
        addi $t1,$t1,1                  # q++
        j skip_lines_loop
skip_lines_loop_end:
        li $t6,32                       # t = ' '
        li $t7,10                       # n = '\n'

skip_words_loop:
        beqz $t3,skip_words_loop_end    # word == 0?
        lb $t5,0($t1)                   # c = *q
        bne $t5,$t6,skip_words_not_eow  # *q != ' '?
        addi $t3,$t3,-1
skip_words_not_eow:
        bne $t5,$t7,skip_words_not_eol  # *q != '\n'?
        li $a0,10                       # out_c = '\n'
        syscall                         # putchar(out_c)
        li $t4,1                        # new_line = 1
        j input_loop
skip_words_not_eol:
        addi $t1,$t1,1
        j skip_words_loop
skip_words_loop_end:
        beqz $t4,not_newline
        li $t4,0
        j print_word_loop

not_newline:
        li $a0,32
        syscall
print_word_loop:
        lb $a0,0($t1)                   # c = *q
        beq $a0,$t6,print_word_loop_end # c == ' '?
        beq $a0,$t7,print_word_loop_end # c == '\n'?
        syscall
        addi $t1,$t1,1
        j print_word_loop
print_word_loop_end:
        j input_loop 
input_loop_end:
        li $a0,10
        syscall



#------------------------------------------------------------------
# Exit, DO NOT MODIFY THIS BLOCK
#------------------------------------------------------------------
main_end:      
        li   $v0, 10          # exit()
        syscall

#----------------------------------------------------------------
# END OF CODE
#----------------------------------------------------------------
