#/bin/bash
echo "Cleaning project..."

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

echo -e "\nBuilding project..."
make all

echo -e "\nBuild Success."
