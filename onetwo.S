
initialise:
    addi x10, x0, 1
    addi x11, x0, 2
    addi x12, x0, 3

set_target:
    addi x13, x0, 15

keep_adding:
    addi x10, x10, 1
    addi x11, x11, 2
    addi x12, x12, 3

check_if_any_greater_than:
    bge x10, x13, done
    bge x11, x13, done
    bge x12, x13, done

    jal x0, keep_adding

done:
    nop
    nop
    jal x0, done
