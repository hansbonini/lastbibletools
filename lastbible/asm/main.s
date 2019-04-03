
.include "sys/sms_arch.s"
  ;.include "base/ram.s"
;.include "base/macros.s"
  ;.include "res/defines.s"

.rombankmap
  bankstotal 64
  banksize $4000
  banks 64
.endro

.emptyfill $FF

.background "lastbible.gg"

.unbackground $80000 $FFFFF

;======================
; free unused space
;======================

; end-of-banks
.unbackground $3F4D $3FFF
.unbackground $7E00 $7FEF
.unbackground $FD60 $FFEF
.unbackground $1BBC0 $1BFEF

; diacritic pair table
.unbackground $1EF4 $1F59

.include "vwf_consts.inc"
.include "ram.inc"
.include "util.s"
.include "vwf.s"
.include "vwf_user.s"

;.macro orig_read16BitTable
;  rst $20
;.endm

; B = tile count
; DE = srcptr
; HL = dstcmd
.macro rawTilesToVdp_macro
  ; set vdp dst
  ld c,vdpCtrlPort
  out (c),l
  out (c),h
  ; write data to data port
  ex de,hl
  dec c
  ld a,b
  -:
    .rept bytesPerTile
      push ix
      pop ix
      outi
    .endr
    dec a
    jp nz,-
.endm

; B = tile count
; DE = srcptr
; HL = dstcmd
.macro rawTilesToVdp_macro_safe
  ; set vdp dst
  ld c,vdpCtrlPort
  out (c),l
  nop
  out (c),h
  nop
  ; write data to data port
  ex de,hl
  dec c
  ld a,b
  -:
    .rept bytesPerTile
      push ix
      pop ix
      outi
    .endr
    dec a
    jp nz,-
.endm

; B = tile count
; DE = srcptr
; HL = srccmd
.macro rawTilesFromVdp_macro
  ; set vdp src
  ld c,vdpCtrlPort
  out (c),l
  nop
  out (c),h
  nop
  ; read data from data port
  ex de,hl
  dec c
  ld a,b
  -:
    .rept bytesPerTile
      push ix
      pop ix
      ini
    .endr
    dec a
    jp nz,-
.endm

; BC = tile count
; DE = srcptr
; HL = dstcmd
.macro rawTilesToVdp_big_macro
  push bc
    ; set vdp dst
    ld c,vdpCtrlPort
    out (c),l
    nop
    out (c),h
    nop
  pop bc
  ; write data to data port
  ex de,hl
  -:
    push bc
      ld c,vdpDataPort
      .rept bytesPerTile
        push ix
        pop ix
        outi
      .endr
    pop bc
    
    dec bc
    ld a,b
    or c
    jr nz,-
.endm

.macro old_read16BitTable
  rst $28
.endm

;===============================================
; Update header after building
;===============================================
.smstag

;========================================
; local defines
;========================================

; ram
.define buttonsPressed $C440
.define textSpeedSetting $C504
.define allySlotsStart $C5C0
.define nameEntryX $CB61
.define nameEntryY $CB62
.define nameEntryBuffer $CB63
.define nameEntryCurIndex $CB6A
.define nameEntryCurPartyMemId $CB6B
.define enemySlotsStart $C8C0
.define old_printBaseX $CF90 
.define old_printBaseY $CF91
.define textSpeed $CF92
.define old_lineNum $CF94
.define old_curPrintNametableAddr $CF97
.define activeAllyId $CF99
.define activeSpellId $CF9A
.define activeItemId $CF9B
.define activeEnemyId $CF9D
.define instantPrintFlag $CF9F
.define printBufferPtr $CFC0
.define openWindowCount $CFC2
.define bcdResultDigitCount $D20C
.define bcdResultDigits $D20D
.define frameCounter $DFE9

; routines
.define old_useJumpTable $0006
.define waitVblank $B9
.define pollInput $3AC
.define getCharSlotPointerPlusC $14C6
.define getCharSlotPointer $14CF
.define lookUpAllySlot $14D3
.define lookUpEnemySlot $14E5
.define lookUpSpell $14ED
.define lookUpItem $14FA
.define old_localCoordsToNametableAddr $1145
.define old_setCharArea $1197
.define toBcd $13A1
.define instaPrintBank4String $1E91
.define doPrintDelay $1E9E
.define setPrintAddrForCurLine $1EDA
.define old_printChar $2101
.define old_printTile $2119
.define old_incPrintNametableAddrRow $2139
.define openWindow $2A3C
.define restoreWindowBackground $2ADB
.define saveWindowBackgroundFromParams $2B55
.define runMenu $3A23
.define runMenuLogic $3A58

; constants
.define oldSpaceIndex $90
.define dialogueBoxMode1Size $1204
.define dialogueBoxMode1ContentPos $010D
.define dialogueBoxMode2Size $1202
.define dialogueBoxMode2ContentPos $0109

; NEW
.define speechEmulationDisableFlag $C53F

.define maxPlayerNameLen 7
.define maxOrigNameTileLen 7
.define maxNameTileLen 12

;========================================
; vwf settings
;========================================

;  ld a,vwfTileSize_main
;  ld b,vwfScrollZeroFlag_main
;  ld c,vwfNametableHighMask_main
;  ld hl,vwfTileBase_main
;  doBankedCall setUpVwfTileAlloc

;========================================
; remove bizarre and pointless software
; delay that's added after a hard reset
; only if machine region is japanese
;========================================

.bank $01 slot 1
.org $3D43
.section "no bootup delay" overwrite
  ; init memory if needed
  call $7D82
  call newStartup
.ends

.unbackground $7D55 $7D81

.bank $01 slot 1
.section "extra startup code" free
  newStartup:
    ; init vwf
    ld a,vwfTileSize_main
    ld b,vwfScrollZeroFlag_main
    ld c,vwfNametableHighMask_main
    ld hl,vwfTileBase_main
    doBankedCallSlot1 setUpVwfTileAlloc
    
    ret
.ends

;========================================
; preserve RAM which we want to be
; persistent across uses of the
; graphic decompression buffer area
; that we've put our new stuff in
;========================================

.bank $00 slot 0
.org $09A4
.section "preserve special ram 1" overwrite
  doBankedCallSlot1 preserveSpecialRam_ext
  jp $9B5
.ends

.slot 1
.section "preserve special ram 2" superfree
  preserveSpecialRam_ext:
    ; make up work
    ld a,b
    ld ($FFFF),a
    
    ; save important values
    ld (scratch),hl
    ld hl,(vwfAllocationArraySize)
    push hl
    ld hl,(vwfAllocationArrayBaseTile)
    push hl
    ld hl,(assumeScrollZeroFlag)
    push hl
      
      ;=====
      ; copy allocation array to expansion RAM
      ;=====
    
      ; disable interrupts while expansion ram in use (if not already off)
      ; TODO: is this actually necessary?
/*      ld a,c
      or a
      jr z,+
        di
      +: */
      
      ld a,(cartRamCtrl)
      push af
      push bc
      
        ; enable expansion RAM at 8000-BFFF
        or $08
        ld (cartRamCtrl),a
        
        ; copy data
        push de
          ld bc,maxVwfTiles
          ld de,expRam_vwfAllocationArrayBackup_compression
          ld hl,vwfAllocationArray
          ldir
        pop de
      
      pop bc
      pop af
      ld (cartRamCtrl),a
      
/*      ld a,c
      or a
      jr z,+
        ei
      +: */
      
      ;=====
      ; do decompression
      ;=====
      
      push bc
        ld hl,(scratch)
        ; make up work
        ld a,c
        ; reset buffer?
        call $09BA
        ; set up params?
        call $09D2
        ; decompress
        call $09EE
        ; reset buffer
        call $09BA
      pop bc
      
      ;=====
      ; copy allocation array back from expansion RAM
      ;=====
    
      ; disable interrupts while expansion ram in use (if not already off)
/*      ld a,c
      or a
      jr z,+
        di
      +: */
      
      ld a,(cartRamCtrl)
      push af
      push bc
      
        ; enable expansion RAM at 8000-BFFF
        or $08
        ld (cartRamCtrl),a
        
        ; copy data
        ld bc,maxVwfTiles
        ld de,vwfAllocationArray
        ld hl,expRam_vwfAllocationArrayBackup_compression
        ldir
      
      pop bc
      pop af
      ld (cartRamCtrl),a
      
/*      ld a,c
      or a
      jr z,+
        ei
      +: */
    
    ; restore important values
    pop hl
    ld (assumeScrollZeroFlag),hl
    pop hl
    ld (vwfAllocationArrayBaseTile),hl
    pop hl
    ld (vwfAllocationArraySize),hl
    ret
    
.ends

;========================================
; script
;========================================

.include "out/script/string_bucket_hashtabledialogue.inc"

.slot 2
.section "enemy names" superfree
  enemyNames:
    .incbin "out/script/enemies.bin"
  enemyNamesPlural:
    .incbin "out/script/enemies_plural.bin"
  
  
.ends

;========================================
; new main print routines
;========================================

  ;========================================
  ; new dialogue printing
  ;========================================

  ; HL = local x/y
  ; IX = string pointer
  .macro printString_logic_macro
    push ix
    push de
      ; save target x/y
      push hl
        ; get string pointer in HL
        push ix
        pop hl
        ; look up hashed string
        call getStringHash
        ; get hashed pointer in IX
        push hl
        pop ix
      ; restore target x/y
      pop hl
      
      ; switch to target bank
      ld a,(mapperSlot2Ctrl)
      push af
        ld a,c
        
        ; check if bank switch needed (shouldn't happen unless error)
        or a
        jp m,+
          ld (mapperSlot2Ctrl),a
        +:
        ; print hashed string
        doBankedCallSlot1 startNewString
      pop af
      ld (mapperSlot2Ctrl),a
    pop de
    pop ix
  .endm
  
  .bank $00 slot 0
  .org $1D74
  .section "new printString 1" SIZE $4B overwrite
    printString:
      printString_logic_macro
      
      ; callers expect IX to now point to one past the original string's
      ; terminator, so seek through it.
      ; we don't need to parse any ops -- the only ones with parameters
      ; are [char] and [textspeed], which should never use FF anyways
      -:
        ld a,(ix+0)
        inc ix
        cp terminatorIndex
        jr nz,-
      
      ret
  .ends

  ;========================================
  ; no diacritics
  ;========================================

  /*.bank $00 slot 0
  .org $1D95
  .section "no diacritics main print 1" overwrite
    jr c,printLiteral
  .ends

  .bank $00 slot 0
  .org $1DB5
  .section "no diacritics main print 2" overwrite
    printLiteral:
  ;    call printChar
      call $2101
  .ends

  .unbackground $1DA1 $1DB4 */

  ;========================================
  ; new menu string printing
  ;========================================
  
  .bank $00 slot 0
  .org $2BBF
  .section "new menu printing 1" SIZE $27 overwrite
    newMenuPrint:
      ; read x/y, but do not convert to sms-local
      ld a,(hl)
;      add a,$06
      ld d,a
      inc hl
      ld a,(hl)
;      add a,$03
      ld e,a
      inc hl
      ; DE = srcdata pointer
      ex de,hl
    
      doBankedCallSlot1 newMenuPrint_ext
      jp $2BE6
  .ends
  
  .slot 1
  .section "new menu printing 2" superfree
    ; wow this is pretty extravagant and wasteful (remapping every
    ; individual string when we could just be remapping the whole menu
    ; data structure, calling the dialogue print routine instead of
    ; the limited printer (because IT DOESN'T EXIST YET), etc.)
    ; but good enough?
    newMenuPrint_ext:
      ; B = length of each item
      ; C = item count
      ; DE = pointer to first menu item
      ; HL = gg-local x/y
      
      push hl
      push de
      push bc
        ; deallocate any existing content (e.g. if menu is redrawn over
        ; itself)
        doBankedCallSlot1 deallocVwfTileArea
        
        ; TODO: can we get away with skipping this?
        ; clear menu area
        ; convert gg-local coords to sms-local
        ld de,(screenVisibleX<<8)|screenVisibleY
        add hl,de
        ; clear with space
        ld a,oldSpaceIndex
        call old_setCharArea
      pop bc
      pop de
      pop hl
      
      ; instant printing on
      ld a,(instantPrintFlag)
      push af
        ld a,$FF
        ld (instantPrintFlag),a
        
        push ix
        -:
          ; IX = old item pointer
          push de
          pop ix
          
          push hl
          push bc
            ; we can't do this because our new printString expects to find
            ; a terminator at the end of the data
    ;        doBankedCallSlot1 printString
            ; this macro does everything printString does except the terminator
            ; seek.
            ; this would be problematic if we needed to print menus out of
            ; slot 1, since we're executing this code out of that slot
            ; and can't hash pointers to it, but that doesn't seem
            ; to ever happen
            printString_logic_macro
          pop bc
          pop hl
          
          ; A = length of item
          ld a,b
          ; add to DE to get pointer to original next item
          add a,e
          ld e,a
          jr nc,+
            inc d
          +:
          
          ; move to next line
          inc l
          dec c
          jr nz,-
        pop ix
        
;       call old_localCoordsToNametableAddr
;        set 6,h
;        ld (old_curPrintNametableAddr),hl
;
;      -:
;        push bc
;          push hl
;          ; print string
;          --:
;            ld a,(de)
;            inc de
;            call old_printChar
;            djnz --
;          pop hl
;          ld (old_curPrintNametableAddr),hl
;          call old_incPrintNametableAddrRow
;        pop bc
;        dec c
;        jr nz,-
      
      ; instant printing off
      pop af
      ld (instantPrintFlag),a
      
      ret
  .ends

;========================================
; redirect certain old op routines that
; are also called manually in code to
; the new ones, so vwf deallocation will
; occur correctly
;========================================

  ;========================================
  ; box clear
  ;========================================

  .bank $00 slot 0
  .org $1FFC
  .section "use new clear (manual)" SIZE $37 overwrite
    push bc
    push hl
      ; assume mode1 box
      ld bc,dialogueBoxMode1ContentPos
      ld (printBaseXY),bc
      
      doBankedCallSlot1 handleOp_clear
    pop hl
    pop bc
    ret
  .ends

;========================================
; reset VWF on map transitions
; (not really necessary, but saves us
; from handling special cases like
; end of intro)
;========================================
  
.bank $01 slot 1
.org $04CF
.section "reset vwf on map transitions 1" overwrite
  call resetVwfOnMapTransition
.ends
  
.bank $01 slot 1
.section "reset vwf on map transitions 2" free
  resetVwfOnMapTransition:
    doBankedCallSlot1 fullyResetVwf
;    doBankedCallSlot1 saveExpandedVwf
    
    ; because stuff like the load game menu couldn't be assed
    ; properly closing the windows
    ld a,(vwfStateSuspended)
    or a
    jr z,+
      doBankedCallSlot1 endExpandedVwf
    +:
    
    ; save us the trouble of dealing with the load menu
    ld a,vwfTileSize_main
    ld b,vwfScrollZeroFlag_main
    ld c,vwfNametableHighMask_main
    ld hl,vwfTileBase_main
    doBankedCallSlot1 setUpVwfTileAlloc
    
    ; make up work
    ld a,($C495)
    ret
.ends

;========================================
; reset vwf on battle start
;========================================

.bank $03 slot 1
.org $0042
.section "battle start vwf reset 1" overwrite
  call newBattleInit
.ends

.bank $03 slot 1
.section "battle start vwf reset 2" free
  newBattleInit:
    doBankedCallSlot1 fullyResetVwf
    
    ; because stuff like the load game menu couldn't be assed
    ; properly closing the windows
    ld a,(vwfStateSuspended)
    or a
    jr z,+
      doBankedCallSlot1 endExpandedVwf
    +:
    
    ld a,vwfTileSize_main
    ld b,vwfScrollZeroFlag_main
    ld c,vwfNametableHighMask_main
    ld hl,vwfTileBase_main
    doBankedCallSlot1 setUpVwfTileAlloc
    
    ; make up work
    jp $4513
.ends

;========================================
; reset vwf on sega logo
;========================================

.bank $06 slot 1
.org $0025
.section "sega logo vwf reset 1" overwrite
  call newSegaLogoInit
.ends

.bank $06 slot 1
.section "sega logo vwf reset 2" free
  newSegaLogoInit:
    doBankedCallSlot1 fullyResetVwf
    
    ; because stuff like the load game menu couldn't be assed
    ; properly closing the windows
    ld a,(vwfStateSuspended)
    or a
    jr z,+
      doBankedCallSlot1 endExpandedVwf
    +:
    
    ld a,vwfTileSize_main
    ld b,vwfScrollZeroFlag_main
    ld c,vwfNametableHighMask_main
    ld hl,vwfTileBase_main
    doBankedCallSlot1 setUpVwfTileAlloc
    
    ; make up work
    jp $40C7
.ends

;========================================
; "freeze" VWF tiles that get
; temporarily covered by a window
;========================================

  ;========================================
  ; when a window background is saved,
  ; freeze all VWF tiles behind it
  ;========================================
  
  .bank $00 slot 0
  .org $2B55
  .section "freeze hidden VWF tiles 1" overwrite
    doBankedCallSlot1 freezeHiddenVwfTiles_ext
    nop
    nop
  .ends
  
  .slot 1
  .section "freeze hidden VWF tiles 2" superfree
    freezeHiddenVwfTiles_ext:
      
      push bc
      push hl
        ; if a full-screen window open is pending, do not do the freeze --
        ; the tiles have already been saved, and will be invalidated by
        ; the new content to be drawn
        ld hl,vwfSuspendWindowPending
        ld a,(hl)
        ld (hl),$00     ; clear flag
        or a
        jr nz,+
        
          ; get gg-local x/y
          ld a,($CF81)
          ld h,a
          ld a,($CF82)
          ld l,a
        
          ; check for vwf tiles
          ld a,vwfNoDeallocSentinel
          ld (vwfFullDeallocFlag),a
            ; w/h
            ld a,($CF83)
            ld b,a
            ld a,($CF84)
            ld c,a
            doBankedCallSlot1 deallocVwfTileArea
          xor a
          ld (vwfFullDeallocFlag),a
        
        +:
      pop hl
      pop bc
      
      ; make up work to derive sms-local coords in HL
      ld a,($CF81)
      ; ??? what the hell does IY have to do with anything???? ex. CEE6
      ; not used here, not a parameter to any calls!
      inc iy
      add a,$06
      ld h,a
      ld a,($CF82)
      inc iy
      add a,$03
      ld l,a
      
      ret
  .ends

  ;========================================
  ; when a window background is restored,
  ; free all VWF tiles in it and
  ; unfreeze all VWF tiles behind it
  ;========================================
  
  .bank $00 slot 0
  .org $2B1E
  .section "unfreeze unhidden VWF tiles 1" overwrite
    doBankedCallNoParamsSlot1 unfreezeUnhiddenVwfTiles_ext
    nop
    nop
  .ends
  
  .slot 1
  .section "unfreeze unhidden VWF tiles 2" superfree
    unfreezeUnhiddenVwfTiles_ext:
      ; get nametable addr (masking off write command from high byte)
      ld a,($CAD0)
      and $3F
      ld h,a
      ld a,($CACF)
      ld l,a
      ; get w/h
      ld bc,($CACB)
      
      push hl
      push de
      push bc
        ; deallocate any VWF tiles we are about to dispose of
        doBankedCallSlot1 deallocVwfTileAreaByAddr
        
        ; if window type is 09, 0B, or 0D, restore vwf state
        ; (actually, ignore 0D; it's used for the naming screen and
        ; does not have a balanced restore call associated with it)
        ld a,(windowBgTypeIndex)
        cp $09
        jr z,@setUpFullScreen
;        cp $0B
;        jr z,@setUpFullScreen
;        cp $0D
;        jr nz,@restoreNametable
        cp $0B
        jr nz,@restoreNametable
        @setUpFullScreen:
          ; blank VWF tiles
          ld hl,$0000
          ld bc,(screenVisibleW<<8)|screenVisibleH
          ld de,$1890
;          doBankedCallSlot1 deallocAndClearVwfTileArea
          doBankedCallSlot1 setTileAreaSlow
          
          doBankedCallSlot1 endExpandedVwf
          xor a
          ld (vwfSuspendWindowPending),a
      
        @restoreNametable:
        ; make up work to restore nametable
        ld a,$FF
        ld ($CACA),a
        ld a,$02
        call waitVblank
      pop bc
      pop de
      pop hl
    
      ; unfreeze revealed VWF tiles
      ld a,vwfForceAllocSentinel
      ld (vwfFullDeallocFlag),a
        doBankedCallSlot1 deallocVwfTileAreaByAddr
      xor a
      ld (vwfFullDeallocFlag),a
      
      ret
  .ends

  ;========================================
  ; save window type in
  ; restoreWindowBackground so we can
  ; check if it's one that requires VWF
  ; restoration
  ;========================================
  
  .bank $00 slot 0
  .org $2ADB
  .section "restoreWindowBackground: remember window type 1" overwrite
    doBankedCallSlot1 rememberRestoredWindowType_ext
  .ends
  
  .slot 1
  .section "restoreWindowBackground: remember window type 2" superfree
    rememberRestoredWindowType_ext:
      ; save window type
      ld (windowBgTypeIndex),a
      
      ; make up work
      push af
        ld a,$02
        call $1034
      pop af
      ld hl,$867F
      old_read16BitTable
      ld ($CF85),hl
      ret
  .ends

;========================================
; if a window is about to be opened and
; is not flagged to have the background
; it will cover saved, deallocate that
; background
;========================================
  
.bank $00 slot 0
.org $2A48
.section "openWindow: dealloc unneeded tiles 1" overwrite
  doBankedCallSlot1 deallocWindowOpenTiles_ext
.ends

.slot 1
.section "openWindow: dealloc unneeded tiles 2" superfree
  deallocWindowOpenTiles_ext:
    ; make up work to get window infoptr
    ; A = raw window ID
    push af
      and $7F
      ld hl,$867F
      old_read16BitTable
    pop af
    
    ; if high bit of window ID is unset, the window background will be
    ; saved and we don't need to do anything
    or a
    jp p,@checkFullScreen
    
      ; background will be destroyed by window open, so deallocate it
      push hl
      
        ; TEST
  ;      push bc
  ;      push de
  ;      push hl
  ;        doBankedCallSlot1 saveExpandedVwf
  ;        doBankedCallSlot1 restoreExpandedVwf
  ;      pop hl
  ;      pop de
  ;      pop bc
      
        ; read window position info
        ; w/h
        ld b,(hl)
        inc hl
        ld c,(hl)
        inc hl
        ; x/y
        ld a,(hl)
        inc hl
        ld l,(hl)
        ld h,a
        
        doBankedCallSlot1 deallocVwfTileArea
      pop hl
      jr @done
    
    @checkFullScreen:
    
    ; if window type is 09, 0B, or 0D, we are initiating a full-screen
    ; window and need to save the vwf state
    ; (0D/naming screen does not have balanced open/restore: ignore)
    
    cp $09
    jr z,@setUpFullScreen
;    cp $0B
;    jr z,@setUpFullScreen
;    cp $0D
;    jr nz,@done
    cp $0B
    jr nz,@done
    
    @setUpFullScreen:
      push hl
        doBankedCallSlot1 startExpandedVwf
        ld a,$FF
        ld (vwfSuspendWindowPending),a
      pop hl
    
    @done:
    
    ; make up work
    ld ($CF85),hl
    ld a,(hl)
    inc hl
    ld ($CF83),a
    ret
.ends

;========================================
; when certain full-screen menus are
; opened, save the entire state of the
; VWF allocation as well as all the
; tile data to expansion RAM, then
; copy it back when the menu is closed
;========================================

  ;========================================
  ; routines for copying to/from expansion
  ; ram
  ;========================================
  
  .define numBackupTilesA $72

  .slot 1
  .section "preserve expanded vwf tiles 1" superfree
    startExpandedVwf:
      ld a,$FF
      ld (vwfStateSuspended),a
      
      call saveExpandedVwf

      ; new vwf settings
      ld a,vwfTileSize_fullscreen
      ld b,vwfScrollZeroFlag_fullscreen
      ld c,vwfNametableHighMask_fullscreen
      ld hl,vwfTileBase_fullscreen
      doBankedCallSlot1 setUpVwfTileAlloc
      
      ret
    
    endExpandedVwf:
      xor a
      ld (vwfStateSuspended),a
      
      ; no need to explictly restore settings after this call -- they're
      ; already restored by the call itself
      jp restoreExpandedVwf
    
    saveExpandedVwf:
      
      ld a,(cartRamCtrl)
      push af
      or $08
;      di
        
        ; enable expansion RAM at 8000-BFFF
        ld (cartRamCtrl),a
        
        ;=====
        ; copy tile data
        ;=====
        
        ; copy tiles $B-$7C
;        ld b,vwfTileSize_main
;        ld de,expRam_tileBuffer+0
;        ld hl,vwfTileBase_main*bytesPerTile
;        rawTilesFromVdp_macro

        ; stagger copy across multiple interrupts to reduce sound issues
        .rept 3 INDEX count
          di
            ld b,$26
            ld de,expRam_tileBuffer+(count*$26*bytesPerTile)
            ld hl,(vwfTileBase_main*bytesPerTile)+((count*$26)*bytesPerTile)
            rawTilesFromVdp_macro
          ei
          nop
        .endr 
        ; original idea was to optimize by reading the allocation array
        ; and copying only tiles which are actually allocated (doing
        ; the reverse procedure for the restoration).
        ; turns out it isn't significantly faster.
        ; could be made faster with RLE, but...
/*        ld b,(maxVwfTiles&$FF)
        ld de,expRam_tileBuffer
        ld hl,vwfAllocationArray+maxVwfTiles-1
        --:
          ld a,(hl)
          or a
          jr z,+
            push hl
            push bc
              ; does not generalize!
              ; but fine for this game's limited use
              ld a,vwfTileBase_main-1
              add a,b
              ld l,a
              ld h,$00
              
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              
              ld b,$01
              di
                rawTilesFromVdp_macro
              ei
              
              ex de,hl
            pop bc
            pop hl
          +:
          dec hl
          djnz -- */
        
;        di
;          ld b,$29
;          ld de,expRam_tileBuffer+(2*$2A*bytesPerTile)
;          ld hl,(vwfTileBase_main*bytesPerTile)+((2*$2A)*bytesPerTile)
;          rawTilesFromVdp_macro
;        ei
;        nop
        
        ; copy tiles $A0-$FF
;        ld b,$100-$A0
;        ld de,expRam_tileBuffer+(vwfTileSize_main*bytesPerTile)
;        ld hl,$A0*bytesPerTile
;        rawTilesFromVdp_macro
        .rept 3 INDEX count
          di
            ld b,$20
            ld de,expRam_tileBuffer+(numBackupTilesA*bytesPerTile)+(count*$20*bytesPerTile)
            ld hl,($A0*bytesPerTile)+((count*$20)*bytesPerTile)
            rawTilesFromVdp_macro
          ei
          nop
        .endr
        
        ;=====
        ; copy alloc array
        ;=====
        
        ld bc,maxVwfTiles
        ld de,expRam_vwfAllocationArrayBackup
        ld hl,vwfAllocationArray
        ldir
        
        ;=====
        ; copy params
        ;=====
        
        ld hl,(vwfAllocationArraySize)
        ld (expRam_vwfAllocationArraySize),hl
        ld hl,(vwfAllocationArrayBaseTile)
        ld (expRam_vwfAllocationArrayBaseTile),hl
        ld hl,(assumeScrollZeroFlag)
        ld (expRam_assumeScrollZeroFlag),hl
      
      pop af
      ld (cartRamCtrl),a
;      ei
      
      ret
    
    restoreExpandedVwf:
      ld a,(cartRamCtrl)
      push af
      or $08
        
        ; enable expansion RAM at 8000-BFFF
        ld (cartRamCtrl),a
        
        ;=====
        ; copy alloc array
        ;=====
        
        ld bc,maxVwfTiles
        ld de,vwfAllocationArray
        ld hl,expRam_vwfAllocationArrayBackup
        ldir
        
        ;=====
        ; copy tile data
        ;=====
        
        ; copy tiles $B-$7C
        ; stagger copy across multiple interrupts to reduce sound issues
        .rept 3 INDEX count
          di
            ld b,$26
            ld de,expRam_tileBuffer+(count*$26*bytesPerTile)
            ld hl,$4000|((vwfTileBase_main*bytesPerTile)+((count*$26)*bytesPerTile))
            rawTilesToVdp_macro_safe
          ei
          nop
        .endr
/*        ld b,(maxVwfTiles&$FF)
        ld de,expRam_tileBuffer
        ld hl,vwfAllocationArray+maxVwfTiles-1
        --:
          ld a,(hl)
          or a
          jr z,+
            push hl
            push bc
              ; does not generalize!
              ; but fine for this game's limited use
              ld a,vwfTileBase_main-1
              add a,b
              ld l,a
              ld h,$00
              
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              set 6,h
              
              ld b,$01
              di
                rawTilesToVdp_macro_safe
              ei
              
              ex de,hl
            pop bc
            pop hl
          +:
          dec hl
          djnz -- */
        
        ; copy tiles $A0-$FF
        .rept 3 INDEX count
          di
            ld b,$20
            ld de,expRam_tileBuffer+(numBackupTilesA*bytesPerTile)+(count*$20*bytesPerTile)
            ld hl,$4000|(($A0*bytesPerTile)+((count*$20)*bytesPerTile))
            rawTilesToVdp_macro_safe
          ei
          nop
        .endr
        
        ;=====
        ; copy params
        ;=====
        
        ld hl,(expRam_vwfAllocationArraySize)
        ld (vwfAllocationArraySize),hl
        ld hl,(expRam_vwfAllocationArrayBaseTile)
        ld (vwfAllocationArrayBaseTile),hl
        ld hl,(expRam_assumeScrollZeroFlag)
        ld (assumeScrollZeroFlag),hl
      
      pop af
      ld (cartRamCtrl),a
      
      ret
  .ends

;========================================
; modify "enemy(ies) appeared" message
; to deal with the fact that each
; message is now 2 lines and must be
; waited on individually
;========================================

.bank $03 slot 1
.org $04D9
.section "enemies appeared update 1" SIZE $3A overwrite
  showEnemiesAppearedMessage:
    ; completely irrelevant, but while we're here: replace first
    ; character of each enemy name with space, which our updated
    ; name printing opcode uses as a sentinel value to detect that
    ; the enemy name should not be followed by its subslot number
    call markEnemySlotsNoNum
    
    @main:
    ld hl,$CBC2
    ld de,$CBC6
    ld bc,$0200
    ; loop
    -:
      push bc
      push de
      push hl
        ld a,(hl)
        bit 7,a
        jr nz,+
          ; get first enemy in group's slot ID
          ; (max 4 enemies per group)
          ld a,c
          add a,a
          add a,a
          ; save to reference
          ld (activeEnemyId),a
          ; get enemy quantity
          ld a,(de)
          ld ($D200),a
          xor a
          ld ($D201),a
          ld ($D202),a
          call showEnemiesAppearedMessage_ext
        +:
      pop hl
      pop de
      pop bc
      inc hl
      inc de
      inc c
      djnz -
    call waitBoxMode2
    ret 
.ends

.bank $03 slot 1
.section "enemies appeared update 2" free
  showEnemiesAppearedMessage_ext:
    ; handle enemy plurality.
    ; if only one enemy, use original message; otherwise, use
    ; special multi-enemy message.
    ; DE = pointer to quantity
    ld a,(de)
    cp $02
    jr c,@singularEnemy
    @pluralEnemies:
      doBankedCallSlot1 showPluralEnemiesAppearedMessage
      jr @wait
    @singularEnemy:
      ; print bank4 string 0x1C: "enemy appeared"
          ; always print on line 1
;      ld hl,$0109
      ld a,c
      add a,$09
      ld l,a
      ld h,$01
      ld bc,$001C
      call instaPrintBank4String
    
    @wait:
    
    ; wait for user to close box
;    call waitBoxMode2
    
    ; clear box (in case a message for another monster needs to be
    ; displayed)
;    doBankedCallSlot1 handleOp_clear2
    ret
  
  ; replace first character of each enemy name with a space (sentinel
  ; value indicating subslot num should not be added)
  markEnemySlotsNoNum:
    ld b,$08
    xor a
    -:
      push af
        ; get enemy name pointer
        ld c,$01
        call getCharSlotPointerPlusC
        
        ; replace first character with space
        ld (hl),oldSpaceIndex
      pop af
      inc a
      djnz -
    
    ret
.ends

/*.slot 1
.section "compose plural enemy string" superfree
  composePluralEnemyString:
    
    ret
.ends */

.slot 2
.section "plural enemy messages" superfree
  ordinalNumberStringTable:
    .incbin "out/script/ordinal_numbers.bin"
  pluralEnemyAppearedString:
    .incbin "out/script/enemy_appeared_plural.bin"
.ends

;========================================
; implement the process above for
; enemies that are summoned during
; battle
;========================================

; fortunately, this is much simpler, since we only display the message
; for one enemy type.
; as a minor complicating factor, we want to ignore the
; enemy's enumeration (which the original actually fails to do,
; resulting in messages like "Anubis 1 x2 appeared!")

.bank $03 slot 1
.org $1BA7
.section "enemies summoned 1" overwrite
  call doEnemiesSummonedMessage
  nop
  nop
  nop
.ends

.bank $03 slot 1
.section "enemies summoned 2" free
  doEnemiesSummonedMessage:
    doBankedJumpSlot1 doEnemiesSummonedMessage_ext
.ends

.slot 1
.section "enemies summoned 3" superfree
  doEnemiesSummonedMessage_ext:
    ; if CBD7 (whatever that is) is nonzero, we don't print this message
    ld a,($CBD7)
    or a
    ret nz
    
    ld a,$FF
    ld (instantPrintFlag),a
    
      ; activeEnemyId = target enemy subslot 1 ID
      ; D200 = enemy quantity
      
      ;=====
      ; temporarily remove enemy's enumeration
      ;=====
      
      ld a,(activeEnemyId)
      call getCharSlotPointer
      inc hl
      ld a,(hl)
      push af
      push hl
      
      ; overwrite first character of name with non-digit garbage
      xor a
      ld (hl),a
        
        ;=====
        ; now, do as before
        ;=====
          
        ; from 0xE5C1
        ld a,$82
        call openWindow
;          ld hl,$0109
;          call old_instaPrintBank4String

        ; if only one enemy, use regular message
        ld a,($D200)
        cp $01
        jr nz,@plural
        @singluar:
          ld bc,$00C2
          ld hl,$0109
          call instaPrintBank4String
;          doBankedCallSlot1 $65C6
          
          jr @messageDone
        ; otherwise, use plural message
        @plural:
          ; A = quantity, C = target line num
          ld c,$00
          doBankedCallSlot1 showPluralEnemiesAppearedMessage
          
          ; wait for input (mode2)
          call waitBoxMode2
          
        @messageDone:
        
      ;=====
      ; restore enemy's enumeration
      ;=====
      
      pop hl
      pop af
      ld (hl),a
    
    xor a
    ld (instantPrintFlag),a
    ret
.ends


;========================================
; use new enemy name printing scheme.
; 
; instead of adding enemy subslot ID to
; end of name, it goes in the first
; byte. the name in RAM is ignored, and
; we instead get the name from ROM,
; appending the number in RAM.
;========================================

  ;========================================
  ; write subslots numbers to start of
  ; names
  ;========================================

  .bank $03 slot 1
  .org $04B6
  .section "enemy enumeration update 1" SIZE $23 overwrite
    enumerateEnemySubslots:
      ld b,$08
      ld c,$00
      -:
        push bc
          ; get enemy name pointer
          ld a,c
          ld c,$00
          call getCharSlotPointerPlusC
        pop bc
        
        ; per the original game, universal will does not get enumerated.
        ; (nor would any other enemy with 7 characters in its name, but
        ; it happens to be the only one)
        ; check enemy ID
        ld a,(hl)
        ; ret if universal will
        cp $72
        ret z
        
        inc hl
        
        ; get subslot num
        ld a,c
        and $03
        
        ; replace first character with an "old" digit (this makes
        ; things easier when the game wants to remove it later)
        add a,oldEnemyDigitOffset
        ld (hl),a
        inc c
        djnz -
      ret 
  .ends

  ;========================================
  ; update printing routine to use new
  ; names + numbering system
  ;========================================
  
;========================================
; 2x text speedup
;========================================

.bank $00 slot 0
.org $1EBA
.section "printing text delay, no override 1" SIZE $20 overwrite
  push bc
  push de
  push hl
/*    ; get user's default text speed
    ld a,(textSpeedSetting)
    add a,a
    inc a
    ld b,a
    -:
      ld a,$01
      call waitVblank
      push bc
      call pollInput
      pop bc
      ; if button pressed, break loop
      ld a,(buttonsPressed)
      and $B0
      jr nz,+
        djnz -
    +: */
    doBankedCallSlot1 printDelayNoOverride_ext
  pop hl
  pop de
  pop bc
  ret 
.ends

;.define fastForwardButtonsMask $B0
; don't use button 2 (the confirm/next text box button) as fast forward
.define fastForwardButtonsMask $B0

.slot 1
.section "printing text delay, no override 2" superfree
  printDelayNoOverride_ext:
    ; check for fast-forward
    call pollInput
    ld a,(buttonsPressed)
    and fastForwardButtonsMask
    ld (textFastForwardFlag),a
  
    ; increment print skip counter
    ld hl,printSkipCounter
    inc (hl)
    
    ; decrement speech emulation counter if nonzero
    ld a,(speechEmulationCharsLeft)
    or a
    jr z,+
      dec a
      ld (speechEmulationCharsLeft),a
    +:
    
    ; decrement speech emulation delayed delay counter if nonzero
/*    ld a,(speechEmulationDelayedDelayCounter)
    or a
    jr z,+
      dec a
      ld (speechEmulationDelayedDelayCounter),a
    +: */
    
    ; check if speech emulation enabled
    ld a,(speechEmulationDisableFlag)
    or a
    jr nz,@noSpeechEmulation
    
      ; check if a speech emulation delay is pending
      ld a,(speechEmulationCharsLeft)
;      or a
;      jr z,@noSpeechEmulation
      cp $01
      jr nz,@noSpeechEmulation
        
        ; do a delay of the specified length
        @needSpeechDelay:
        ld a,(speechEmulationDelayAmount)
        jr @doDelay
    
    @noSpeechEmulation:
    
    ; if no speech emulation delay pending, and text speed is zero (fast),
    ; do double print check
;    ld a,(speechEmulationDisableFlag)
;    cp $01
;    jr nz,+
      ; get user's default text speed
      ld a,(textSpeedSetting)
      or a
      jr z,@doublePrintCheck
    
      ; if one of the skip buttons pressed, do double printing check
;      xor a
;      ld (textFastForwardFlag),a
      ld a,(textFastForwardFlag)
      or a
      jr nz,@doublePrintCheck
;    +:
    
    ; otherwise, do default printing
    jr @defaultPrinting
    
    @doublePrintCheck:
      
      ; if low bit of print skip counter set, we're done
      ld a,(printSkipCounter)
      and $01
      ret nz
    
    @defaultPrinting:
    
    ; if speed zero, wait 1 frame; otherwise, wait the specified number
    ld a,(textSpeedSetting)
    ; (original game multiplies by 2)
;    add a,a
    or a
    jr nz,+
      inc a
    +:
    
    @doDelay:
    ld b,a
    -:
      ld a,$01
      call waitVblank
      push bc
        call pollInput
      pop bc
      
      ; if button pressed, break loop
      ld a,(buttonsPressed)
      and fastForwardButtonsMask
      ld (textFastForwardFlag),a
      jr z,@continueDelayLoop
        ; flag fast-forward status
;        ld (textFastForwardFlag),a
        
        ; speech emulation delays still apply in fast-forward mode
        ld a,(speechEmulationCharsLeft)
        cp $01
        jr z,@continueDelayLoop
        
        jr @endDelayLoop
      @continueDelayLoop:
      djnz -
    @endDelayLoop:
;      jr nz,+
;        djnz -
    ret 
.ends
  
;========================================
; TODO: wall push
;========================================

/*.bank $01 slot 1
.org $0574
.section "disable wall push" overwrite
  nop
  nop
  nop
.ends */
  
;========================================
; remap limited strings
;========================================

.bank $00 slot 0
.org $205A
.section "print limited string 1" SIZE $14 overwrite
  push bc
  push de
    call printLimitedString_ext
  pop de
  pop bc
  ret
.ends

.bank $00 slot 0
.section "print limited string 2" free
  printLimitedString_ext:
    ld a,c
    push af
      call getStringHash
    pop af
    doBankedJumpSlot1 printRemappedLimitedString
.ends
  
;========================================
; print ally names to status line
;========================================

.bank $00 slot 0
.org $1429
.section "character names status line 1" SIZE $19 overwrite
  doBankedCallSlot1 doCharacterStatusLine
  ; A zero = done
  or a
  jp z,$14B8
  jp $1442
.ends

.slot 1
.section "character names status line 2" superfree
  doCharacterStatusLine:
    ; make up work
    call getCharSlotPointer
    push bc
    push hl
      ld bc,$000C
      add hl,bc
      bit 6,(hl)
    pop hl
    pop bc
    ; done if bit 6 of byte 0xC is set
    jr z,+
      xor a
      ret
    +:
    
    inc de
    call instaPrintCharNameToBuffer
    
    ; return carry to indicate success
    inc hl
    inc de
    ld a,$FF
    ret
  
  ; DE = dst
  ; HL = src slot pointer
  instaPrintCharNameToBuffer:
    
    ; set up buffer print.
    ; our existing vwf-to-memory print only does full-width tilemaps,
    ; but we need half-width, so we print full-width and downconvert
    ; afterward
    push hl
    push de
      ex de,hl
      startLocalPrint nametableBuffer 20 1 0 0
      
      ld bc,(maxNameTileLen<<8)|$01
      ld de,$0000
      doBankedCallSlot1 initVwfString
      
      ; set up nametable buffer with blanks
      ld hl,nametableBuffer
      ld b,maxNameTileLen
;      .rept maxNameTileLen
      -:
        ld a,oldSpaceIndex
        ld (hl),a
        inc hl
        ld a,$18
        ld (hl),a
        inc hl
        
        djnz -
;      .endr
    pop de
    pop hl
    
    ld a,(instantPrintFlag)
    push af
      
      ld a,$FF
      ld (instantPrintFlag),a
      
      push hl
        push de
          doBankedCallSlot1 handleAllyNamePrint_fromPointer
        pop de
        
        ; copy buffer to destination in halved format
        ld hl,nametableBuffer
        ld b,maxOrigNameTileLen
        
        ; do full copy if flag set.
        ; this is needed for the final boss.
        ld a,(copyFullCharNameFlag)
        or a
        jr z,+
          ld b,maxNameTileLen
        +:
        
        -:
          ld a,(hl)
          ld (de),a
          inc hl
          inc hl
          inc de
          djnz -
        
        endLocalPrint
      pop hl
      
      ; advance HL past (old) character name
      ld bc,$0008
      add hl,bc
    
    pop af
    ld (instantPrintFlag),a
    
    ret
.ends

;========================================
; print enemy targets to status line
;========================================

.bank $03 slot 1
.org $0CE8
.section "enemy names status line 1" SIZE $1F overwrite
  push af
    ; init print buffer
    ld hl,(printBufferPtr)
    ld bc,$0012
    ld a,$90
    rst $08
  pop af
  
  call doEnemyTargetLine
  jp $4D07
.ends

.bank $03 slot 1
.section "enemy names status line 2" free
  ; A = slot index
  doEnemyTargetLine:
    call getCharSlotPointer
    ld de,(printBufferPtr)
    ld a,$FF
    ld (copyFullCharNameFlag),a
      doBankedCallSlot1 instaPrintCharNameToBuffer
    xor a
    ld (copyFullCharNameFlag),a
    ret
.ends

; don't add たい after enemy quantity
.bank $03 slot 1
.org $0D17
.section "enemy names status line 3" overwrite
  ; replace たい with "X"
  push af
    ld (hl),$8B
  pop af
  inc hl
  ld (hl),a
  nop
.ends

;========================================
; print affliction healing targets to
; status line
;========================================

.bank $01 slot 1
.org $2F3F
.section "names healing menu 1" overwrite
  call doHealingMenuLine
  jp $6F48
.ends

.bank $01 slot 1
.section "names healing menu 2" free
  ; DE = char slot pointer + 1
  doHealingMenuLine:
    doBankedJumpSlot1 doHealingMenuLine_ext
.ends

.slot 1
.section "names healing menu 3" superfree APPENDTO "vwf and friends"
  ; DE = char slot pointer + 1
  doHealingMenuLine_ext:
    
    push de
      ; set up print location from nametable pos
      call nametableAddrToLocalCoords
      ex de,hl
      ld bc,(maxNameTileLen<<8)|$01
      call initVwfString
    pop de
    
    ; get pointer to start of character slot
    ex de,hl
    dec hl
    
    push hl
      ld a,(instantPrintFlag)
      push af
        ld a,$FF
        ld (instantPrintFlag),a
        call handleAllyNamePrint_fromPointer
      pop af
      ld (instantPrintFlag),a
    pop hl
    
    ; move past character name
    ld de,$0008
    add hl,de
    ex de,hl
    
    ret
.ends
  
;========================================
; replaces hardcoded dashes on status
; screen with "..", because it's
; convenient
;========================================

/*.define dashReplacementCharacter $9A

.bank $00 slot 0
.org $176B
.section "dash replacement 1" overwrite
  ld a,dashReplacementCharacter
.ends

.bank $00 slot 0
.org $179B
.section "dash replacement 2" overwrite
  ld a,dashReplacementCharacter
.ends */
  
;========================================
; increase width of level up stat
; flasher
;========================================

.bank $03 slot 1
.org $3AFB
.section "level up flasher width" overwrite
  ld a,$05+1
.ends
  
;========================================
; name entry
;========================================
  
  ;========================================
  ; reset vwf before running name entry
  ;========================================
  
  ; reset vwf before first scrolling text
  .bank $06 slot 1
  .org $0076
  .section "name entry vwf reset 1" overwrite
    call scroll1VwfReset
  .ends
  
  ; reset vwef before each name entry screen
  .bank $06 slot 1
  .org $008C
  .section "name entry vwf reset 2" overwrite
    call nameEntryVwfReset
  .ends
  
  .bank $06 slot 1
  .section "name entry vwf reset 3" free
    bank6VwfReset:
      doBankedJumpSlot1 fullyResetVwf
    
    scroll1VwfReset:
      push af
        call bank6VwfReset
      pop af
      jp $4FB1
      
    nameEntryVwfReset:
      push af
        call bank6VwfReset
      pop af
      jp $42F5
  .ends
  
  ;========================================
  ; check for new "back" and "done"
  ; positions
  ;========================================
  
  .define nameEntryNewDeleteXPos 6
  .define nameEntryNewDeleteYPos 7
  .define nameEntryNewEndXPos 6
  .define nameEntryNewEndYPos 8
  
  .bank $06 slot 1
  .org $0510
  .section "name entry done check 1" overwrite
    cp nameEntryNewEndYPos
  .ends
  
  .bank $06 slot 1
  .org $0518
  .section "name entry done check 2" overwrite
    cp nameEntryNewEndXPos
  .ends
  
  .bank $06 slot 1
  .org $0522
  .section "name entry back check 1" overwrite
    cp nameEntryNewDeleteYPos
  .ends
  
  .bank $06 slot 1
  .org $052A
  .section "name entry back check 2" overwrite
    cp nameEntryNewDeleteXPos
  .ends
  
  ;========================================
  ; new cursor movement logic
  ;========================================
  
  .bank $06 slot 1
  .org $0619
  .section "name entry cursor move 1" overwrite
    jp newNameEntryCursorMoveCheck
  .ends
  
  .bank $06 slot 1
  .section "name entry cursor move 2a" free
    newNameEntryCursorMoveCheck:
    doBankedJumpSlot1 newNameEntryCursorMoveCheck_ext
  .ends
  
  .slot 1
  .section "name entry cursor move 2b" superfree
    newNameEntryCursorMoveCheck_ext:
      ; new layout:
      ; everything in the box from (0,0) to (8,5) is valid
      ; delete = (6,7)
      ; end = (6,8)
      ; the rest is unselectable
      
      ld a,e
      or a
      jr z,@noYChange
        ld a,(nameEntryY)
        cp 6
        jr nz,+
/*          ; if y == 6, and user moved up, then we previously had delete selected
          ld a,e
          cp $FF
          jr nz,++
            ; skip empty row 6
            ld a,5
            ld (nameEntryY),a
            jr @noYChange
          ++: */
          ; double move to skip empty row 6
          add a,e
          ld (nameEntryY),a
        +:
      @noYChange:
      
      ; if Y == -1
      ld a,(nameEntryY)
      cp $FF
      jr nz,@yNotFF
        ; if X <= 5, Y becomes 5
        ld a,(nameEntryX)
        cp 6
        jr nc,+
          ld a,5
          ld (nameEntryY),a
          jr @yChecksDone
        ; if X >= 6, X becomes 6 and Y becomes 8
        +:
          ld a,6
          ld (nameEntryX),a
          ld a,8
          ld (nameEntryY),a
          jr @yChecksDone
      @yNotFF:
      
      ; if Y >= 7
      ld a,(nameEntryY)
      cp 7
      jr c,@yBelow7
        ; if X <= 5, and move was downward, Y becomes 0
        ld a,(nameEntryX)
        cp 6
        jr nc,+
          ld a,$01
          cp e
          jr nz,+
            xor a
            ld (nameEntryY),a
            jr @yBelow7
        ; if X >= 6, X becomes 6
        +:
          ld a,6
          ld (nameEntryX),a
          ; if Y >= 9, Y becomes 0
          ld a,(nameEntryY)
          cp 9
          jr c,+
            xor a
            ld (nameEntryY),a
          +:
      @yBelow7:
      
      @yChecksDone:
      
      ; if X == -1
      ld a,(nameEntryX)
      cp $FF
      jr nz,@xNotFF
        ; if Y <= 5, X becomes 8
        ld a,(nameEntryY)
        cp 6
        jr nc,+
          ld a,8
          ld (nameEntryX),a
          jr @xNotFF
        ; if Y >= 6, X becomes 6
        +:
          ld a,6
          ld (nameEntryX),a
      @xNotFF:
      
      ; if X >= 9
      ld a,(nameEntryX)
      cp 9
      jr c,@xBelow9
        ; if Y <= 5, X becomes 0
        ld a,(nameEntryY)
        cp 6
        jr nc,+
          xor a
          ld (nameEntryX),a
          jr @xBelow9
        ; if Y >= 6, X becomes 6
        +:
          ld a,6
          ld (nameEntryX),a
      @xBelow9:
      
      ret
  .ends
  
  ;========================================
  ; use new index table
  ;========================================
  
  .bank $06 slot 1
  .org $0598
  .section "name entry index table 1" overwrite
    call useNewNameEntryIndexTable
  .ends

  .bank $06 slot 1
  .section "name entry index table 2" free
    newNameEntryIndexTable:
      .incbin "out/script/nameentry_table.bin"
    
    useNewNameEntryIndexTable:
      ; A = old index
      ; B = old index (must be preserved)
      ; HL = target pos in name buffer
      
      push bc
      push de
        ld a,(nameEntryY)
        
        ; multiply Y by 9
        ld b,a
        add a,a
        add a,a
        add a,a
        add a,b
        
        ; add X
        ld e,a
        ld a,(nameEntryX)
        add a,e
        ; add to table pointer
        ld e,a
        ld d,$00
        push hl
          ld hl,newNameEntryIndexTable
          add hl,de
          ; read target index
          ld a,(hl)
        pop hl
        
        ; write value to name to name
        ld (hl),a
      pop de
      pop bc
      
      ; make up work
      jp $45CA
  .ends
  
/*  ;========================================
  ; disable katakana page switch
  ;========================================
  
  .bank $06 slot 1
  .org $04BB
  .section "name entry no page switch" overwrite
    jp $44C0
  .ends */
  
  ;========================================
  ; pressing start button sets cursor
  ; position to "done"
  ;========================================
  
  .bank $06 slot 1
  .org $04BD
  .section "name entry start button 1" overwrite
    call nz,nameEntryStartButtonHandler
  .ends
  
  .bank $06 slot 1
  .section "name entry start button 2" free
    nameEntryStartButtonHandler:
      ; erase current cursor
      call $4762
      ; set new position
      ld a,nameEntryNewEndXPos
      ld (nameEntryX),a
      ld a,nameEntryNewEndYPos
      ld (nameEntryY),a
      ; redraw
      ; (not necessary, this gets called immediately after we return)
;      jp $472C
      ret
  .ends
  
  
  
  ;========================================
  ; use new default names
  ;========================================
  
  .bank $06 slot 1
  .org $0326
  .section "name entry default names 1" overwrite
    jp useNewDefaultName
  .ends
  
  .bank $06 slot 1
  .section "name entry default names 2" free
    defaultNamePtrTable:
      .dw defaultNameChar0
      .dw defaultNameChar1
      .dw defaultNameChar2
    defaultNameChar0: .incbin "out/script/charnames_default_char0.bin"
    defaultNameChar1: .incbin "out/script/charnames_default_char1.bin"
    defaultNameChar2: .incbin "out/script/charnames_default_char2.bin"
    
    useNewDefaultName:
      ld hl,defaultNamePtrTable
      ld a,(nameEntryCurPartyMemId)
      old_read16BitTable
      
      ; copy name to RAM
      ld b,(hl)
      inc hl
      push bc
        ld c,b
        ld b,$00
        ld de,nameEntryBuffer
        ldir
      pop bc
      
      ; save current nametable pos
      ld de,(nameEntryX)
      push de
        
        ; read series of x/y pairs indicating position in nametable of
        ; characters in name, so we can display the new name visually
        -:
          ; set x/y pos
          ld a,(hl)
          inc hl
          ld (nameEntryX),a
          ld a,(hl)
          inc hl
          ld (nameEntryY),a
          
          push bc
          push hl
            ; read the index at that position
            call $456B
            ; add character (also updates name length)
            call $4582
          pop hl
          pop bc
          
          djnz -
      
      pop de
      ld (nameEntryX),de
      
      scf
      ccf
      ret
  .ends
  
  ;========================================
  ; use new cheat names
  ;========================================
  
  .bank $06 slot 1
  .org $0194
  .section "name entry cheat names" overwrite
    .incbin "out/script/charnames_cheat_char0.bin"
    .incbin "out/script/charnames_cheat_char1.bin"
    .incbin "out/script/charnames_cheat_char2.bin"
    .incbin "out/script/charnames_cheat_soundtest.bin"
  .ends
  
;========================================
; text scrolling
;========================================
  
  ;========================================
  ; data
  ;========================================
  
  .slot 2
  .section "text scrolling resources 1" superfree
    textScroll1Grp:
      .incbin "out/script/scroll1/grp.bin" FSIZE textScroll1GrpSize
      .define numTextScroll1GrpTiles textScroll1GrpSize/bytesPerTile
    
    textScroll1Tilemaps:
      .incbin "out/script/scroll1/tilemaps.bin"
  .ends
  
  .slot 2
  .section "text scrolling resources 2" superfree
    textScroll2Grp:
      .incbin "out/script/scroll2/grp.bin" FSIZE textScroll2GrpSize
      .define numTextScroll2GrpTiles textScroll2GrpSize/bytesPerTile
    
    textScroll2Tilemaps:
      .incbin "out/script/scroll2/tilemaps.bin"
  .ends
  
  .slot 2
  .section "text scrolling resources 3" superfree
    creditsGrp:
      .incbin "out/script/credits/grp.bin" FSIZE creditsGrpSize
      .define numCreditsGrpTiles creditsGrpSize/bytesPerTile
    
    creditsTilemaps:
      .incbin "out/script/credits/tilemaps.bin"
  .ends
  
  ;========================================
  ; init
  ;========================================
  
  .bank $06 slot 1
  .org $0907
  .section "text scrolling init 1" overwrite
    jp initTextScroll
;    jp $48EB
  .ends
  
  .define oldScroll1TextPtr $7628
  .define oldScroll2TextPtr $736A
  .define oldCreditsTextPtr $771D
  
  .define baseScrollGrpTile $B
  .define scrollGrpBannedRangeStart $7B
  .define scrollGrpBannedRangeEnd $F0
  
  .macro initTextScrollParams ARGS grpPtr numGrpTiles tilemapPtr
    ld a,:grpPtr
    ld (scrollSourceBank),a
    
    ld hl,grpPtr
    ld (scrollGrpTilesPointer),hl
    
    ld hl,numGrpTiles
    ld (scrollGrpTilesLeft),hl
    
    ld hl,tilemapPtr
    ld (scrollTilemapPointer),hl
    
;    ld hl,baseScrollGrpTile
;    ld (scrollGrpCurDstTile),hl
  .endm
  
  .bank $06 slot 1
  .section "text scrolling init 2" free
    initTextScroll:
/*      ; make up work to fetch "type"
      ld a,(iy+$00)
      inc iy
      
      ; fetch old text pointer
      ld l,(iy+$00)
      inc iy
      ld h,(iy+$00)
      inc iy */
      
      ; HL = text pointer
      
      ; make up work
      ld ($CB79),hl
      
      ; check which text scroll this is.
      ; to make our life easier, only check the top byte, since those
      ; happen to be distinct for the three input cases.
      ld a,h
      cp >oldScroll1TextPtr
      jr nz,+
        ; scroll 1
        initTextScrollParams textScroll1Grp numTextScroll1GrpTiles textScroll1Tilemaps
        jr @remap
      +:
      cp >oldScroll2TextPtr
      jr nz,+
        ; scroll 2
        initTextScrollParams textScroll2Grp numTextScroll2GrpTiles textScroll2Tilemaps
        jr @remap
      +:
      cp >oldCreditsTextPtr
      jr nz,+
        ; credits
        initTextScrollParams creditsGrp numCreditsGrpTiles creditsTilemaps
        jr @remap
      +:
      
        ; nothing we care about
        ret
      
      @remap:
        
      ld hl,baseScrollGrpTile
      ld (scrollGrpCurDstTile),hl
      xor a
      ld (scrollTilemapsLeft),a
      
      ret
  .ends
  
  ;========================================
  ; update
  ;========================================
  
  .bank $06 slot 1
  .org $0911
  .section "text scrolling update 1" overwrite
    jp updateTextScroll_grp
  .ends
  
  .bank $06 slot 1
  .org $0967
  .section "text scrolling update 2" overwrite
    jp updateTextScroll_tilemap
  .ends
  
  .define maxScrollTilesLoadedPerUpdate $20
  
  .bank $06 slot 1
  .section "text scrolling update 3" free
    updateTextScroll_grp:
      doBankedCallSlot1 updateTextScroll_grp_ext
      
      ; make up work if grp load done
      or a
      ret nz
      ld a,($CB6E)
      jp $4914
      
    updateTextScroll_tilemap:
      ; make up work
      ld ix,($CB79)
      
      ; if dst tile zero, assume this is some case we somehow missed and
      ; fall back to the regular behavior
;      ld hl,(scrollGrpCurDstTile)
;      ld a,h
;      or l
;      jp z,$496B
      ; do nothing while no tilemaps queued
      ld a,(scrollTilemapsLeft)
      or a
      jp z,$496B
      
      ; clear next line
      ld hl,$0615
      call old_localCoordsToNametableAddr
      set 6,h
      ld (old_curPrintNametableAddr),hl
      ld b,$15
      -:
        ld d,$08
        ld e,$90
        ; call 2119 = printTile
        call old_printTile
        djnz -
      
      ;=====
      ; copy in new tilemap
      ;=====
      
      ld a,(scrollSourceBank)
      ld b,a
      openTempBankSlot2_B
      
        ; get centering offset
        ld hl,(scrollTilemapPointer)
        ; tile count
        ld b,(hl)
        inc hl
        ; centering offset
        ld a,(hl)
        inc hl
        
        ; set write position
        push hl
          ; add centering offset to base x/y pos
          ld hl,$0615
          add a,h
          ld h,a
          ; set as printing position
          call old_localCoordsToNametableAddr
          set 6,h
          ld (old_curPrintNametableAddr),hl
        pop ix
        
        ; skip line if tile count zero
        ld a,b
        or a
        jr z,@copyDone
          
          ; print new tiles
          -:
            ld e,(ix+0)
            inc ix
            ld d,(ix+0)
            inc ix
            call old_printTile
            djnz -
        
        @copyDone:
        
        ; save tilemap position
        ld (scrollTilemapPointer),ix
        
      closeTempBankSlot2
      
      ;=====
      ; decrement tilemaps remaining counter
      ;=====
      
      ld a,(scrollTilemapsLeft)
      dec a
      ld (scrollTilemapsLeft),a
      jr nz,+
        ; if all tilemaps transferred, flag as done
        ld a,($CB6E)
        res 4,a
        ld ($CB6E),a
      +:
      
      ; make up work (sort of)
;      jp $4993
      pop ix
      ret
  .ends
  
  .slot 1
  .section "text scrolling update 4" superfree
    updateTextScroll_grp_ext:
      ; if tiles already loaded, done
      ld hl,(scrollGrpTilesLeft)
      ld a,h
      or l
      jp z,@grpDone
    
        ; load graphics
        ld a,(scrollSourceBank)
        ld b,a
        openTempBankSlot2_B
          ; continue until we run out of tiles or reach the update limit
          
          ld bc,maxScrollTilesLoadedPerUpdate
          ld hl,(scrollGrpTilesLeft)
          or a
          sbc hl,bc
          ld b,c
          jr nc,+
            ; if tiles left < update limit, use number of tiles left
            ld a,(scrollGrpTilesLeft)
            ld b,a
          +:
          
          ; FUN FACT: wla-dx does not scope +/- labels within macros!
          ; rawTilesToVdp_macro uses a - for its loop.
          ; if we used a - label here, the eventual djnz would instead bind
          ; to the macro's -.
          --:
            push bc
              ; B = count
              ; DE = src
              ; HL = dstcmd
              
              ld b,1
              ld de,(scrollGrpTilesPointer)
              
              ; convert dst tilenum to write command
              ld hl,(scrollGrpCurDstTile)
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              add hl,hl
              set 6,h
              
              di
                rawTilesToVdp_macro
              ei
              
              ; save updated srcptr
              ld (scrollGrpTilesPointer),hl
            
              ; decrement remaining counter
              ld hl,(scrollGrpTilesLeft)
              dec hl
              ld (scrollGrpTilesLeft),hl
              
              ; move to next tile in sequence
              ld hl,(scrollGrpCurDstTile)
              inc hl
              ld (scrollGrpCurDstTile),hl
              
              ; if we hit the start of the banned range, move to end
              ld bc,scrollGrpBannedRangeStart
              or a
              sbc hl,bc
              jr nz,+
                ld hl,scrollGrpBannedRangeEnd
                ld (scrollGrpCurDstTile),hl
              +:
            
            pop bc
            
;            djnz --
            dec b
            jp nz,--
        
          ; check if done
          ld hl,(scrollGrpTilesLeft)
          ld a,h
          or l
          jr nz,+
            ; queue tilemaps
            ld hl,(scrollTilemapPointer)
            ld a,(hl)
            inc hl
            ld (scrollTilemapsLeft),a
            ld (scrollTilemapPointer),hl
          +:
          
        closeTempBankSlot2
        
        ; prevent normal scroll/text update from occurring while graphics
        ; are not fully loaded
        ld a,$FF
        ret
    
      @grpDone:
      xor a
      ret
  
  .ends
  
;========================================
; use new "the end" text
;========================================

  .define scriptOp_call $0B
  
  .bank $06 slot 1
  .org $1AC0
  .section "the end 1" overwrite
    ; update the script that handles the original tilemap transfer.
    ; the scripting system fortuitously includes a command to jump to
    ; an arbitrary routine (op 0B), which we use to run our new
    ; printing logic and then skip the part of the script we don't
    ; care about.
    .db scriptOp_call
    .dw printTheEnd
  .ends
  
;  .define theEndCharDelay $1E
  .define theEndCharDelay $1E/3
  .define theEndBasePos $0709
  
  .bank $06 slot 1
  .section "the end 2" free
    theEndText:
      .incbin "out/script/theend.bin"
      
    printTheEnd:
      ; reset script pointer to opcode that triggers this
      ld iy,$5AC0
    
      ; if delay set, decrement and wait
      ld hl,specialPrintWaitCounter
      ld a,(hl)
      or a
      jr z,+
        dec (hl)
        ret
      +:
      
      ; reset delay
      ld a,theEndCharDelay
      ld (hl),a
      
      ; if text pointer not set, initialize printing
      ld hl,(specialPrintPointer)
      ld a,h
      or l
      jr nz,+
        ld bc,$0701
        ld de,theEndBasePos
        doBankedCallSlot1 initVwfString
        ld hl,theEndText
        ld (specialPrintPointer),hl
      +:
      
      ; fetch next character
      ld a,(hl)
      inc hl
      ld (specialPrintPointer),hl
      ; if terminator, done
      cp terminatorIndex
      jr z,@theEndFinished
      
        ; print character
        ld c,a
        doBankedCallSlot1 printVwfChar
      
        ; not done: leave IY untouched, so this routine runs again next
        ; update
        ret
      
      @theEndFinished:
      ld hl,$0000
      ld (specialPrintPointer),hl
      
      ; set IY (script pointer) to the next command we care about,
      ; which turns off scene skipping and waits for the "but..." message
      ld iy,$5AD8
      
      ; HACK: force speech emulation on. if the player actually waits
      ; out the full five minutes and the game "resets" by itself,
      ; it doesn't actually perform a full reset, so the speech emulation
      ; disable flag will be carried over from the user's set preference,
      ; which could cause it to be disabled if the player starts a new
      ; game at the point.
      xor a
      ld (speechEmulationDisableFlag),a
      
      ret
  .ends
  
;========================================
; intro
;========================================
  
  .bank $06 slot 1
  .org $09ED
  .section "intro 1" overwrite
    call introPrint_ext
  .ends
  
  .bank $06 slot 1
  .section "intro 2" free
    introPrint_ext:
      ; HL = old string pointer
      
      ; remap string
      call getStringHash
      
      ld a,c
      ld (specialPrintBank),a
      
      ; get pointer in IX
      push hl
      pop ix
      
      ; init printing
      ld bc,dialogueBoxMode1Size
      ld de,dialogueBoxMode1ContentPos
      doBankedCallSlot1 initVwfString
      
      ret
      
    introCharPrintUpdate:
      ld c,a
      doBankedCallSlot1 printVwfChar
      ret
      
    introOpUpdate:
      ; to hell with tables
      
      ; linebreak
      cp $F2
      jr nz,+
        push af
          doBankedCallSlot1 handleOp_br
        pop af
        jr @origOp
      +:
      ; clear box
      cp $F9
      jr nz,+
        doBankedJumpSlot1 handleOp_clear
      +:
      
      ; for anything else, use original behavior
      @origOp:
      sub $F0
      ld hl,$4A8E
      jp old_useJumpTable
    
    finishIntroString:
      ; deallocate visible text (but don't clear; it should remain visible)
      ld bc,dialogueBoxMode1Size
      ld hl,(printBaseXY)
      doBankedCallSlot1 deallocVwfTileArea
      ret
  .ends
  
  ; use proper character printing
/*  .bank $06 slot 1
  .org $0A72
  .section "intro 3" SIZE $14 overwrite
    ; we're overwriting the diacritic code here
    ld c,a
    doBankedCallSlot1 printVwfChar
    jp $4A86
  .ends */
  
  .bank $06 slot 1
  .org $0A5A
  .section "intro 3" SIZE $33 overwrite
    ; load new bank
    ld a,(specialPrintBank)
    ld b,a
    openTempBankSlot2_B
      ; fetch next character
      ld a,(ix+$00)
      inc ix
      ; done if terminator
      cp terminatorIndex
      jr z,@stringDone
        ; op
        cp $F0
        jr c,+
;          sub $F0
;          ld hl,$4A8E
;          call old_useJumpTable
          call introOpUpdate
          jr @stringNotDone
          
        +:
        ; literal
;        call old_printChar
        call introCharPrintUpdate
        
        @stringNotDone:
        closeTempBankSlot2
        scf 
        ccf 
        ret 
        
      @stringDone:
      closeTempBankSlot2
      call finishIntroString
      scf 
      ret 
  .ends
  
  ; increase text speed
  
  .define newIntroTextSpeed $03-1
  
  .bank $06 slot 1
  .org $0A02
  .section "intro 4" overwrite
    ld a,newIntroTextSpeed
  .ends
  
  .bank $06 slot 1
  .org $0A12
  .section "intro 5" overwrite
    ld a,newIntroTextSpeed
  .ends
  
;========================================
; title screen
;========================================

  ;========================================
  ; load our new graphics in place of old
  ;========================================
  
  .bank $06 slot 1
  .org $15DD
  .section "title grp load 1" overwrite
    .db scriptOp_call
    .dw loadTitleGrp_script
  .ends
  
  .bank $06 slot 1
  .org $1689
  .section "title grp load 2" overwrite
    .db scriptOp_call
    .dw loadTitleGrp_script
  .ends
  
  .bank $06 slot 1
  .org $187F
  .section "title grp load 3" overwrite
    .db scriptOp_call
    .dw loadTitleGrp_script
  .ends

  ;========================================
  ; overwrite tilemaps
  ;========================================
  
  .bank $06 slot 1
  .org $258D
  .section "title tilemaps 1" overwrite
    .incbin "out/maps/title.bin"
  .ends
  
  .bank $06 slot 1
  .org $26F5
  .section "title tilemaps 2" overwrite
    .incbin "out/maps/title_empty.bin"
  .ends
  
  .bank $06 slot 1
  .org $285D
  .section "title tilemaps 3" overwrite
    .incbin "out/maps/title_bottom.bin"
  .ends

  ;========================================
  ; overwrite "burn" tilemaps
  ;========================================
  
/*  .bank $06 slot 1
  .org $3300
  .section "title tilemaps 4" overwrite
    .incbin "out/maps/intro_burn_0.bin"
    .incbin "out/maps/intro_burn_1.bin"
    .incbin "out/maps/intro_burn_2.bin"
    .incbin "out/maps/intro_burn_3.bin"
    .incbin "out/maps/intro_burn_4.bin"
    .incbin "out/maps/intro_burn_5.bin"
    .incbin "out/maps/intro_burn_6.bin"
    .incbin "out/maps/intro_burn_7.bin"
    .incbin "out/maps/intro_burn_8.bin"
    .incbin "out/maps/intro_burn_9.bin"
    .incbin "out/maps/intro_burn_10.bin"
    .incbin "out/maps/intro_burn_11.bin"
    .incbin "out/maps/intro_burn_12.bin"
    .incbin "out/maps/intro_burn_13.bin"
    .incbin "out/maps/intro_burn_14.bin"
    .incbin "out/maps/intro_burn_15.bin"
    .incbin "out/maps/intro_burn_16.bin"
    .incbin "out/maps/intro_burn_17.bin"
    .incbin "out/maps/intro_burn_18.bin"
    .incbin "out/maps/intro_burn_19.bin"
    .incbin "out/maps/intro_burn_20.bin"
    .incbin "out/maps/intro_burn_21.bin"
    .incbin "out/maps/intro_burn_22.bin"
    .incbin "out/maps/intro_burn_23.bin"
    .incbin "out/maps/intro_burn_24.bin"
    .incbin "out/maps/intro_burn_25.bin"
  .ends */
  
  ; we're now putting "megami tensei gaiden" on one line, so this
  ; gets more complicated.
  ; * maps 24 and 25 are disused, reducing the total count from
  ;   26 to 24.
  ; * maps 22-24 are moved from the bottom row to the top.
  .bank $06 slot 1
  .org $3300
  .section "title tilemaps 4" overwrite
    introBurnMap00: .incbin "out/maps/intro_burn_0.bin"
    introBurnMap01: .incbin "out/maps/intro_burn_1.bin"
    introBurnMap02: .incbin "out/maps/intro_burn_2.bin"
    introBurnMap03: .incbin "out/maps/intro_burn_3.bin"
    introBurnMap04: .incbin "out/maps/intro_burn_4.bin"
    introBurnMap05: .incbin "out/maps/intro_burn_5.bin"
    introBurnMap06: .incbin "out/maps/intro_burn_6.bin"
    introBurnMap07: .incbin "out/maps/intro_burn_7.bin"
    introBurnMap08: .incbin "out/maps/intro_burn_8.bin"
    introBurnMap09: .incbin "out/maps/intro_burn_9.bin"
    introBurnMap0A: .incbin "out/maps/intro_burn_10.bin"
    introBurnMap0B: .incbin "out/maps/intro_burn_11.bin"
    introBurnMap0C: .incbin "out/maps/intro_burn_12.bin"
    introBurnMap0D: .incbin "out/maps/intro_burn_13.bin"
    introBurnMap0E: .incbin "out/maps/intro_burn_14.bin"
    introBurnMap0F: .incbin "out/maps/intro_burn_15.bin"
    introBurnMap10: .incbin "out/maps/intro_burn_16.bin"
    introBurnMap11: .incbin "out/maps/intro_burn_17.bin"
    introBurnMap12: .incbin "out/maps/intro_burn_18.bin"
    introBurnMap13: .incbin "out/maps/intro_burn_19.bin"
    introBurnMap14: .incbin "out/maps/intro_burn_20.bin"
    
    introBurnMap15: .incbin "out/maps/intro_burn_21.bin"
    introBurnMap16: .incbin "out/maps/intro_burn_22.bin"
    introBurnMap17: .incbin "out/maps/intro_burn_23.bin"
    introBurnMap18: .incbin "out/maps/intro_burn_24.bin"
;    introBurnMap19: .incbin "out/maps/intro_burn_25.bin"
  .ends

  ;========================================
  ; move tilemaps 22-24 to new positions
  ;========================================
  
  .bank $06 slot 1
  .org $1EC3+($7*0)
  .section "move intro tilemaps 1" overwrite
    ; x/y
    .db $0B+6,$05
  .ends
  
  .bank $06 slot 1
  .org $1EC3+($7*1)
  .section "move intro tilemaps 2" overwrite
    ; x/y
    .db $0D+6,$05
  .ends
  
  .bank $06 slot 1
  .org $1EC3+($7*2)
  .section "move intro tilemaps 3" overwrite
    ; x/y
    .db $0F+6,$05
  .ends

  ;========================================
  ; redo burn sequence script commands
  ;========================================
  
  .define introTilemapIdentifier00 $5E27
  .define introTilemapIdentifier01 $5E2E
  .define introTilemapIdentifier02 $5E35
  .define introTilemapIdentifier03 $5E3C
  .define introTilemapIdentifier04 $5E43
  .define introTilemapIdentifier05 $5E4A
  .define introTilemapIdentifier06 $5E51
  .define introTilemapIdentifier07 $5E58
  .define introTilemapIdentifier08 $5E5F
  .define introTilemapIdentifier09 $5E66
  .define introTilemapIdentifier0A $5E6D
  .define introTilemapIdentifier0B $5E74
  .define introTilemapIdentifier0C $5E7B
  .define introTilemapIdentifier0D $5E82
  .define introTilemapIdentifier0E $5E89
  .define introTilemapIdentifier0F $5E90
  .define introTilemapIdentifier10 $5E97
  .define introTilemapIdentifier11 $5E9E
  .define introTilemapIdentifier12 $5EA5
  .define introTilemapIdentifier13 $5EAC
  .define introTilemapIdentifier14 $5EB3
  .define introTilemapIdentifier15 $5EBA
  .define introTilemapIdentifier16 $5EC1
  .define introTilemapIdentifier17 $5EC8
  .define introTilemapIdentifier18 $5ECF
  .define introTilemapIdentifier19 $5ED6
  
  .bank $06 slot 1
  .org $16B3
  .section "new intro burn sequence script" overwrite
    ;=====
    ; entry 0
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $01
      ; explosion x/y
      .db $34,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier00
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 1
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $02
      ; explosion x/y
      .db $44,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier01
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 2
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $03
      ; explosion x/y
      .db $54,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier02
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 3
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $04
      ; explosion x/y
      .db $64,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier03
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 4
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $74,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier04
      ; delay
      .db $0D,$00,$04
      
    ;==================================================
    ; splice in rearranged tilemaps
    ;==================================================
    
    ;=====
    ; entry 22
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $01
      ; explosion x/y
;      .db $7C,$5C
      .db $84,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier16
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 23
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $02
      ; explosion x/y
;      .db $8C,$5C
      .db $94,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier17
      ; delay
      .db $0D,$00,$04
    
    ;=====
    ; entry 24
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $03
      ; explosion x/y
;      .db $9C,$5C
      .db $A4,$34
      ; tilemap
      .db $09
      .dw introTilemapIdentifier18
      ; delay
      .db $0D,$00,$04
      
    ;==================================================
    ; resume normal sequence
    ;==================================================
    
    ;=====
    ; entry 5
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $04
      ; explosion x/y
      .db $3C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier05
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 6
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $4C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier06
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 7
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $01
      ; explosion x/y
      .db $5C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier07
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 8
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $02
      ; explosion x/y
      .db $6C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier08
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 9
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $03
      ; explosion x/y
      .db $7C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier09
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 10
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $04
      ; explosion x/y
      .db $8C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0A
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 11
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $9C,$44
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0B
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 12
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $01
      ; explosion x/y
      .db $3C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0C
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 13
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $02
      ; explosion x/y
      .db $4C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0D
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 14
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $03
      ; explosion x/y
      .db $5C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0E
      ; delay
      .db $0D,$00,$04
    
    ;=====
    ; entry 21
    ; (this is the bottom bit of the "t"; it was originally done
    ; as part of the now-disused last row, but that looked kind of
    ; awkward once the "last bible" katakana was removed, since
    ; the explosions suddenly jumped back to the middle of the screen
    ; and then abruptly stopped. this results in a smoother horizontal
    ; motion.)
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
;      .db $01
      .db $04
      ; explosion x/y
      .db $6C,$5C
      ; tilemap
      .db $09
      .dw introTilemapIdentifier15
      ; delay
      .db $0D,$00,$04

    ;=====
    ; entry 15
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $6C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier0F
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 16
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $01
      ; explosion x/y
      .db $7C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier10
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 17
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $02
      ; explosion x/y
      .db $8C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier11
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 18
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $03
      ; explosion x/y
      .db $9C,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier12
      ; delay
      .db $0D,$00,$04
    ;=====
    ; entry 19
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $AC,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier13
      ; delay
;      .db $0D,$00,$04
    ;=====
    ; entry 20
    ;=====
      ; ?
/*      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $BC,$54
      ; tilemap
      .db $09
      .dw introTilemapIdentifier14
      ; delay
;      .db $0D,$00,$04 */
      
    ;==================================================
    ; skip disused data
    ;==================================================
    
      ; final delay
      .db $0D,$00,$1E
      
      ; jump to routine that skips remaining disused data
      .db $0B
      .dw skipIntroBurnEnd
      
/*
    ;=====
    ; entry 25
    ;=====
      ; ?
      .db $12,$24
      ; explosion
      .db $0B
      .dw $4E07
      .db $00
      ; explosion x/y
      .db $AC,$5C
      ; tilemap
      .db $09
      .dw introTilemapIdentifier19
      ; delay
      .db $0D,$00,$1E */
  .ends
  
  .bank $06 slot 1
  .section "skip end of intro burn" free
    skipIntroBurnEnd:
      ld iy,$581F
      ret
  .ends

/*  ;========================================
  ; move lowest part of burn down a row
  ;========================================
  
  .define newTitleLowestRowY $0A+1
  
  .bank $06 slot 1
  
  .org $1EBD+(7*0)
  .section "title last row 0" overwrite
    .db newTitleLowestRowY
  .ends
  .org $1EBD+(7*1)
  .section "title last row 1" overwrite
    .db newTitleLowestRowY
  .ends
  .org $1EBD+(7*2)
  .section "title last row 2" overwrite
    .db newTitleLowestRowY
  .ends
  .org $1EBD+(7*3)
  .section "title last row 3" overwrite
    .db newTitleLowestRowY
  .ends
  .org $1EBD+(7*4)
  .section "title last row 4" overwrite
    .db newTitleLowestRowY
  .ends
  
  ; move explosions too
  
  .define newTitleLowestRowExplosionY $5C+8
  
  .org $17E0+($E*0)
  .section "title last row explosion 0" overwrite
    .db newTitleLowestRowExplosionY
  .ends
  .org $17E0+($E*1)
  .section "title last row explosion 1" overwrite
    .db newTitleLowestRowExplosionY
  .ends
  .org $17E0+($E*2)
  .section "title last row explosion 2" overwrite
    .db newTitleLowestRowExplosionY
  .ends
  .org $17E0+($E*3)
  .section "title last row explosion 3" overwrite
    .db newTitleLowestRowExplosionY
  .ends
  .org $17E0+($E*4)
  .section "title last row explosion 4" overwrite
    .db newTitleLowestRowExplosionY
  .ends */

  ;========================================
  ; new stuff
  ;========================================
  
  .bank $06 slot 1
  .section "title grp 1" free
    loadTitleGrp_script:
      openTempBankSlot2 :titleGrp
        ld b,numTitleGrpTiles
        ld de,titleGrp
        ld hl,$6000
        rawTilesToVdp_macro
      closeTempBankSlot2
      
      ret
  
  .ends
  
  .bank $06 slot 2
  .section "title grp 2" superfree
    titleGrp:
      .incbin "out/grp/title.bin" FSIZE titleGrpSize
    .define numTitleGrpTiles titleGrpSize/bytesPerTile
    
  .ends
  
;========================================
; speech emulation
;========================================

  ;========================================
  ; new system menu, including option
  ; to turn the speech emulation on/off
  ;========================================
  
  .slot 2
  .section "new system menu 1a" superfree
    newSystemWindowTilemap:
      .dw $1882,$1883,$1883,$1883,$1883,$1883,$1883,$1883,$1A82
      .rept 3
        .dw $1884,$1890,$1890,$1890,$1890,$1890,$1890,$1890,$1A84
      .endr
      .dw $1C82,$1C83,$1C83,$1C83,$1C83,$1C83,$1C83,$1C83,$1E82
    
    newSystemMenuText:
      .incbin "out/script/new_system_menu.bin"
      
    speechSettingsMenuText:
      .incbin "out/script/speech_settings_menu.bin"
      
    speechSettingsExplanationText:
      .incbin "out/script/speech_settings_explanation.bin"
      
    speechSettingsMenuLabel:
      .incbin "out/script/speech_settings_menulabel.bin"
  .ends
  
  .bank $01 slot 1
  .org $168D
  .section "save menu target" overwrite
    runSaveMenu:
      ld a,$09
  .ends
  
  .bank $01 slot 1
  .org $16A4
  .section "messages menu target" overwrite
    runMessagesMenu:
      ld a,$07
  .ends
  
  .slot 1
  .section "new system menu 1b" superfree
;    newSystemWindow:
;      ; w/y/x/y
;      .db $09,$04+1,$0B,$00
    
    .define newSystemWindowX $0B
    .define newSystemWindowY $00
    .define newSystemWindowW $09
    .define newSystemWindowH $04+1
    
    newSystemWindowParams:
      .db newSystemWindowW,newSystemWindowH,newSystemWindowX,newSystemWindowY
    
    newSystemMenuParams:
      .db $03   ; number of items
      .db $00   ; no idea, apparently never referenced
      ; item specifiers:
      ; cursor x/y, then the index to move to when up/right/down/left pressed
      .db $0C,$01,$02,$FF,$01,$FF
      .db $0C,$02,$00,$FF,$02,$FF
      .db $0C,$03,$01,$FF,$00,$FF
    
    runNewSystemWindow:
      ;=====
      ; open window
      ;=====
      
      ; save area behind window
      ld a,newSystemWindowW
      ld ($CF83),a
      ld a,newSystemWindowH
      ld ($CF84),a
      ld a,newSystemWindowX
      ld ($CF81),a
      ld a,newSystemWindowY
      ld ($CF82),a
      call saveWindowBackgroundFromParams
      
      ; transfer window tilemap
      openTempBankSlot2 :newSystemWindowTilemap
        ld bc,(newSystemWindowW<<8)|newSystemWindowH
        ld de,newSystemWindowTilemap
        ld hl,(newSystemWindowX<<8)|newSystemWindowY
        doBankedCallSlot1 writeLocalTilemapToNametable
      closeTempBankSlot2
      
      ;=====
      ; make up work
      ;=====
      
      ; "system" label on menu border
      ld hl,$0C00
      ld bc,$0034
      call instaPrintBank4String
      
      @menuLoop:
      
        ; longjump to god knows what (ROM C003)
        ; (clears middle window and displays "what will you do?" message)
        ld bc,$0002
        call $0C77
        .db $03
        .dw $4003
        
        ;=====
        ; run menu
        ;=====
        
;        ld a,$03
;        call runMenu
        
;        ld bc,(newSystemWindowW<<8)|newSystemWindowH
;        ld de,((newSystemWindowX+1)<<8)|(newSystemWindowY+1)
;        doBankedCallSlot1 initVwfString
        ld a,$FF
        ld (instantPrintFlag),a
        push ix
          openTempBankSlot2 :newSystemMenuText
            ; deallocate old text
            ; (but not "system" on top line
            ld bc,((newSystemWindowW)<<8)|(newSystemWindowH-1)
            ld hl,((newSystemWindowX)<<8)|(newSystemWindowY+1)
            doBankedCallSlot1 deallocVwfTileArea
          
            ld ix,newSystemMenuText
            ld hl,((newSystemWindowX+2)<<8)|(newSystemWindowY+1)
            doBankedCallSlot1 startNewString
          closeTempBankSlot2
        pop ix
        xor a
        ld (instantPrintFlag),a
        
;        ld a,$03
;        call runMenu

        @temp:
        ; pretend we are running menu type 0 and do hacks hacks hacks in order
        ; to get around the fact that the game was only ever designed to open
        ; windows of certain predefined sizes containing predefined contents
        xor a
        ld hl,@retPos
        push hl
        ld hl,newSystemMenuParams
        push af
        jp $3A5D
        @retPos:
        
        
        
        ;=====
        ; handle results
        ;=====
        or a
        jp m,@menuDone
          
          jr nz,+
            doBankedCallNoParamsSlot1 runSaveMenu
            jr @menuLoop
          +:
          cp $01
          jr nz,+
            doBankedCallNoParamsSlot1 runMessagesMenu
            jr @menuLoop
          +:
          cp $02
          jr nz,+
            call runSpeechSettingsMenu
            jr @menuLoop
          +:
        
      @menuDone:
      
      ;=====
      ; restore content under window
      ;=====
      
      ; push slot 2 bank onto stack because restore routine will
      ; pop it
      ; yes we have to fucking do this
      ld a,$02
      call $1034
      
      ; restore background
      ld hl,newSystemWindowParams
      ; hack our hack so the hacked window restoration code will not
      ; attempt to do extended VWF restoration (window types 9/b/d)
      xor a
      ld (windowBgTypeIndex),a
      ; have to make this up since our other hack overwrote it
      ld ($CF85),hl
      call $2AE9
      
      ret
    
    runSpeechSettingsMenu:
      ; open the old system-type window
      ld a,$06
      call openWindow
      
      ; print content
      ld a,$FF
      ld (instantPrintFlag),a
      push ix
      openTempBankSlot2 :speechSettingsExplanationText
        ; print label
        ld hl,$0C00
        ld ix,speechSettingsMenuLabel
        doBankedCallSlot1 startNewString
        
        ; clear box
        ld hl,$0109
        ld (printBaseXY),hl
        doBankedCallNoParamsSlot1 handleOp_clear2
        
        ; print explanation
        ld hl,$0109
        ld ix,speechSettingsExplanationText
        doBankedCallSlot1 startNewString
        
        ; print menu
        ld ix,speechSettingsMenuText
        ld hl,((newSystemWindowX+2)<<8)|(newSystemWindowY+1)
        doBankedCallSlot1 startNewString
      closeTempBankSlot2
      pop ix
      xor a
      ld (instantPrintFlag),a
      
      ld a,$03
      call runMenuLogic
      
      ; handle result
      or a
      jp m,+
        jr nz,++
          ; 00 = enable
          ld (speechEmulationDisableFlag),a
          jr +
        ++:
        ; other (01) = disable
        ld a,$FF
        ld (speechEmulationDisableFlag),a
      +:
      
      ld a,$06
      call restoreWindowBackground
      ret
    
  .ends
  
  .bank $01 slot 1
  .org $165A
  .section "new system menu 2" overwrite
    doBankedJumpSlot1 runNewSystemWindow
  .ends
  
;========================================
; deallocate when area cleared
;========================================

.bank $00 slot 0
.org $1197
.section "setCharArea dealloc 1" overwrite
  call setCharAreaDealloc
.ends

.bank $00 slot 0
.section "setCharArea dealloc 2" free
  setCharAreaDealloc:
    doBankedJumpSlot1 setCharAreaDealloc_ext
    
    ; do nothing if not clearing to blank
;    cp oldSpaceIndex
;    jr nz,@done
/*      push af
      push bc
      push de
        ; convert sms-local to gg-local
        ld de,(screenVisibleX<<8)|screenVisibleY
        or a
        sbc hl,de
        call deallocVwfTileArea
      pop de
      pop bc
      pop af
    @done:
    ret */
.ends

.slot 1
.section "setCharArea dealloc 3" superfree APPENDTO "vwf and friends"
  setCharAreaDealloc_ext:
    ; do nothing if not clearing to blank
;    cp oldSpaceIndex
;    jr nz,@done
      push af
      push bc
      push de
      push hl
        ; convert sms-local to gg-local
        ld de,(screenVisibleX<<8)|screenVisibleY
        or a
        sbc hl,de
        doBankedCallSlot1 deallocVwfTileArea
      pop hl
      pop de
      pop bc
      pop af
    @done:
    ; make up work
    jp old_localCoordsToNametableAddr
.ends
  
;========================================
; deallocate and clear content behind
; inventory messages (since we don't
; have enough tiles in the worst case
; to preserve the existing content, and
; the items are redrawn when the box
; is closed anyway)
;========================================

  .slot 1
  .section "inventory message dealloc 1" superfree
    deallocDropInventoryMsg_mode1:
      ld hl,$010D
      ld bc,$1204
      ld de,vwfClearTile
      doBankedJumpSlot1 deallocAndClearVwfTileArea
    
    deallocDropInventoryMsg_mode2:
      ld hl,$0109
      ld bc,$1202
      ld de,vwfClearTile
      doBankedJumpSlot1 deallocAndClearVwfTileArea
  .ends
  
  ;========================================
  ; most stuff
  ;========================================
  
  .bank $03 slot 1
  .org $25B5
  .section "inventory message dealloc std 1" overwrite
    call deallocStdInventoryMsg
  .ends

  .bank $03 slot 1
  .section "inventory message dealloc std 2" free
    deallocStdInventoryMsg:
      push af
      push bc
/*        ; if bit 7 of message ID is set (which will happen only if we
        ; changed the code ourself), then we actually don't want to
        ; deallocate.
        ; we're doing this because a few menus that use this routine
        ; don't need the tiles and don't redraw the destroyed
        ; area afterward.
        bit 7,b
        jr nz,+
          doBankedCallSlot1 deallocDropInventoryMsg_mode2
        +: */
        ; if bit 7 of message ID is set (which will happen only if we
        ; changed the code ourself), then we want to deallocate.
        ; this is safer than the other way around.
        bit 7,b
        jr z,+
          doBankedCallSlot1 deallocDropInventoryMsg_mode2
        +:
      pop bc
      pop af
      
      ; clear bit 7 of message ID
      res 7,b
      
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; discarding stuff
  ;========================================
  
  .bank $01 slot 1
  .org $1254
  .section "inventory message dealloc drop 1" overwrite
    call deallocDropInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc drop 2" free
    deallocInventoryMessageSafe_mode1:
      push af
        doBankedCallSlot1 deallocDropInventoryMsg_mode1
      pop af
      ret
    
    deallocInventoryMessageSafe_mode2:
      push af
        doBankedCallSlot1 deallocDropInventoryMsg_mode2
      pop af
      ret
      
    deallocDropInventoryMsg:
      call deallocInventoryMessageSafe_mode2
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; selling stuff
  ;========================================
  
  .bank $01 slot 1
  .org $2A17
  .section "inventory message dealloc sell 1" overwrite
    call deallocSellInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc sell 2" free
    deallocSellInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; selling unsellable stuff
  ;========================================
  
  .bank $01 slot 1
  .org $29E9
  .section "inventory message dealloc unsell 1" overwrite
    call deallocUnsellInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc unsell 2" free
    deallocUnsellInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; selling equipped stuff
  ;========================================
  
  .bank $01 slot 1
  .org $2A02
  .section "inventory message dealloc sell equipped 1" overwrite
    call deallocSellEquippedInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc sell equipped 2" free
    deallocSellEquippedInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; depositing item
  ;========================================
  
  .bank $01 slot 1
  .org $3303
  .section "inventory message dealloc deposit 1" overwrite
    call deallocDepositInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc deposit 2" free
    deallocDepositInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
  ;========================================
  ; depositing equipped item
  ;========================================
  
  .bank $01 slot 1
  .org $32B1
  .section "inventory message dealloc deposit equipped 1" overwrite
    call deallocDepositEquippedInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc deposit equipped 2" free
    deallocDepositEquippedInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
  ; note: the original game does not redraw the item list if you
  ; choose "no" at the prompt for depositing an equipped item,
  ; which causes corruption under our new system, so we now
  ; need to redraw those items too.
  
  .bank $01 slot 1
  .org $3286
  .section "inventory message redraw deposit equipped 1" overwrite
    ; used only as jr target
    depositInventoryRedrawStart:
      ld hl,$0706
  .ends
  
  .bank $01 slot 1
  .org $32C7
  .section "inventory message redraw deposit equipped 2" overwrite
    jr nz,depositInventoryRedrawStart
  .ends
  
  ;========================================
  ; withdrawing item
  ;========================================
  
  .bank $01 slot 1
  .org $3479
  .section "inventory message dealloc withdraw 1" overwrite
    call deallocWithdrawInventoryMsg
  .ends

  .bank $01 slot 1
  .section "inventory message dealloc withdraw 2" free
    deallocWithdrawInventoryMsg:
      call deallocInventoryMessageSafe_mode1
      ; make up work
      jp openWindow
  .ends
  
;========================================
; flag certain messages as deallocate
;========================================
  
/*  ;========================================
  ; fusing when level too low
  ;========================================
  
  .bank $01 slot 1
  .org $1904
  .section "nodealloc fusing level too low 1" overwrite
    ld bc,$00A5|$8000
  .ends
  
  ;========================================
  ; unfusable
  ;========================================
  
  .bank $01 slot 1
  .org $1923
  .section "nodealloc unfusable 1" overwrite
    ld bc,$00A6|$8000
  .ends
  
  ;========================================
  ; fusing already-fused monsters
  ;========================================
  
  .bank $01 slot 1
  .org $1937
  .section "nodealloc fusing existing 1" overwrite
    ld bc,$00A7|$8000
  .ends */
  
  ;========================================
  ; "The [item] is equipped and can't be discarded"
  ;========================================
  
  .bank $01 slot 1
  .org $122D
  .section "dealloc can't discard because equipped 1" overwrite
    ld bc,$0007|$8000
  .ends
  
  ;========================================
  ; "The [item] can't be discarded"
  ;========================================
  
  .bank $01 slot 1
  .org $1247
  .section "dealloc can't discard 1" overwrite
    ld bc,$0008|$8000
  .ends
  
  ;========================================
  ; "The [item] can't be discarded"
  ;========================================
  
  .bank $01 slot 1
  .org $126F
  .section "dealloc dicarded 1" overwrite
    ld bc,$0009|$8000
  .ends
  
  ;========================================
  ; "That item can't be equipped"
  ;========================================
  
  .bank $01 slot 1
  .org $1046
  .section "dealloc can't equip 1" overwrite
    ld bc,$0015|$8000
  .ends
  
  ;========================================
  ; "[enemy] already has the [item] equipped"
  ;========================================
  
  .bank $01 slot 1
  .org $10B3
  .section "dealloc already equipped 1" overwrite
    ld bc,$0016|$8000
  .ends
  
  ;========================================
  ; "The [item] was equipped"
  ;========================================
  
  .bank $01 slot 1
  .org $10F6
  .section "dealloc equipped 1" overwrite
    ld bc,$0017|$8000
  .ends
  
  ;========================================
  ; "The [item] was removed"
  ;========================================
  
  .bank $01 slot 1
  .org $106F
  .section "dealloc removed 1" overwrite
    ld bc,$0018|$8000
  .ends
  
  ;========================================
  ; "[ally] is unable to equip the [item]"
  ;========================================
  
  .bank $01 slot 1
  .org $1088
  .section "dealloc incompatible 1" overwrite
    ld bc,$0019|$8000
  .ends
  
  ;========================================
  ; "I can't accept this"
  ;========================================
  
/*  .bank $01 slot 1
  .org $29EC
  .section "dealloc can't sell 1" overwrite
    ld bc,$0056|$8000
  .ends
  
  ;========================================
  ; "I can't take the [item] if you've got it equipped"
  ;========================================
   */
  
  ;========================================
  ; "Attack became [num]"
  ;========================================
  
  .bank $01 slot 1
  .org $113C
  .section "dealloc new attack 1" overwrite
    ld bc,$009E|$8000
  .ends
  
  ;========================================
  ; "Defense became [money]"
  ;========================================
  
  .bank $01 slot 1
  .org $1141
  .section "dealloc new defense 1" overwrite
    ld bc,$009F|$8000
  .ends
  
  
;========================================
; reset vwf after fusion
;========================================

/*.bank $01 slot 1
.org $0F16
.section "deallocate after fusion 1" overwrite
  jp deallocateAfterFusion
.ends

.bank $01 slot 1
.section "deallocate after fusion 1" free
  deallocateAfterFusion:
    ; check if fused
    or a
    jp z,$4EFB
    
    ; reset VWF (we'll be kicked directly to the field menu)
    
    jp $4F19
.ends */

.bank $01 slot 1
.org $03B1
.section "deallocate after fusion 1" overwrite
  jp deallocateAfterFusion
.ends

.bank $01 slot 1
.section "deallocate after fusion 2" free
  deallocateAfterFusion:
    ; unload VWF if pending
    ld a,(vwfStateSuspended)
    or a
    jr z,+
      doBankedCallSlot1 endExpandedVwf
    +:
    
    jp $4000
.ends
  
;========================================
; bank deallocation stuff
;========================================
  
  ;========================================
  ; deallocate digits in initial VWF-
  ; printed bank quantity
  ;========================================

  .bank $01 slot 1
  .org $356E
  .section "dealloc bank digits 1" overwrite
    call deallocBankDigits
  .ends

  .bank $01 slot 1
  .section "dealloc bank digits 2" free
    deallocBankDigits:
      push bc
      push de
        ld hl,$0D08
        ld bc,$0301
        doBankedCallSlot1 deallocVwfTileArea
      pop bc
      pop de
      ; make up work
      ld hl,$C47B
      ret
  .ends
  
  ;========================================
  ; deallocate digits in initial VWF-
  ; printed bank quantity
  ;========================================

  .bank $01 slot 1
  .org $34C0
  .section "dealloc bank player money label 1" overwrite
;    ld hl,$1105
;    ld bc,$0801
    ld hl,$0705
    ld bc,$1201
    ld a,$90
    call old_setCharArea
    ld bc,$003B
    ld hl,$0102
    call instaPrintBank4String
  .ends

  .bank $01 slot 1
  .org $34E5
  .section "dealloc bank deposited money label 1" overwrite
;    ld hl,$1108
;    ld bc,$0801
    ld hl,$0708
    ld bc,$1201
    ld a,$90
    call old_setCharArea
    ld bc,$00FB
    ld hl,$0105
    call instaPrintBank4String
  .ends
  
;========================================
; deallocate character status labels
; before overwriting them
;========================================

.bank $00 slot 0
.org $135E
.section "dealloc character status names 1" overwrite
  doBankedCallSlot1 deallocCharacterStatusNames
.ends

.slot 1
.section "dealloc character status names 2" superfree APPENDTO "vwf and friends"
  deallocCharacterStatusNames:
    ; make up work
    ld a,b
    add a,$0F
    ld l,a
    ; X = 6
    ld h,$06
    call old_localCoordsToNametableAddr
    set 6,h
    ld (old_curPrintNametableAddr),hl
    
    ; deallocate line
;    ld bc,$1401
    ; save a little time by checking only the first 9 tiles, since all we care
    ; about is the name (which will be no more than 7 tiles
    ld bc,$0901
    jp deallocVwfTileAreaByAddr
.ends
  
;========================================
; move "beast's possessions" label
; left so it can be centered
;========================================

.bank $01 slot 1
.org $2A9C
.section "move beast's possessions label 1" overwrite
  ld hl,$0401
.ends
  
;========================================
; lazy hack: solion's recruitment
; message, unlike the proper fusion
; messages, closes the box after it's
; done. this would be fine except that
; the hexagram sprites from the fusion
; cutscene remain on screen (with a
; blacked-out palette), and some
; of them are in the area we're
; using for extra VWF tiles, meaning
; garbage may get displayed.
; to get around this, we make the
; window non-closing.
; (probably also applies to the
; fully-recovered dragon/amon/qilin
; recruitment too)
;========================================

/*.bank $00 slot 0
.org $2750
.section "solion recruitment hack 1" overwrite
  ld a,$00|$80  ; window ID = non-saving $00
.ends

.bank $00 slot 0
.org $275E
.section "solion recruitment hack 2" overwrite
  ; skip window restoration
  jp $2763
.ends */
  
;========================================
; just get rid of the stupid fucking
; pentagram once it's done
;========================================

.bank $01 slot 1
.org $2275
.section "pentagram removal 1" overwrite
  jp fusionEndScrollDoneCheck
.ends

.bank $01 slot 1
.section "pentagram removal 2" free
  fusionEndScrollDoneCheck:
    cp $88
    jp c,$6278
    
    ; if done, hide pentagram sprites
    push af
    push bc
    push hl
      ld a,$E0
      ld hl,$C300
      ld b,$20
      -:
        ld (hl),a
        inc hl
        djnz -
    pop hl
    pop bc
    pop af
    
    ret
.ends
  
;========================================
; shift ending "but..." left a tile
; so it can be centered
;========================================

.bank $06 slot 1
.org $1384
.section "end center 1" overwrite
  ld hl,$080F
.ends
  
;========================================
; shift title screen options left so
; they can be centered
;========================================

; visual layout
.bank $00 slot 0
.org $3BFE
.section "title menu shift 1" overwrite
  .db $05-2
.ends

; cursor 1 x
.bank $00 slot 0
.org $3DA0
.section "title menu shift 2" overwrite
  .db $05-2
.ends

; cursor 2 x
.bank $00 slot 0
.org $3DA6
.section "title menu shift 3" overwrite
  .db $05-2
.ends
