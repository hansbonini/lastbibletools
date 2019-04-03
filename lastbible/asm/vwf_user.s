
.slot 1
.section "vwf user-implemented code" superfree APPENDTO "vwf and friends"
  ;================================
  ; if any VWF tiles are in use
  ; somewhere in the program,
  ; but are not currently used in
  ; the VDP nametable, flag them
  ; as allocated
  ;================================
  markHiddenVwfTilesAllocated_user:
/*    ; if any VWF tiles are hidden by an open window, flag them
    @windowHideCheck:
    ld a,(openWindowCount)
    or a
    jr z,+
      push de
      push bc
        ld b,a
        -:
          ld hl,(highestOpenWindowAddr)
          
          ; skip VDP addr
          inc l
          inc l
          
          ; height
          ld e,(hl)
          inc l
          ; width
          ld d,(hl)
          inc l
          
          push hl
          push bc
            call checkHiddenVwfTiles
          pop bc
          pop hl
          
          ; move to next-lowest window
          dec h
          djnz -
      
      pop bc
      pop de
    +: */
    ret
  
  ;=====
  ; check if we printed into the tile containing the right border
  ; of the window. if so, we need to draw the border onto the
  ; tile.
  ; (done primarily to allow us to "cheat" so we can squeeze
  ; seven-character party member names into what was supposed to be
  ; a four-tile space)
  ;=====
  checkBorderTransfer_user:
/*    ; FIXME: oops non-nametable prints aren't setting up the width
    ld a,(vwfLocalTargetFlag)
    or a
    ret z
    
    ; FIXME: this only works for nametable transfers if the base x-offset
    ; is 1
    ld a,(printOffsetX)
    inc a
    inc a
;    ld hl,printAreaW
    ld hl,vwfLocalTargetW
    cp (hl)
    jr nz,+
      push bc
        ; border is 4px on right side of tile
        ld c,$F0
        ld b,:font_rshift_00
        ld de,vwfBuffer
        ld hl,font_rshift_00+(bytesPerTile*vwfWindowRBorderIndex)
        call orToTileBuffer
      pop bc
    +: */
    
    ret
  
  ;================================
  ; check for special printing
  ; sequences.
  ;
  ; A = character index
  ; HL = pointer to data immediately following character
  ;================================
;  checkPrintOpcodes_user:
  printVwfChar_user:
    ; check for linebreak
    cp vwfBrIndex
    jr nz,+
      call sendVwfBufferIfPending
      
      ; reset VWF
      call resetVwf
      
      @vdpLinebreak:
      ; reset X
      xor a
      ld (printOffsetX),a
      
      ; Y++
      ld a,(printOffsetY)
;          add a,$02
      inc a
      ld (printOffsetY),a
      
      ld a,(vwfLocalTargetFlag)
      or a
      jr z,++
        @localLinebreak:
        push hl
          ld hl,(vwfLocalTargetCurrLineAddr)
          
          ; add nametable tile width * 2 to current line address to
          ; get next line's address
          ld a,(vwfLocalTargetW)
          sla a
          ld e,a
          ld d,$00
          add hl,de
          
          ld (vwfLocalTargetCurrLineAddr),hl
        pop hl
        jr @done
      ++:
      
      ; if printing to VDP and we exceeded the height of the printing area,
      ; we have to shift the bottom rows up
      ld a,(printAreaH)
      ld e,a
      ld a,(printOffsetY)
      cp e
      jr c,@done
        call doLineBreakLineShift
        jr @done
    +:
    
    ; check for box clear
    cp vwfBoxClearIndex
    jr nz,+
      @boxClear:
      
      ; deallocate box area
      ld hl,(printBaseXY)
      ld bc,(printAreaWH)
      call deallocVwfTileArea
      
      ; clear box (fill with tile 0101)
      push hl
        ld bc,(printAreaWH)
        
;        ld de,(oldPrintNametableBase)
;        ld a,e
;        add a,$01
;        ld e,a
      ld de,vwfClearTile
        
        ld hl,(printBaseXY)
        
        call clearNametableArea
        
        ; reset print offset
        ld hl,$0000
        ld (printOffsetXY),hl
      pop hl
      
      jr @done
    +:
    
    ; check for old number op
/*        cp opNumIndex
    jr nz,+
      @oldNumOp:
      push hl
        ; get target number
        ld hl,($C520)
        ; digit count = don't care, hide leading zeroes
        ld bc,$0000
        call prepNumberString
        
        ; print result
        ld hl,numberPrintBuffer
        call printVwfString
      pop hl
      jr @done
    +: */
    
    ; check for new inline number print op
    cp opInlineNumIndex
    jr nz,+
      @newNumOp:
      push hl
        call printScriptNum
      pop hl
      jr @done
    +:
    
    ; check for name op
/*        cp opNameIndex
    jr nz,+
      push hl
        call printScriptName
      pop hl
      jr @done
    +: */
    
;        ; check for ops
;        cp controlCodeStartIndex
;        jr c,+
;          
;          controlCodeJumpTable:
;          
;        +:

    ld c,a
    call printVwfChar

    @done:
    ret
    
  ; BC = w/h
  ; DE = clear value
  ; HL = x/y
  clearNametableArea:
    dec b
    dec c
    
    @clearYLoop:
      
      push bc
      
      @clearXLoop:
        push hl
        push bc
          add hl,bc
          call writeLocalTileToNametable
        pop bc
        pop hl
        
        dec b
        jp p,@clearXLoop
      
      pop bc
      dec c
      jp p,@clearYLoop
    ret
  
  ; move box lines 1..n up a line, deleting line 0
  doLineBreakLineShift:
    push hl
      ; deallocate top line
      ld hl,(printBaseXY)
      ld bc,(printAreaWH)
      ld c,1
      call deallocVwfTileArea
      
      ; target lines 1..n
      ld hl,(printBaseXY)
      inc l
      ld bc,(printAreaWH)
      dec c
      
      @yLoop:
        
        push hl
        push bc
        @xLoop:
          push bc
            ; read tile from original pos
            push hl
              call readLocalTileFromNametable
            pop hl
            
            ; write to (y - 1)
            push hl
              dec l
              call writeLocalTileToNametable
            pop hl
          pop bc
          ; move to next x-pos
          inc h
          djnz @xLoop
        
        pop bc
        pop hl
        ; move to next y-pos
        inc l
        dec c
        jr nz,@yLoop
      
      ; move y-offset up a line
      
      ld a,(printOffsetY)
      dec a
      ld (printOffsetY),a
      
      ; clear bottom line
      
      ld bc,(printAreaWH)
      
;      ld de,(oldPrintNametableBase)
;      ld a,e
;      add a,$01
;      ld e,a
      ld de,vwfClearTile
      
      ; target bottom line
      ld hl,(printBaseXY)
      ld a,l
      add a,c
      dec a
      ld l,a
      
      ld c,1
      
      call clearNametableArea
    
    pop hl
    ret
  
  ; B = string bank
  ; HL = string pointer
  printVwfString_user:
    ; load string bank (slot 1)
;    ld a,(mapperSlot1Ctrl)
;    push af
;      ld a,b
;      ld (mapperSlot1Ctrl),a

      @printLoop:
        call bankedFetch
;        ld a,(hl)
        inc hl
        
        ; check for terminator
        cp terminatorIndex
        jr z,@printingDone
          
        push bc
  ;        call checkPrintOpcodes_user
          push hl
            call printVwfChar_user
          pop hl
          
          ; C = target char index
;          ld c,a
;          push hl
;            call printVwfChar
;          pop hl
;        inc hl
        pop bc
        jr @printLoop
      
      @printingDone:
      
      ; do possible final transfer
      call sendVwfBufferIfPending
      
;    pop af
;    ld (mapperSlot1Ctrl),a
    ret
  
  .define mainScreenScrollXLo $C460
  .define mainScreenScrollYLo $C462

  getScrollX_user:
    ld a,(mainScreenScrollXLo)
    ret
  
  getScrollY_user:
    ld a,(mainScreenScrollYLo)
    ret
  
.ends

;========================================
; string hash lookup
;========================================

.slot 0
.section "string hash lookup" free

  ;========================================
  ; string hash lookup
  ; 
  ; HL = pointer to orig string (in
  ;      appropriate slot)
  ;
  ; returns:
  ;   C = new bank (or fail code)
  ;   HL = new pointer
  ;========================================
  getStringHash:
    call getPointerBank
    doBankedCallSlot1 getStringHash_ext
    ret
  
.ends

.slot 1
.section "string hash lookup 2" superfree APPENDTO "vwf and friends"
  ;========================================
  ; string hash lookup
  ; 
  ; A = C = pointer to orig bank
  ; HL = pointer to orig string (in
  ;      appropriate slot)
  ;
  ; returns:
  ;   C = new bank (or fail code)
  ;   HL = new pointer
  ;========================================
  getStringHash_ext:
    ; if RAM identifier, no change needed
    cp ramBankIdentifier
    jr z,@done
    
      ; look up hash
      ld b,:bucketArrayHashTabledialogue
      doBankedCallSlot1 lookUpHashedPointer
    
    @done:
    ret
.ends

;========================================
; string print
;========================================

.slot 1
.section "new printString 2" superfree APPENDTO "vwf and friends"
  ;=====================================
  ; HL = local x/y
  ;=====================================
  initString:
    ; init
    xor a
    ld (textSpeed),a
    ld (old_lineNum),a
    ld a,h
    ld (old_printBaseX),a
    ld (printBaseX),a
    ld a,l
    ld (old_printBaseY),a
    ld (printBaseY),a
;      call setPrintAddrForCurLine

    ex de,hl
    ld bc,dialogueBoxMode1Size
;    doBankedCall initVwfString
    jp initVwfString
    
  borderPrintStartCheck:
    ;=====
    ; check if the tile at our starting point is
    ; either a window corner (0082) or window top/bottom (0083).
    ; if so, print a 1px space so the text will not be flush against the
    ; border.
    ;=====
    
    ld hl,(printBaseXY)
    
    ; don't do check if x is zero
;    ld a,h
;    or a
;    jr z,@borderCheckDone
      ; read tile from (x-1,y)
;      dec h
      call readLocalTileFromNametable
      
      ; upper bit of pattern num must be zero
      srl d
      jr c,@borderCheckDone
      
      ld a,e
      cp $82
      jr z,@matched
      cp $83
      jr nz,@selfOverwriteCheck
      
        @matched:
        ld (needRightBorderFlag),a
        ; print 1px space
        ld c,vwfSpace1pxIndex
        call printVwfChar
        jr @borderCheckDone
      
    @selfOverwriteCheck:
    
/*    ;=====
    ; HACK: ok, here's a hack for you:
    ; before printing a string directly to the nametable, check if the
    ; first tile we're being asked to write to is
    ; 1. a VWF tile
    ; 2. not a window border (tiles 0x82, 0x83; 0x84 doesn't matter)
    ; 3. not a space (tile 0x90)
    ; if all these conditions are met, assume we are being asked to
    ; overwrite a string with itself and print nothing.
    ; this stops "flicker" from erasing and overwriting the same text.
    ;=====
    
    ; conditions 1 and 2 are already checked at this point.
    ; check if a space.
    cp oldSpaceIndex
    jr z,@borderCheckDone
      
      ; clear carry flag to indicate string printing should not proceed
      scf
      ccf
      ret */
        
    @borderCheckDone:

    ; return carry set to indicate string printing should proceed
;    scf
    ret
  
  borderPrintEndCheck:
    ;=====
    ; if we previously detected that we're printing over a window border,
    ; fill in the remainder of the tile with a border (leaving a 1px
    ; margin on the right.)
    ; remember that characters have a 1px space prebaked into their right
    ; side.
    ;=====
    
    ld a,(needRightBorderFlag)
    or a
    jr z,@done
      
      ; clear flag
      xor a
      ld (needRightBorderFlag),a
      
      ; decide what we need to print based on final vwfPixelOffset
      ld a,(vwfPixelOffset)
      
      ; if zero or >= 8 (should be impossible), we don't need to do anything
      or a
      jr z,printDone
      cp 8
      jr nc,printDone
      
      ; otherwise, add base index of border tiles to get one we need to print
      ; (minus 1 to exclude the non-existent zero case)
      add a,vwfBorderFillBase-1
      ld c,a
      call printVwfChar
    
    @done:
    ret
  
  ;=====================================
  ; HL = local x/y
  ; IX = string pointer
  ;=====================================
  startNewString:
    call initString
    call borderPrintStartCheck
;    jr nc,+
      call newStringPrintLogic
;    +:
    jp borderPrintEndCheck
;    ret
    
  ;=====================================
  ; IX = string pointer
  ;=====================================
  newStringPrintLogic:
    ;=====
    ; print
    ;=====
  
    @charPrintLoop:
      call doPrintDelay
      
      ; speech emulation check
      ld a,(speechEmulationDisableFlag)
      or a
      jr nz,+
        call checkSpeechEmulation
      +:
      
      ld a,(ix+0)
      inc ix
      cp $FF
      jr z,@printDone
      cp controlCodeStartIndex
      jr c,+
        sub controlCodeStartIndex
        ld hl,opJumpTable
        read16BitTable_macro
        call opJumpCall
        jr @nextChar
      +:
      ld c,a
;      doBankedCall printVwfChar
      call printVwfChar
      
      @nextChar:
;        inc ix
      jr @charPrintLoop
    
    @printDone:
    
    printDone:
      ret
  
  checkSpeechEmulation:
    ; do nothing if insta-printing
    ld a,(instantPrintFlag)
    or a
    ret nz
    
    ; do nothing if text speed manually overriden
    ld a,(textSpeed)
    or a
    ret nz
  
    ; do nothing if already handling an existing sequence (chars remaining
    ; >= 2)
    ld a,(speechEmulationCharsLeft)
    cp $02
;    ret nc
    jp nc,@terminatorCheck
    
    push ix
      
      ; fetch next char
      ld a,(ix+0)
      
      ; period
      cp vwfPeriodIndex
      jr nz,+
        ; if there is more than one sequential period, consider all periods
        ; up to the next non-period an ellipsis
        inc ix
        ld a,(ix+0)
        cp vwfPeriodIndex
        jr nz,@singlePeriod
          
        @ellipsis:
          ; search for first non-period
          ld b,$01
          -:
            inc b
            inc ix
            ld a,(ix+0)
            cp vwfPeriodIndex
            jr z,-
          
          ; while first non-period is a question mark or exclamation mark,
          ; include in the count
          -:
          cp vwfQuestionMarkIndex
          jr z,++
          cp vwfExclamationMarkIndex
          jr z,++
          jr @ellipsisSearchDone
          ++:
            inc b
            inc ix
            ld a,(ix+0)
            jr -
          
          @ellipsisSearchDone:
          ld c,speechEmulationLen_ellipsis
          jr @setDelay
;          ld a,speechEmulationLen_ellipsis
;          ld (speechEmulationDelayAmount),a
;          ld a,b
;          inc a
;          ld (speechEmulationDelayedDelayCounter),a
;          jr @done
        @singlePeriod:
          ld c,speechEmulationLen_singlePeriod
          ld b,$01
          jr @setDelay
      +:
      ; question mark
      cp vwfQuestionMarkIndex
      jr nz,+
        ; interrobang?
;        inc ix
;        ld a,(ix+0)
;        cp vwfExclamationMarkIndex
;        jr z,@interrobang
        
        ; if followed by another question or exclamation mark, ignore
        inc ix
        ld a,(ix+0)
        cp vwfQuestionMarkIndex
        jr z,@done
        cp vwfExclamationMarkIndex
        jr z,@done
        
        ld c,speechEmulationLen_singleQuestion
        ld b,$01
        jr @setDelay
      +:
      ; exclamation mark
      cp vwfExclamationMarkIndex
      jr nz,+
        ; interrobang?
;        inc ix
;        ld a,(ix+0)
;        cp vwfQuestionMarkIndex
;        jr z,@interrobang
        
        ; if followed by another exclamation or question mark, ignore
        inc ix
        ld a,(ix+0)
        cp vwfQuestionMarkIndex
        jr z,@done
        cp vwfExclamationMarkIndex
        jr z,@done
        
        ld c,speechEmulationLen_singleExclaim
        ld b,$01
        jr @setDelay
        
;        @interrobang:
;        ld c,speechEmulationLen_interrobang
;        ld b,$02
;        jr @setDelay
      +:
      ; comma
      cp vwfCommaIndex
      jr nz,+
        ld c,speechEmulationLen_comma
        ld b,$01
        jr @setDelay
      +:
      ; colon
      cp vwfColonIndex
      jr nz,+
        ld c,speechEmulationLen_colon
        ld b,$01
        jr @setDelay
      +:
      ; semicolon
      cp vwfSemicolonIndex
      jr nz,+
        ld c,speechEmulationLen_semicolon
        ld b,$01
        jr @setDelay
      +:
      
      ; no match: do nothing
      jr @done
      
    @setDelay:
    ; C = delay, B = raw numchars
    
    ; account for user's text speed setting
    push bc
      ld a,(textSpeedSetting)
      inc a
      ld b,a
      
      ; if fast-forward on, treat all speeds as fast
      @temp:
      ld a,(textFastForwardFlag)
      or a
      jr z,+
        ld b,$01
      +:
      
      ; multiply delay by 2 for fast text, 3 for medium, 4 for slow
      ld a,c
      -:
;        sla c
        add a,c
        djnz -
    pop bc
    
    ld (speechEmulationDelayAmount),a
    ld a,b
    inc a       ; account for future predecrement
    ld (speechEmulationCharsLeft),a
    
    @done:
    pop ix
    
    ; HACK: don't delay for last character before terminator or wait
    
    ; ignore the actual terminating characters
    @terminatorCheck:
    ld a,(ix+0)
    cp terminatorIndex
    jr z,@endCheckDone
    cp vwfWaitIndex
    jr z,@endCheckDone
      ld a,(ix+1)
      cp terminatorIndex
      jr z,++
      cp vwfWaitIndex
      jr z,++
        jr @endCheckDone
      ++:
        xor a
        ld (speechEmulationCharsLeft),a
    @endCheckDone:
    ret
  
  ;=====================================
  ; IX = string pointer
  ;=====================================
  appendNewString:
;    ld a,(needRightBorderFlag)
;    push af
;      xor a
;      ld (needRightBorderFlag),a
;      call newStringPrintLogic
;    pop af
;    ld (needRightBorderFlag),a
;    ret
    jp newStringPrintLogic
  
  ;=====================================
  ; C = max length
  ; HL = string pointer
  ;=====================================
  startLimitedString:
    push bc
    push hl
      ; convert old nametable target to local coords
      ld hl,old_curPrintNametableAddr
      call nametableAddrToLocalCoords
      call initString
    pop hl
    pop bc
  ; C = max length
  ; HL = string pointer
  appendLimitedString:
    @charPrintLoop:
      push bc
      call doPrintDelay
      ld a,(hl)
      inc hl
      
      ; stop when the old space index (0x90) is encountered,
      ; because this is what the game expects for e.g. character names
      ; TODO: this maps to some narrow character in the new font: does
      ; it matter?
      cp oldSpaceIndex
      jr z,@printDoneAlt
      ld c,a
      push hl
        call printVwfChar
      pop hl
      
      @nextChar:
      pop bc
      dec c
      jr z,@printDone
      jr @charPrintLoop
    
    @printDoneAlt:
    pop bc
    @printDone:
    ret
  
  ; A = max length
  ; C = bank or fail code
  ; HL = string pointer (or garbage if C == fail code)
  printRemappedLimitedString:
    ; B = max length
    ld b,a
    ; A = bank or fail code
    ld a,c
    
    ; if fail code, oops
    cp noBankIdentifier
    ret z
    
    ; set up print location
    push bc
    push hl
      ; convert nametable pos to local
      ld hl,(old_curPrintNametableAddr)
      call nametableAddrToLocalCoords
      
      ex de,hl
      ; supposed w/h -- we don't really know and it doesn't matter
      ld bc,$0701
      call initVwfString
      
      ; start border if needed
      call borderPrintStartCheck
    pop hl
    pop bc
    
    ; if string is in ram, we need to figure out what it is so we can
    ; handle it appropriately
    ld a,c
    cp ramBankIdentifier
    jr nz,@stringIsRemappable
    @stringIsInRam:
      ; the only things we should ever be printing from ram with this
      ; (i hope) are ally or enemy names.
      ; ally names will come from the slots at C580+
      ; enemy names will come from the slots at C8C0+
      
      ; assume any pointer >= C8C0 is an enemy name pointer
      dec hl
      push bc
      push hl
        ld bc,enemySlotsStart
        or a
        sbc hl,bc
      pop hl
      pop bc
      ld c,b
      jr nc,@enemy
      
      @ally:
        call handleAllyNamePrint_fromPointer
        jr @done
      @enemy:
        call handleEnemyNamePrint_fromPointer
        jr @done
      
    @stringIsRemappable:
    ; do as we normally would
    ; switch to target bank
    ld a,(mapperSlot2Ctrl)
    push af
    push ix
      ld a,c
      ld (mapperSlot2Ctrl),a
      ; print hashed string
      push hl
      pop ix
      call appendNewString
    pop ix
    pop af
    ld (mapperSlot2Ctrl),a
    
    @done:
    ; end border if needed
    jp borderPrintEndCheck
;    ret
  
  opJumpCall:
    jp (hl)
  
  opJumpTable:
    ; EF = [stringbuf]
    .dw handleOp_stringbuf
    ; F0 = [char]
    .dw handleOp_char
    ; F1 = [wait]
    .dw waitBoxMode1
    ; F2 = [br]
    .dw handleOp_br
    ; F3 = [textspeed]
    .dw handleOp_textspeed
    ; F4 = [clear]
    .dw handleOp_clear
    ; F5 = [ally]
    .dw handleOp_ally
    ; F6 = [wait2]
    .dw waitBoxMode2
    ; F7 = [enemy]
    .dw handleOp_enemy
    ; F8 = [num]
    .dw handleOp_num
    ; F9 = [scroll]
    .dw handleOp_scroll
    ; FA = [scroll2]
    .dw handleOp_scroll2
    ; FB = [money]
    .dw handleOp_money
    ; FC = [nop]
    .dw printDone
    ; FD = [spell]
    .dw handleOp_spell
    ; FE = [item]
    .dw handleOp_item
    ; FF = [end] (hardcoded, this should never be called)
    .dw printDone
  
  handleOp_stringbuf:
    ; print content
    push ix
      ld ix,stringBuffer
      call appendNewString
    pop ix
    ret
  
  handleOp_char:
    ; fetch character ID
    ld a,(ix+0)
    inc ix
    
    jp handleAllyNamePrint
  
  handleOp_br:
    ; reset X
    xor a
    ld (printOffsetX),a
    
    ; Y++
    ld hl,printOffsetY
    inc (hl)
;      ld a,(printOffsetY)
;      inc a
;      ld (printOffsetY),a
    
    ld a,(vwfLocalTargetFlag)
    or a
    jr z,++
      @localLinebreak:
      ld hl,(vwfLocalTargetCurrLineAddr)
      
      ; add nametable tile width * 2 to current line address to
      ; get next line's address
      ld a,(vwfLocalTargetW)
      sla a
      ld e,a
      ld d,$00
      add hl,de
      
      ld (vwfLocalTargetCurrLineAddr),hl
      jr @done
    ++:
    
    @done:
    ret
  
  handleOp_textspeed:
    ; fetch speed
    ld a,(ix+0)
    inc ix
    ; update value
    ld (textSpeed),a
    ret
  
  handleOp_clear:
    ; assume mode1 box
    ld bc,dialogueBoxMode1Size
    ld (printAreaWH),bc
    
    @logic:
    ; deallocate box area
    ld hl,(printBaseXY)
;      ld bc,(printAreaWH)
;    doBankedCall deallocVwfTileArea
    call deallocVwfTileArea
    
    ld bc,(printAreaWH)
    ld de,vwfClearTile
    ld hl,(printBaseXY)
      
;    doBankedCall clearNametableArea
    call clearNametableArea
    
    ; reset print offset
    ld hl,$0000
    ld (printOffsetXY),hl
      
    ret
  
  ; clear mode 2 box (battle)
  ; this doesn't exist in the original game, but we need it
  handleOp_clear2:
    ; assume mode2 box
    ld bc,dialogueBoxMode2Size
    ld (printAreaWH),bc
    jr handleOp_clear@logic
    
/*    ; deallocate box area
    ld hl,(printBaseXY)
;      ld bc,(printAreaWH)
;    doBankedCall deallocVwfTileArea
    call deallocVwfTileArea
    
    ld bc,(printAreaWH)
    ld de,vwfClearTile
    ld hl,(printBaseXY)
      
;    doBankedCall clearNametableArea
    call clearNametableArea
    
    ; reset print offset
    ld hl,$0000
    ld (printOffsetXY),hl
      
    ret */
  
  handleOp_ally:
    ; unfortunately, it turns out this op is not actually exclusive to
    ; allies, so we have to check for enemy IDs too
    ld a,(activeAllyId)
    jr handleOp_enemy@check
    
  handleOp_enemy:
    ; unfortunately, it turns out this op is not actually exclusive to
    ; enemies, so we have to check for player IDs too
    ld a,(activeEnemyId)
    @check:
    or a
    jp p,handleEnemyNamePrint
    jr handleAllyNamePrint
  
  ; A = slot ID
  handleAllyNamePrint:
    call lookUpAllySlot
  ; HL = slot pointer
  handleAllyNamePrint_fromPointer:
    ; does slot contain a human character (byte 0 = 80, 81, or 82)?
    ld a,(hl)
    or a
    jp p,@beast
    ; if so, limited-print name directly from ram
    @human:
      ; get pointer to character name
      inc hl
      ; print as limited string
      ld c,maxPlayerNameLen
      call appendLimitedString
      jr @done
    ; otherwise, do enemy name composition procedure and print that
    @beast:
      ; look up name
      push hl
        ; copy name to buffer
        ld c,a
        ld de,stringBuffer
        ld b,:enemyNames
        ld hl,enemyNames
        call copyTabledString
      pop hl
      
      ; print content
      push ix
        ld ix,stringBuffer
        call appendNewString
      pop ix
    @done:
    ret
  
  ; A = slot ID
  handleEnemyNamePrint:
    call lookUpEnemySlot
  ; HL = slot pointer
  handleEnemyNamePrint_fromPointer:
    ; get enemy type
    ld a,(hl)
    ; look up name
    push hl
      ; copy name to buffer
      ld c,a
      ld de,stringBuffer
      ld b,:enemyNames
      ld hl,enemyNames
      call copyTabledString
    pop hl
    
    ; get pointer to first byte of what used to be enemy name
    inc hl
    ; get what used to be the first byte of the enemy name, but will now be
    ; either a space (if no number needed after name) or the enemy's subslot
    ; number.
    ld a,(hl)
;    cp oldSpaceIndex
;    jr z,@numberAddDone
    ; OR, AFTER DISCOVERING THE FUSION MENU: something at the start of the
    ; name that is not a digit 1-9 in the old font (0x2-0xA), in which
    ; case we assume no numbering
    cp $01
    jr c,@numberAddDone
    cp $0B
    jr nc,@numberAddDone
      ; add space after name content
      ld a,vwfSpaceOffset
      ld (de),a
      inc de
      
      ; convert slot index to VWF digit
      ld a,(hl)
      add a,vwfDigitStartOffset-oldEnemyDigitOffset+1
      ld (de),a
      inc de
    
      ; add terminator
      ld a,terminatorIndex
      ld (de),a
    @numberAddDone:
    
    ; print content
    push ix
      ld ix,stringBuffer
      call appendNewString
    pop ix
    
    ret
  
  handleOp_num:
    call toBcd
    ; get digit count
    ld hl,bcdResultDigitCount
    ld a,(hl)
    ld b,a
    ; add digit count to base pos
    add a,l
    ld l,a
    jr nc,+
      inc h
    +:
    ; print each digit (digits are in reverse order)
    -:
      ld a,(hl)
      dec hl
      ; add VWF digit offset
      add a,vwfDigitStartOffset
      ; print
      ld c,a
      push hl
        push bc
          call printVwfChar
        pop bc
        
        ; note: original game does not do this. numbers always print
        ; instantly.
        ld a,b
        cp $01
        jr z,+
          call doPrintDelay
        +:
      pop hl
      djnz -
    
    ret
  
  handleOp_scroll:
    ld hl,dialogueBoxMode1Size
    @call:
    ld (printAreaWH),hl
;    doBankedCall doLineBreakLineShift
    call doLineBreakLineShift
    
    ; decrement linenum
;      ld hl,printOffsetY
;      dec (hl)
    ret
  
  handleOp_scroll2:
    ld hl,dialogueBoxMode2Size
    jr handleOp_scroll@call
  
  handleOp_money:
    call swapBcdConvBuffers
    call handleOp_num
  ; !!! drop through !!!
  swapBcdConvBuffers:
    ; swap D200-D202 with D203-D205
    ld hl,$D200
    ld de,$D203
    ; note: original game does not set B here, resulting in an undefined
    ; value for the djnz below.
    ; B appears to always be zero in the cases where this routine was used
    ; in the original game, resulting in all bytes from D203-D2FF getting
    ; shifted 3 bytes to the left while D200-D202 is relocated to D300-D302.
    ; this just happens to work fine due to none of that memory being
    ; used for anything else.
    ; here we realize the intended behavior of swapping the BCD source
    ; buffers.
    ld b,$03
    -:
      ; A = fetch from D203+
      ld a,(de)
      ; C = fetch from D200+
      ld c,(hl)
      ; switch values
      ld (hl),a
      ld a,c
      ld (de),a
      inc de
      inc hl
      djnz -
    ret
  
  handleOp_spell:
    ld a,(activeSpellId)
    call lookUpSpell
    call getStringHash
    jp appendBankedString
  
  handleOp_item:
    ld a,(activeItemId)
    call lookUpItem
    call getStringHash
    jp appendBankedString
  
  
  
  ; C = bank
  ; HL = pointer
  appendBankedString:
    ld a,(mapperSlot2Ctrl)
    push af
    push ix
      ld a,c
      ld (mapperSlot2Ctrl),a
      push hl
      pop ix
      call appendNewString
    pop ix
    pop af
    ld (mapperSlot2Ctrl),a
    ret
  
  ; B = table bank
  ; C = index
  ; DE = dstptr
  ; HL = offset table start pointer (slot 2)
  ;
  ; returns
  ;   DE = pointer to terminator of copied content.
  copyTabledString:
    ld a,(mapperSlot2Ctrl)
    push af
      ld a,b
      ld (mapperSlot2Ctrl),a
      
      ld a,c
      call readOffsetTable
      
      ; copy loop
      -:
      ; TODO: handle narrow-font conversion here?
      ; or make separate routine?
        ld a,(hl)
        ld (de),a
        
        ; check for terminator
        cp terminatorIndex
        jr z,+
        
        inc hl
        inc de
        jr -
      +:
    pop af
    ld (mapperSlot2Ctrl),a
    ret
  
  ; A = enemy quantity
  ; C = enemy typeslot (0 for first type, 1 for second)
  showPluralEnemiesAppearedMessage:
    ; set string buffer to enemy quantity/type string
    push bc
      call composePluralEnemyString
    pop bc
    ; print the plural enemy message
    openTempBankSlot2 :pluralEnemyAppearedString
      push ix
        ld a,(instantPrintFlag)
        push af
          ld a,$FF
          ld (instantPrintFlag),a
          ld ix,pluralEnemyAppearedString
          
;          ld hl,dialogueBoxMode2ContentPos
          ; set target line depending on which enemy type we're printing
          ld a,c
          add a,$09
          ld l,a
          ld h,$01
          
          call startNewString
        pop af
        ld (instantPrintFlag),a
      pop ix
    closeTempBankSlot2
    ret
  
  ; A = enemy quantity
  composePluralEnemyString:
    ld de,stringBuffer
    
    ; write quantity string
    ld hl,ordinalNumberStringTable
    ld c,a
    ld b,:ordinalNumberStringTable
    call copyTabledString
    
    ; add space
    ld a,vwfSpaceOffset
    ld (de),a
    inc de
    
    ; get enemy type
    push hl
    push de
      ; get enemy slot pointer
      ld a,(activeEnemyId)
      call lookUpEnemySlot
      ; get type
      ld a,(hl)
    pop de
    pop hl
    
    ; add plural enemy name
    ld hl,enemyNamesPlural
    ld c,a
    ld b,:enemyNamesPlural
    call copyTabledString
    ret
  
.ends