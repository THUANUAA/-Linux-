cmd_/home/interest/st_drive/platform_drive/dht11/Module.symvers := sed 's/\.ko$$/\.o/' /home/interest/st_drive/platform_drive/dht11/modules.order | scripts/mod/modpost -m -a  -o /home/interest/st_drive/platform_drive/dht11/Module.symvers -e -i Module.symvers   -T -
