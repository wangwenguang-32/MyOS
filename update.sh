#!/bin/bash
# 简化版挂载.img脚本
# 用法：sudo ./mount_img.sh
 
# 配置参数（修改为你的实际路径）
IMG_FILE="./disk.img"  # 镜像文件路径
MOUNT_DIR="./hdc"          # 固定挂载目录
PARTITION_NUM="1"                   # 默认挂载第一个分区
 
# 检查root权限
if [ "$(id -u)" -ne 0 ]; then
    echo "错误：请使用sudo或root用户运行此脚本"
    exit 1
fi
 
# 检查文件是否存在
if [ ! -f "$IMG_FILE" ]; then
    echo "错误：镜像文件 $IMG_FILE 不存在"
    exit 1
fi
 
# 创建挂载目录（如果不存在）
mkdir -p "$MOUNT_DIR"
 
# 查找可用loop设备
LOOP_DEV=$(losetup -f)
if [ -z "$LOOP_DEV" ]; then
    echo "错误：没有可用的loop设备"
    exit 1
fi
 
# 关联镜像到loop设备
echo "关联 $IMG_FILE 到 $LOOP_DEV"
losetup -P "$LOOP_DEV" "$IMG_FILE"
 
# 构造分区设备名
PART_DEV="${LOOP_DEV}p${PARTITION_NUM}"
 
# 检查分区是否存在
if ! fdisk -l "$LOOP_DEV" | grep -q "${LOOP_DEV}p${PARTITION_NUM}"; then
    echo "错误：分区 ${LOOP_DEV}p${PARTITION_NUM} 不存在"
    losetup -d "$LOOP_DEV"
    exit 1
fi
 
# 挂载分区
echo "挂载 $PART_DEV 到 $MOUNT_DIR"
mount "$PART_DEV" "$MOUNT_DIR"
 
echo -e "\n挂载成功！"
cp  kernel.bin ./hdc/boot/
echo "sudo umount $MOUNT_DIR && sudo losetup -d $LOOP_DEV"
sudo umount $MOUNT_DIR && sudo losetup -d $LOOP_DEV
