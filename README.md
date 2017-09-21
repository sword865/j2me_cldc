# j2me_cldc


find libgcc_s:
    sudo /sbin/ldconfig -p | grep libgcc
    link it to /usr/lib/x86_64-linux-gnu/libgcc_s.so
export path:
    export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu