cmd_/home/interest/st_drive/hello/Module.symvers := sed 's/\.ko$$/\.o/' /home/interest/st_drive/hello/modules.order | scripts/mod/modpost -m -a  -o /home/interest/st_drive/hello/Module.symvers -e -i Module.symvers   -T -
