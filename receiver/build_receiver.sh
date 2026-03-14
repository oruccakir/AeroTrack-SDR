#!/bin/bash
source /home/cakir/Xilinx/2025.1/Vitis/settings64.sh
# Varsa PetaLinux ortam değişkenlerini (SYSROOT_PATH vs) dışarıdan alıyoruz, yoksa standart aarch64-linux-gnu-g++ kullanacağız.
CXX=${CXX:-aarch64-linux-gnu-g++}

echo "--> Compiling ZCU102 Receiver application..."

# Dizinler
SRC_DIR="src"
INC_DIR="include"
OUT_DIR="build_zcu102"
OUTPUT_BIN="${OUT_DIR}/aerotrack_receiver"

mkdir -p ${OUT_DIR}

# Eğer SYSROOT tanımlıysa sysroot bayrağını ekleyelim
SYSROOT_FLAG=""
if [ -n "${SYSROOT_PATH}" ]; then
    SYSROOT_FLAG="--sysroot=${SYSROOT_PATH}"
fi

# Gerekli kaynak dosyalar
HOST_SRCS=(
    "${SRC_DIR}/main.cpp"
    "${SRC_DIR}/udp_receiver.cpp"
)

# Derleme işlemi
${CXX} ${SYSROOT_FLAG} \
    -O3 -std=c++17 -Wall -Wextra \
    -I${INC_DIR} \
    "${HOST_SRCS[@]}" \
    -o "${OUTPUT_BIN}" \
    -lpthread

if [ $? -eq 0 ]; then
    echo "✔ Başarılı! Çıktı: receiver/${OUTPUT_BIN}"
    echo "ZCU102'ye göndermek için şu komutu çalıştırabilirsiniz:"
    echo "scp receiver/${OUTPUT_BIN} petalinux@10.5.146.20:/home/petalinux/"
else
    echo "✖ Derleme hatası."
    exit 1
fi