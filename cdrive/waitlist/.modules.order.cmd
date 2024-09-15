cmd_/home/interest/st_drive/waitlist/modules.order := {   echo /home/interest/st_drive/waitlist/waitlist.ko; :; } | awk '!x[$$0]++' - > /home/interest/st_drive/waitlist/modules.order
