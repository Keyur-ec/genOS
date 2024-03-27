# genOS
bootloader and kernel for x86 arch

prerequisite:
1) first update the pakages -> sudo apt update
2) need nasm assambler -> sudo apt install nasm
3) need qemu emulator to run project -> sudo apt install qemu-system-x86

compilation steps:
1) go to genOS directory -> cd genOS
2) clean the project -> make clean
3) building a project -> ./build.sh
4) go to bin directory -> cd bin
5) run project -> qemu-system-i386 -hda ./os.bin
