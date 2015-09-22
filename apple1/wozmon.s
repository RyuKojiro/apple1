; Wozniak Apple1 Monitor

.org $FF00	; Rom Start

;******* Hardware Variables ************

DSP   = $D012	; Video I/O
DSPCR = $D013

KBD   = $D010	; Keyboard I/O
KBDCR = $D011

;********* Zero Page Variables *********

XAML = $24
XAMH = $25
STL  = $26
STH  = $27
L    = $28
H    = $29
YSAV = $2A
MODE = $2B

;******** $200-$27F Text Buffer *********

IN = $0200

;******** Listing *************

RESET:          CLD             ; Clear decimal arithmetic mode.
                CLI
                LDY #$7F        ; Mask for DSP data direction register
                STY DSP         ; Set it up.
                LDA #$A7        ; KBD and DSP control register mask.
                STA KBDCR       ; Enable interrupts, set CA1, CB1, for
                STA DSPCR       ;  postitive edge sense/output mode.
NOTCR:          CMP #$DF        ; "<-"?
                BEQ BACKSPACE   ; Yes.
                CMP #$9B        ; ESC?
                BEQ ESCAPE      ; Yes.
                INY             ; Advance text index.
                BPL NEXTCHAR    ; Auto ESC if > 127.
ESCAPE:         LDA #$DC        ; "\".
                JSR ECHO        ; Output it.
GETLINE:        LDA #$8D        ; CR.
                JSR ECHO        ; Output it.
                LDY #$01        ; Initiallize text index.
BACKSPACE:      DEY             ; Backup text index.
                BMI GETLINE     ; Beyond start of line, reinitialize
NEXTCHAR:       LDA KBDCR       ; Key ready?
                BPL NEXTCHAR    ; Loop until ready.
                LDA KBD         ; Load character. B7 shoul be '1'
                STA IN,Y        ; Add to text buffer.
                JSR ECHO        ; Display character.
                CMP #$8D        ; CR?
                BNE NOTCR       ; No.
                LDY #$FF        ; Reset text index.
                LDA #$00        ; For XAM mode.
                TAX             ; 0->X.
SETSTOR:        ASL A           ; Leaves $7B if setting STOR mode.
SETMODE:        STA MODE        ; $00 = XAM, $7B = STOR, $A3 = BLOK XAM
BLSKIP:         INY             ; Advance text index.
NEXTITEM:       LDA IN,Y        ; Get character.
                CMP #$8D        ; CR?
                BEQ GETLINE     ; Yes, done this line.
                CMP #$A3        ; "."?
                BCC BLSKIP      ; Skip delimiter.
                BEQ SETMODE     ; Set BLOCK XAM mode.
                CMP #$BA        ; ":"?
                BEQ SETSTOR     ; Yes, set STOR mode.
                CMP #$D2        ; "R"?
                BEQ RUN         ; Yes, run user program.
                STX L           ; $00->L.
                STX H           ;  and H.
                STY YSAV        ; Save Y for comparison.
NEXTHEX:        LDA IN,Y        ; Get character for hex test.
                EOR #$B0        ; Map digits to $0-9.
                CMP #$0A        ; Digit?
                BCC DIG         ; Yes.
                ADC #$88        ; Map letter "A"-"F" to $FA-FF.
                CMP #$FA        ; Hex letter?
                BCC NOTHEX      ; No, character not hex.
DIG:            ASL A
                ASL A           ; Hex digit to MSD of A.
                ASL A           ;
                ASL A           ;
                LDX #$04        ; Shift count.
HEXSHIFT:       ASL A           ; Hex digit left, MSDB to carry.
                ROL L           ; Rotate into LSD.
                ROL H           ; Rotate into MSD's.
                DEX             ; Done 4 shifts?
                BNE HEXSHIFT    ; No, loop.
                INY             ; Advance text index.
                BNE NEXTHEX     ; Always taken. Check next character for hex.
NOTHEX:         CPY YSAV        ; Check if L, H empty (no hex digits)
                BEQ ESCAPE      ; Yes, generate ESC sequence.
                BIT MODE        ; Test MODE byte.
                BVC NOTSTOR     ; B6 = 0 for STOR, 1 for XAM and BLOCK XAM
                LDA L           ; LSD's of hex data.
                STA (STL,X)     ; Store at current 'store index'.
                INC STL         ; Increment store index.
                BNE NEXTITEM    ; Get next item. (no carry).
                INC STH         ; Add carry to 'store index' high order.
TONEXTITEM:     JMP NEXTITEM    ; Get next command item.
RUN:            JMP (XAML)      ; Run at current XAM index.
NOTSTOR:        BMI XAMNEXT     ; B7 = 0 for XAM, 1 for BLOCK XAM.
                LDX #$02        ; Byte count.
SETADR:         LDA L-1,X       ; Copy hex data to
                STA STL-1,X     ;  'store index'.
                STA XAML-1,X    ; And to 'XAM index'.
                DEX             ; Next of 2 bytes.
                BNE SETADR      ; Loop unless X = 0.
NXTPRNT:        BNE PRDATA      ; NE means no address to print.
                LDA #$8D        ; CR.
                JSR ECHO        ; Output it.
                LDA XAMH        ; 'Examine index' high-order byte.
                JSR PRBYTE      ; Output it in hex format.
                LDA XAML        ; Low-order 'examine index' byte.
                JSR PRBYTE      ; Output it in hex format.
                LDA #$BA        ; ":".
                JSR ECHO        ; Output it.
PRDATA:         LDA #$A0        ; Blank.
                JSR ECHO        ; Output it.
                LDA (XAML,X)    ; Get data byte at 'examine index'
                JSR PRBYTE      ; Output it in hex format.
XAMNEXT:        STX MODE        ; 0->MODE (XAM mode).
                LDA XAML
                CMP L           ; Comapre 'examine index' to hex data.
                LDA XAMH        ; 
                SBC H
                BCS TONEXTITEM  ; Not less, so no more data to output.
                INC XAML        ; 
                BNE MOD8CHK     ; Increment 'examine index'.
                INC XAMH        ; 
MOD8CHK:        LDA XAML        ; Check low-order 'examine index' byte
                AND #$07        ;  For MOD 8=0
                BPL NXTPRNT     ; Always taken.
PRBYTE:         PHA             ; Save A for LSD.
                LSR A
                LSR A
                LSR A           ; MSD to LSD position.
                LSR A
                JSR PRHEX       ; Output hex digit.
                PLA             ; Restore A.
PRHEX:          AND #$0F        ; Make LSD for hex print.
                ORA #$B0        ; Add "0".
                CMP #$BA        ; Digit?
                BCC ECHO        ; Yes, output it.
                ADC #$06        ; Add offset for letter.
ECHO:           BIT DSP         ; DA bit (B7) cleared yet?
                BMI ECHO        ; No, wait for display
                STA DSP         ; Output character. Sets DA.
                RTS             ; Return.

.byte $0000
.byte $0F00                     ; NMI
.byte $FF00                     ; RESET
.byte $0000                     ; IRQ
