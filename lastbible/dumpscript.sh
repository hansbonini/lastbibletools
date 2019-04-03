mkdir -p script/orig

make libsms && make lastbible_scriptdmp
./lastbible_scriptdmp lastbible.gg script/orig/
