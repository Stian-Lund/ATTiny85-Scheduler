.global start_task
.global suspend_task

; The X-register is a special register designed to be used with address pointers.
; The X-register high byte is = r27 and the X-register low byte is = r26
;
; sts | k, Rr | Store Direct to SRAM     | (k) <- Rr
; lds | rd, k | Load Direct from SRAM    | Rd <- (k)
; ld  | rd, X | Load Indirect            | Rd <- (X)
; ld  | rd, X+| Load Indirect & Post-Inc | Rd <- (X), X <- X+1
; st  | X, Rr | Store Indirect           | (X)<- Rr
; st  | X+,Rr | Store Indirect & Post-Inc| (X)<- Rr, X <- X+1
; ret |       | Subroutine Return        | PC <- STACK

start_task:
    ; Store the stackpointer to kernel_sp variable
    lds r18, SPL
    sts kernel_sp, r18
    lds r18, SPH
    sts kernel_sp+1, r18

    ; Load X register with current_task stack pointer
    lds r26, current_task
    lds r27, current_task+1

    ; Writes X register to SPL and SPH
    ld r18, X+      ; Loads X into r18 and then increments X by 1
    sts SPL, r18
    ld r18, X
    sts SPH, r18

    ret

suspend_task:
    ; save the task stack pointer
    lds r26, current_task
    lds r27, current_task+1
    lds r18, SPL
    st X+, r18
    lds r18, SPHs
    st X, r18

    ; Restore the main task SP
    lds r18, kernel_sp
    sts SPL, r18
    lds r18, kernel_sp+1
    sts SPH, r18
    ret

