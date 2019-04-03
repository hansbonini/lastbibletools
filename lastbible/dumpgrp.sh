mkdir -p rsrc
mkdir -p rsrc/orig
mkdir -p rsrc_raw
mkdir -p rsrc_raw/grp

make libsms && make rawdmp
make libsms && make grpdmp_gg
make libsms && make tilemapdmp_gg
#make libsms && make godzilla_fontdmp
make libsms && make lastbible_decmp

# unit font
#./godzilla_decmp godzilla.gg 0x3DB5E rsrc_raw/title_godzilla_grp.bin
#./tilemapdmp_gg godzilla.gg 0x3F74F half 0xD 0xD rsrc_raw/title_godzilla_grp.bin 0x0101 rsrc/orig/title_godzilla_base.png -h 0x01
# resupply complete overlay

./lastbible_decmp lastbible.gg 0x7F780 rsrc_raw/grp/font_main.bin
./lastbible_decmp lastbible.gg 0x29620 rsrc_raw/grp/title.bin

#rm -r -f rsrc_raw/rawdmp
mkdir -p rsrc_raw/grp/rawdmp
for file in rsrc_raw/grp/*.bin; do
  ./grpdmp_gg "$file" "rsrc_raw/grp/rawdmp/$(basename $file .bin).png"
done

./tilemapdmp_gg lastbible.gg 0x1A58D half 0x14 0x12 rsrc_raw/grp/title.bin 0x0100 rsrc/orig/title.png -h 0x01 -p rsrc_raw/title_distinct.pal
./tilemapdmp_gg lastbible.gg 0x1A6F5 half 0x14 0x12 rsrc_raw/grp/title.bin 0x0100 rsrc/orig/title_empty.png -h 0x01 -p rsrc_raw/title_distinct.pal
./tilemapdmp_gg lastbible.gg 0x1A85D half 0x14 0x3 rsrc_raw/grp/title.bin 0x0100 rsrc/orig/title_bottom.png -h 0x01 -p rsrc_raw/title_distinct.pal


# generic
# ./bssm_decmp bssmgg.gg 0xA111 rsrc_raw/grp/font_main.bin
# ./bssm_decmp bssmgg.gg 0x70486 rsrc_raw/grp/background-patterns.bin
# ./bssm_decmp bssmgg.gg 0x7AD2F rsrc_raw/grp/stage_intro.bin
# ./bssm_decmp bssmgg.gg 0x13379 rsrc_raw/grp/cursor.bin
# 
# # title
# ./bssm_decmp bssmgg.gg 0x8796 rsrc_raw/grp/title_bg.bin
# ./bssm_decmp bssmgg.gg 0x9B3F rsrc_raw/grp/title_sprites.bin
# ./tilemapdmp_gg bssmgg.gg 0x1B4E7 full 0x14 0x0A rsrc_raw/grp/title_bg.bin 0x0 rsrc/orig/title.png -p rsrc_raw/title.pal
# 
# # menus
# ./bssm_decmp bssmgg.gg 0x5B466 rsrc_raw/grp/mainmenu.bin
# ./bssm_decmp bssmgg.gg 0x11C25 rsrc_raw/grp/minigame_menu.bin
# ./tilemapdmp_gg bssmgg.gg 0x10042 full 0x14 0x12 rsrc_raw/grp/mainmenu.bin 0x0 rsrc/orig/mainmenu.png
# 
# # transformation
# ./bssm_decmp bssmgg.gg 0x48000 rsrc_raw/grp/transform_usa-1.bin
# ./bssm_decmp bssmgg.gg 0x497F4 rsrc_raw/grp/transform_usa-2.bin
# ./bssm_decmp bssmgg.gg 0x3C44E rsrc_raw/grp/transform_chibiusa-1.bin
# ./bssm_decmp bssmgg.gg 0x4C000 rsrc_raw/grp/transform_chibiusa-2.bin
# 
# # luna-p ball's picnic
# ./bssm_decmp bssmgg.gg 0x74000 rsrc_raw/grp/lunap_intro_usa.bin
# ./bssm_decmp bssmgg.gg 0x64000 rsrc_raw/grp/lunap_intro_chibiusa.bin
# ./bssm_decmp bssmgg.gg 0x655E5 rsrc_raw/grp/lunap_game_bg.bin
# ./bssm_decmp bssmgg.gg 0x6606D rsrc_raw/grp/lunap_game_sprites.bin
# 
# # find tuxedo mask
# ./bssm_decmp bssmgg.gg 0x7558C rsrc_raw/grp/findtuxedo_intro_usa.bin
# ./bssm_decmp bssmgg.gg 0x68200 rsrc_raw/grp/findtuxedo_intro_chibiusa.bin
# # bg graphic shared with roulette
# ./bssm_decmp bssmgg.gg 0x6C488 rsrc_raw/grp/findtuxedo_bg.bin
# ./bssm_decmp bssmgg.gg 0x6F79A rsrc_raw/grp/findtuxedo_sprites_usa.bin
# ./bssm_decmp bssmgg.gg 0x70000 rsrc_raw/grp/findtuxedo_sprites_tuxedo.bin
# 
# # sailor team roulette
# ./bssm_decmp bssmgg.gg 0x44000 rsrc_raw/grp/roulette_intro_usa.bin
# ./bssm_decmp bssmgg.gg 0x69CAF rsrc_raw/grp/roulette_intro_chibiusa.bin
# 
# # quiz world
# ./bssm_decmp bssmgg.gg 0x78000 rsrc_raw/grp/quiz_intro_usa.bin
# ./bssm_decmp bssmgg.gg 0x70ADA rsrc_raw/grp/quiz_intro_chibiusa.bin
# ./bssm_decmp bssmgg.gg 0x7203B rsrc_raw/grp/quiz_bg.bin
# ./bssm_decmp bssmgg.gg 0x6311A rsrc_raw/grp/quiz_potraits1.bin
# 
# # rei's fortune teller
# ./bssm_decmp bssmgg.gg 0x6E224 rsrc_raw/grp/fortune_intro.bin
# ./bssm_decmp bssmgg.gg 0x76A72 rsrc_raw/grp/fortune_main.bin
# ./tilemapdmp_gg bssmgg.gg 0x77BB5 full 0x14 0x12 rsrc_raw/grp/fortune_main.bin 0x0 rsrc/orig/fortune1.png -p rsrc_raw/fortune.pal
# ./tilemapdmp_gg bssmgg.gg 0x734B4 full 0x14 0x12 rsrc_raw/grp/fortune_main.bin 0x0 rsrc/orig/fortune2.png -p rsrc_raw/fortune.pal
# 
# # password/score screen
# ./bssm_decmp bssmgg.gg 0x1099D rsrc_raw/grp/password_score.bin
# ./tilemapdmp_gg bssmgg.gg 0x115FD full 0x14 0xA rsrc_raw/grp/password_score.bin 0x0 rsrc/orig/password1.png
# ./tilemapdmp_gg bssmgg.gg 0x1178F full 0x14 0x8 rsrc_raw/grp/password_score.bin 0x0 rsrc/orig/password2.png
# ./tilemapdmp_gg bssmgg.gg 0x118D1 full 0xA 0x2 rsrc_raw/grp/password_score.bin 0x0 rsrc/orig/password3.png
# ./tilemapdmp_gg bssmgg.gg 0x118FB full 0x6 0x2 rsrc_raw/grp/password_score.bin 0x0 rsrc/orig/password4.png
# 
# 
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/lunap_intro_chibiusa.bin 0x0 rsrc/orig/intro_lunap_chibiusa.png -p rsrc_raw/intro_lunap_chibiusa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/lunap_intro_usa.bin 0x0 rsrc/orig/intro_lunap_usa.png -p rsrc_raw/intro_lunap_usa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/findtuxedo_intro_chibiusa.bin 0x0 rsrc/orig/intro_findtuxedo_chibiusa.png -p rsrc_raw/intro_findtuxedo_chibiusa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/findtuxedo_intro_usa.bin 0x0 rsrc/orig/intro_findtuxedo_usa.png -p rsrc_raw/intro_findtuxedo_usa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/roulette_intro_chibiusa.bin 0x0 rsrc/orig/intro_roulette_chibiusa.png -p rsrc_raw/intro_roulette_chibiusa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/roulette_intro_usa.bin 0x0 rsrc/orig/intro_roulette_usa.png -p rsrc_raw/intro_roulette_usa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/quiz_intro_chibiusa.bin 0x0 rsrc/orig/intro_quiz_chibiusa.png -p rsrc_raw/intro_quiz_chibiusa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/quiz_intro_usa.bin 0x0 rsrc/orig/intro_quiz_usa.png -p rsrc_raw/intro_quiz_usa.pal
# ./tilemapdmp_gg bssmgg.gg 0x6BD21 full 0x14 0x12 rsrc_raw/grp/fortune_intro.bin 0x0 rsrc/orig/intro_fortune.png -p rsrc_raw/intro_fortune.pal
# ./tilemapdmp_gg bssmgg.gg 0x12EC9 full 0x14 0x12 rsrc_raw/grp/minigame_menu.bin 0x0 rsrc/orig/minigame_menu.png -p rsrc_raw/minigame_menu.pal
# 
# ./tilemapdmp_gg bssmgg.gg 0x7C9E6 full 0x14 0x12 rsrc_raw/grp/font_main.bin 0x100 rsrc/orig/soundtest.png
# 
# ./tilemapdmp_gg bssmgg.gg 0x4B506 full 0x14 0x12 rsrc_raw/grp/transform_usa-1.bin 0x0 rsrc/orig/transform_usa-1.png -p "rsrc_raw/transform_usa-1.pal"
# ./tilemapdmp_gg bssmgg.gg 0x4B818 full 0x14 0x12 rsrc_raw/grp/transform_usa-1.bin 0x0 rsrc/orig/transform_usa-2.png -p "rsrc_raw/transform_usa-1.pal"
# ./tilemapdmp_gg bssmgg.gg 0x4BAEA full 0x14 0x12 rsrc_raw/grp/transform_usa-2.bin 0x0 rsrc/orig/transform_usa-3.png -p "rsrc_raw/transform_usa-2.pal"
# ./tilemapdmp_gg bssmgg.gg 0x7C714 full 0x14 0x12 rsrc_raw/grp/transform_usa-2.bin 0x0 rsrc/orig/transform_usa-4.png -p "rsrc_raw/transform_usa-2.pal"
# 
# #rm -r -f rsrc_raw/rawdmp
# mkdir -p rsrc_raw/grp/rawdmp
# for file in rsrc_raw/grp/*.bin; do
#   ./grpdmp_gg "$file" "rsrc_raw/grp/rawdmp/$(basename $file .bin).png"
# done


