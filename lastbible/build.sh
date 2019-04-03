set -o errexit

if [ ! -e "script/dialogue.txt" ]; then
  echo "ERROR: The required script files have not been placed in the script folder."
  echo "Please see the repository README for instructions."
  exit 1
fi

echo "*******************************************************************************"
echo "Setting up environment..."
echo "*******************************************************************************"

BASE_PWD=$PWD
PATH=".:./asm/bin/:$PATH"
INROM="lastbible.gg"
OUTROM="lastbible_en.gg"
WLADX="./wla-dx/binaries/wla-z80"
WLALINK="./wla-dx/binaries/wlalink"

cp "$INROM" "$OUTROM"

mkdir -p out

echo "*******************************************************************************"
echo "Building tools..."
echo "*******************************************************************************"

make blackt
make libsms
make

if [ ! -f $WLADX ]; then
  
  echo "********************************************************************************"
  echo "Building WLA-DX..."
  echo "********************************************************************************"
  
  cd wla-dx
    cmake -G "Unix Makefiles" .
    make
  cd $BASE_PWD
  
fi

# echo "*******************************************************************************"
# echo "Doing initial ROM prep..."
# echo "*******************************************************************************"
# 
# mkdir -p out
# mold_romprep "$OUTROM" "$OUTROM"

# echo "*******************************************************************************"
# echo "Prepping intro..."
# echo "*******************************************************************************"
# 
# mkdir -p out/intro
# lastbible_introprep rsrc/ out/intro/
# 
# echo "*******************************************************************************"
# echo "Prepping mission intros..."
# echo "*******************************************************************************"
# 
# lastbible_missionintroprep "$OUTROM" "$OUTROM"

echo "*******************************************************************************"
echo "Building font..."
echo "*******************************************************************************"

mkdir -p out/font
vwf_fontbuild rsrc/font_vwf/ out/font/ 0xE

#convert rsrc/title.png -dither None -remap rsrc/orig/title.png PNG32:rsrc/title.png

# echo "*******************************************************************************"
# echo "Building graphics..."
# echo "*******************************************************************************"
# 
# mkdir -p out/precmp
# mkdir -p out/grp
# #grpundmp_gg rsrc/font.png out/precmp/font.bin 0x90
# #grpundmp_gg rsrc/battle_font.png out/grp/battle_font.bin 0x17
# 
# grpundmp_gg rsrc/stageinfo.png out/grp/stageinfo.bin 8
# 
# grpundmp_gg rsrc/unit_moves_left.png out/grp/unit_moves_left.bin 8
# filepatch "$OUTROM" 0x92CF out/grp/unit_moves_left.bin "$OUTROM"
# 
# grpundmp_gg rsrc/resupply_complete.png out/grp/resupply_complete.bin 8
# filepatch "$OUTROM" 0x93CF out/grp/resupply_complete.bin "$OUTROM"
# 
# grpundmp_gg rsrc/completed_1.png out/grp/completed_1.bin 12 -r 4
# grpundmp_gg rsrc/completed_2.png out/grp/completed_2.bin 12 -r 4
# grpundmp_gg rsrc/completed_3.png out/grp/completed_3.bin 12 -r 4
# grpundmp_gg rsrc/completed_4.png out/grp/completed_4.bin 12 -r 4
# filepatch "$OUTROM" 0x3161E out/grp/completed_1.bin "$OUTROM"
# filepatch "$OUTROM" 0x3179E out/grp/completed_2.bin "$OUTROM"
# filepatch "$OUTROM" 0x3191E out/grp/completed_3.bin "$OUTROM"
# filepatch "$OUTROM" 0x31A9E out/grp/completed_4.bin "$OUTROM"
# 
# grpundmp_gg rsrc/congratulations_continued_1.png out/grp/congratulations_continued_1.bin 6 -r 3
# grpundmp_gg rsrc/congratulations_continued_2.png out/grp/congratulations_continued_2.bin 6 -r 3
# filepatch "$OUTROM" 0x69AF3 out/grp/congratulations_continued_1.bin "$OUTROM"
# filepatch "$OUTROM" 0x69BB3 out/grp/congratulations_continued_2.bin "$OUTROM"
# 
# grpundmp_gg rsrc/compendium_menulabel.png out/grp/compendium_menulabel.bin 9
# 
# grpundmp_gg rsrc/font_credits.png out/grp/font_credits.bin 0x50
 
echo "*******************************************************************************"
echo "Building tilemaps..."
echo "*******************************************************************************"

mkdir -p out/intro_burn

lastbible_introprep rsrc/title.png out/intro_burn/

mkdir -p out/maps
mkdir -p out/grp

for file in tilemappers/*; do
  tilemapper_gg "$file"
done

# filepatch "$OUTROM" 0x77DC0 out/maps/compendium_map_main.bin "$OUTROM"
# 
# filepatch "$OUTROM" 0x77D18 out/maps/compendium_map_name00.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D26 out/maps/compendium_map_name01.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D34 out/maps/compendium_map_name02.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D42 out/maps/compendium_map_name03.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D50 out/maps/compendium_map_name04.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D5E out/maps/compendium_map_name05.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D6C out/maps/compendium_map_name06.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D7A out/maps/compendium_map_name07.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D88 out/maps/compendium_map_name08.bin "$OUTROM"
# filepatch "$OUTROM" 0x77D96 out/maps/compendium_map_name09.bin "$OUTROM"
# filepatch "$OUTROM" 0x77DA4 out/maps/compendium_map_name0A.bin "$OUTROM"
# filepatch "$OUTROM" 0x77DB2 out/maps/compendium_map_name0B.bin "$OUTROM"
# 
# filepatch "$OUTROM" 0x3F74F out/maps/title_lastbible_base.bin "$OUTROM"
# filepatch "$OUTROM" 0x3F7F8 out/maps/title_lastbible_mouth.bin "$OUTROM"
# 
# #cat out/grp/title_grp.bin out/maps/title.bin rsrc_raw/title_pal.bin > out/precmp/title_data.bin

# echo "*******************************************************************************"
# echo "Compressing graphics..."
# echo "*******************************************************************************"
# 
# mkdir -p out/cmp
# for file in out/precmp/*; do
#   mold_grpcmp "$file" "out/cmp/$(basename $file)"
# done

# echo "*******************************************************************************"
# echo "Patching graphics..."
# echo "*******************************************************************************"
# 
# #filepatch "$OUTROM" 0x5401C out/cmp/font.bin "$OUTROM" -l 2432
# #filepatch "$OUTROM" 0x56D22 out/grp/battle_font.bin "$OUTROM"
# 
# grpundmp_gg "rsrc/title_sprites.png" "out/grp/title_sprites.bin" 0x40
# 
# grpundmp_gg "rsrc/intro_findtuxedo_chibiusa.png" "out/grp/intro_findtuxedo_chibiusa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_findtuxedo_chibiusa.pal"
# grpundmp_gg "rsrc/intro_findtuxedo_usa.png" "out/grp/intro_findtuxedo_usa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_findtuxedo_usa.pal"
# grpundmp_gg "rsrc/intro_lunap_chibiusa.png" "out/grp/intro_lunap_chibiusa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_lunap_chibiusa.pal"
# grpundmp_gg "rsrc/intro_lunap_usa.png" "out/grp/intro_lunap_usa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_lunap_usa.pal"
# grpundmp_gg "rsrc/intro_quiz_chibiusa.png" "out/grp/intro_quiz_chibiusa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_quiz_chibiusa.pal"
# grpundmp_gg "rsrc/intro_quiz_usa.png" "out/grp/intro_quiz_usa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_quiz_usa.pal"
# grpundmp_gg "rsrc/intro_roulette_chibiusa.png" "out/grp/intro_roulette_chibiusa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_roulette_chibiusa.pal"
# grpundmp_gg "rsrc/intro_roulette_usa.png" "out/grp/intro_roulette_usa.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_roulette_usa.pal"
# grpundmp_gg "rsrc/intro_fortune.png" "out/grp/intro_fortune.bin" 0x28 -r 0x14 -p "rsrc_raw/intro_fortune.pal"
# 
# grpundmp_gg "rsrc/minigame_menu.png" "out/grp/minigame_menu.bin" 0x28 -r 0x14 -p "rsrc_raw/minigame_menu.pal"
# 
# grpundmp_gg "rsrc/quiz_bg.png" "out/grp/quiz_bg.bin" 0x20
# grpundmp_gg "rsrc/lunap_game_bg.png" "out/grp/lunap_game_bg.bin" 0x80

echo "*******************************************************************************"
echo "Building intro..."
echo "*******************************************************************************"

#rm -r out/script
mkdir -p out/script/scroll1
mkdir -p out/script/scroll2
mkdir -p out/script/credits

lastbible_introbuild script/ table/lastbible_vwf_en.tbl out/script/

echo "*******************************************************************************"
echo "Building script..."
echo "*******************************************************************************"

#rm -r out/script
mkdir -p out/script
mkdir -p out/script/strings

# cat together all string files intro one big file for remapping
cat "script/dialogue.txt" "script/items.txt" "script/menus.txt" "script/spells.txt" > "out/script/dialogue_all.txt"

lastbible_scriptbuild script/ table/lastbible_vwf_en.tbl out/script/

echo "********************************************************************************"
echo "Applying ASM patches..."
echo "********************************************************************************"

mkdir -p "out/asm"
cp "$OUTROM" "asm/lastbible.gg"

cd asm
  # apply hacks
  ../$WLADX -I ".." -o "main.o" "main.s"
  ../$WLALINK -s -v linkfile lastbible_patched.gg
  
  mv -f "lastbible_patched.gg" "lastbible.gg"
  
  # update region code in header (WLA-DX forces it to 4,
  # for "export SMS", when the .smstag directive is used
  # -- we want 7, for "international GG")
  ../$WLADX -o "main2.o" "main2.s"
  ../$WLALINK -v linkfile2 lastbible_patched.gg
cd "$BASE_PWD"

mv -f "asm/lastbible_patched.gg" "$OUTROM"
mv -f "asm/lastbible_patched.sym" "$(basename $OUTROM .gg).sym"
rm "asm/lastbible.gg"
rm "asm/main.o"
rm "asm/main2.o"

# echo "*******************************************************************************"
# echo "Finalizing ROM..."
# echo "*******************************************************************************"
# 
# romfinalize "$OUTROM" "out/villgust_chr.bin" "$OUTROM"

echo "*******************************************************************************"
echo "Success!"
echo "Output file:" $OUTROM
echo "*******************************************************************************"
