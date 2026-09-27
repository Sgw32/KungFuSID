;
; Startup code for cc65 (C64 version)
; modified
;
; This must be the *first* file on the linker command line
;
    .export         __STARTUP__ : absolute = 1      ; Override c64.lib's crt0.s
    
    .import        initlib, donelib, callirq
    .import        zerobss
    .import        callmain
    .import        RESTOR, BSOUT, CLRCH
    .import        __INTERRUPTOR_COUNT__
    .import        __MAIN_START__, __MAIN_SIZE__    ; Linker generated

    .importzp       sp, sreg, regsave
    .importzp       ptr1, ptr2, ptr3, ptr4
    .importzp       tmp1, tmp2, tmp3, tmp4
    .importzp       regbank

; ------------------------------------------------------------------------
; Place the startup code in a special segment.

.segment           "STARTUP"

; BASIC header with a SYS call

        .word   Head            ; Load address
Head:   .word   @Next
        .word   .version        ; Line number
        .byte   $9E,"2061"      ; SYS 2061
        .byte   $00             ; End of BASIC line
@Next:  .word   0               ; BASIC end marker

; ------------------------------------------------------------------------
; Actual code

        lda #$36        ; Hide BASIC/CART
        sta $01

        ; Close open files
        jsr CLRCH

        ; Switch to second charset
        lda #14
        jsr BSOUT

        ; Clear the BSS data
        jsr zerobss

        ; Set argument stack ptr
        lda #<(__MAIN_START__ + __MAIN_SIZE__)
        sta sp
        lda #>(__MAIN_START__ + __MAIN_SIZE__)
        sta sp+1

        ; Call module constructors
        jsr initlib

        ; Call main - will never return
        jmp callmain