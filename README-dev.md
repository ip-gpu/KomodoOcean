# Scribbles of madman
## dev notes

This file contains only **dev** notes. It's a some kind of notepad or "scribbles of madman" ...

- https://github.com/bitcoin/bitcoin/pull/3914/files - Build fully static versions of bitcoind and bitcoin-cli for older Linux distros
- ```./configure --enable-glibc-back-compat --prefix=$(pwd)/depends/x86_64-pc-linux-gnu LDFLAGS="-static-libstdc++"```
- https://bitcointalk.org/index.php?topic=1080289.0;all -  [HOWTO] compile altcoin for windows on linux using mxe and mingw


```
objdump -T src/qt/komodo-qt | grep 2.25
0000000000000000      DF *UND*	0000000000000000  GLIBC_2.25  __explicit_bzero_chk
0000000000000000  w   DF *UND*	0000000000000000  GLIBC_2.25  getentropy
```

- https://stackoverflow.com/questions/44488972/static-libstdc-works-on-g-but-not-on-pure-gcc
- https://thecharlatan.github.io/GLIBC-Back-Compat/ - Make binaries portable across linux distros

```
-Wl,--wrap=log2f

AX_CHECK_LINK_FLAG([[-Wl,--wrap=__divmoddi4]], [COMPAT_LDFLAGS="$COMPAT_LDFLAGS -Wl,--wrap=__divmoddi4"])
AX_CHECK_LINK_FLAG([[-Wl,--wrap=log2f]], [COMPAT_LDFLAGS="$COMPAT_LDFLAGS -Wl,--wrap=log2f"])
```

- https://drewdevault.com/2016/07/19/Using-Wl-wrap-for-mocking-in-C.html
