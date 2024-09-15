cmd_/home/interest/st_drive/iio/mpu6050/modules.order := {   echo /home/interest/st_drive/iio/mpu6050/mpu6050.ko; :; } | awk '!x[$$0]++' - > /home/interest/st_drive/iio/mpu6050/modules.order
