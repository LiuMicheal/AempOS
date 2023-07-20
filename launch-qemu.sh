# run qemu, without using gdb
# added by xw, 18/6

# qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
# -monitor stdio -smp 2

#modified by mingxuan 2019-3-13
qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
-monitor stdio -smp 4