cmd_/home/interest/st_drive/platform_drive/timer/modules.order := {   echo /home/interest/st_drive/platform_drive/timer/driver.ko; :; } | awk '!x[$$0]++' - > /home/interest/st_drive/platform_drive/timer/modules.order
