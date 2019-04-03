 
;=====
; user stuff
;=====
  
; base tile at which vwf tiles are initially allocated
.define vwfTileBase_main $000B
; one past the last index of assignable vwf tiles
;.define vwfTileSize_main $007B-vwfTileBase_main
.define vwfTileSize_main $0100-vwfTileBase_main
; if nonzero, assume nametable scroll is zero regardless of actual value
.define vwfScrollZeroFlag_main $00
; high byte for nametable prints
.define vwfNametableHighMask_main $18
  
.define vwfTileBase_fullscreen $000B
; for fullscreen vwf, we assign from tiles $B to $FF with special
; hardcoded exclusions baked into allocVwfTile for the range $7D-$A0,
; which consists of various tiles (box borders, etc.) we want preserved
.define vwfTileSize_fullscreen $00FF-vwfTileBase_fullscreen
.define vwfScrollZeroFlag_fullscreen $00
.define vwfNametableHighMask_fullscreen $18

.define waitBoxMode2 $1F82
.define waitBoxMode1 $1F87

.define maxVwfTiles $100
.define vwfClearTile $1890

.define vwfSpace1pxIndex $1C
.define vwfBorderFillBase $C1
.define vwfTileBrIndex $EE
.define opNumIndex $F8
.define opNameIndex $F9
.define opInlineNumIndex $FA
.define vwfBoxClearIndex $FD
.define vwfWaitIndex $F1
.define vwfBrIndex $FF
.define terminatorIndex $FF

.define vwfPeriodIndex $69
.define vwfCommaIndex $6A
.define vwfQuestionMarkIndex $12
.define vwfExclamationMarkIndex $01
.define vwfColonIndex $11
.define vwfSemicolonIndex $68

.define ramBankIdentifier $FE
.define noBankIdentifier $FF
.define newStringSentinel $80

.define oldEnemyDigitOffset $02

;  .define speechEmulationBaseCharCount $01

; multiply by 2 for fast speed, 3 for medium, 4 for slow
.define speechEmulationLen_singlePeriod         8
.define speechEmulationLen_comma                5
.define speechEmulationLen_singleExclaim        8
.define speechEmulationLen_singleQuestion       8
.define speechEmulationLen_ellipsis             10
.define speechEmulationLen_colon                5
.define speechEmulationLen_semicolon            8
;.define speechEmulationLen_interrobang          8

;=====
; vwf settings
;=====

; maximum number of digits that can be printed
.define maxPrintingDigits 5

.define vwfWindowRBorderIndex $1A
.define controlCodeStartIndex $EF
.define controlCodeLimitIndex $FF

.define vwfSpaceOffset $20
.define vwfDigitStartOffset $07
.define vwfDigitSpaceOffset $1F

.define vwfAllocatedSentinel $FF
.define vwfNoDeallocSentinel $FE
.define vwfForceAllocSentinel $FD

;=====
; misc
;=====

.define screenVisibleW 20
.define screenVisibleH 18
.define screenVisibleX 6
.define screenVisibleY 3
;.define screenVisibleX 0
;.define screenVisibleY 0


