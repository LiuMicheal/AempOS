# run qemu, with gdb support
# added by xw, 18/6

# gnome-terminal -x bash -c "echo 'type in gdb: target remote :1234';echo '';gdb -s kernel.gdb.bin" &
# qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
# -gdb tcp::1234 -S -monitor stdio -smp 2

# modified by mingxuan 2019-3-13
gnome-terminal -x bash -c "echo 'type in gdb: target remote :1234';echo '';gdb -s kernel.gdb.bin" &
qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
-gdb tcp::1234 -S -monitor stdio -smp 4