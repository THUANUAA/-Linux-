cmd_/home/interest/st_drive/hello/modules.order := {   echo /home/interest/st_drive/hello/helloworld.ko; :; } | awk '!x[$$0]++' - > /home/interest/st_drive/hello/modules.order
