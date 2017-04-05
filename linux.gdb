target remote localhost:1234
symbol-file build/x86_multiboot/meso.bin
add-symbol-file linux/vmlinux 0xC0800000
directory linux/

b pip_iret
commands
    # stfu gdb
    silent 
    printf "%x\n",iret_esp
    continue
end

b pageMapHook
commands
	silent
	printf "Map page %x into %x:%x\n",src,targetPartition,target
	continue
end

