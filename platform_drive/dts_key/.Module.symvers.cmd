cmd_/home/interest/st_drive/platform_drive/dts_key/Module.symvers := sed 's/\.ko$$/\.o/' /home/interest/st_drive/platform_drive/dts_key/modules.order | scripts/mod/modpost -m -a  -o /home/interest/st_drive/platform_drive/dts_key/Module.symvers -e -i Module.symvers   -T -
