..\Ignore\cc65\bin\ca65 Firmware.asm -o firmware.o --listing Firmware.lst --list-bytes 255 
..\Ignore\cc65\bin\ld65 -t none firmware.o -o Firmware.bin 
..\Ignore\cc65\bin\ca65 flash.asm -o flash.o --listing flash.lst --list-bytes 255 
..\Ignore\cc65\bin\ld65 -t none flash.o -o flash.bin 
..\Ignore\cc65\bin\ca65 allvols.asm -o allvols.o --listing allvols.lst --list-bytes 255 
..\Ignore\cc65\bin\ld65 -t none allvols.o -o allvols.bin 
..\Ignore\cc65\bin\ca65 bootpg.asm -o bootpg.o --listing bootpg.lst --list-bytes 255 
..\Ignore\cc65\bin\ld65 -t none bootpg.o -o bootpg.bin
echo const uint8_t bootblocks[] PROGMEM = { > bootpg.h 
c:\msys64\usr\bin\xxd.exe -i < bootpg.bin >> bootpg.h
echo }; >> bootpg.h 
copy /b flash.bin+Firmware.bin flash.system
