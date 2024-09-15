cmd_/home/interest/st_drive/cdrive/gpio_led/modules.order := {   echo /home/interest/st_drive/cdrive/gpio_led/led.ko; :; } | awk '!x[$$0]++' - > /home/interest/st_drive/cdrive/gpio_led/modules.order
