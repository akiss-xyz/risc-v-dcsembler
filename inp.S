
__begin:
    addi x1, x0, 15
addi x2, x0, 7
sub x3, x1, x2
xori x4, x3, 1
sw x4, 2(x0)
lw x5, 2(x0)

slli x1, x2, 3
srli x1, x2, 3
srai x1, x2, 3

hello:
ecall
ebreak

jal x1, 65620

auipc x1, 13
lui x1, 3
