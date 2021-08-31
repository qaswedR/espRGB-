cd C:\tools\sming\samples\Basic_rBoot & make clean & make
copy C:\tools\sming\samples\Basic_rBoot\out\firmware\rboot.bin \\192.168.0.4\xampp\htdocs\rboot.bin /y
copy C:\tools\sming\samples\Basic_rBoot\out\firmware\rom0.bin \\192.168.0.4\xampp\htdocs\rom0.bin /y
copy C:\tools\sming\samples\Basic_rBoot\out\firmware\spiff_rom.bin \\192.168.0.4\xampp\htdocs\spiff_rom.bin /y
pause 10