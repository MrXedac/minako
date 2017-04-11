target remote localhost:1234
symbol-file build/x86_multiboot/meso.bin
add-symbol-file linux/vmlinux 0xC0800000
directory linux/

b pageMapHook
commands
	silent
	printf "Map page %x into %x:%x\n",src,targetPartition,target
	continue
end

b pip_pageFault
commands
    silent
    printf "Minako Linux caught Page Fault at address %x\n",cr2
    continue
end

b do_page_fault
commands
    silent
    printf "Calling Linux PF handler\n"
    continue
end
