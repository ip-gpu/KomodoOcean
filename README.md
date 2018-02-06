## KomodoOcean (komodo-qt) ##

![](https://github.com/DeckerSU/komodo-qt/raw/master/images/image00.png)

### Known Linux issues ###

Following dependencies are needed:

```
sudo apt-get install build-essential pkg-config libcurl3-gnutls-dev libc6-dev libevent-dev m4 g++-multilib autoconf libtool ncurses-dev unzip git python zlib1g-dev wget bsdmainutils automake libboost-all-dev libssl-dev libprotobuf-dev protobuf-compiler libqt4-dev libqrencode-dev libdb++-dev ntp ntpdate

sudo apt-get install libcurl4-gnutls-dev 
```

To build Qt wallet execute first:

```
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
```

Aslo, if you issued troubles with dependencies, may be this [doc](https://github.com/bitcoin/bitcoin/blob/master/doc/build-unix.md) will be useful.

**komodo-qt** currently linked dynamically with following dependencies:

```
	linux-vdso.so.1 =>  (0x00007fff06d60000)
	libQt5Network.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Network.so.5 (0x00007f801c4b5000)
	libQt5Widgets.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5 (0x00007f801a62e000)
	libQt5Gui.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Gui.so.5 (0x00007f801a0e6000)
	libQt5Core.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Core.so.5 (0x00007f8019c10000)
	libprotobuf.so.9 => /usr/lib/x86_64-linux-gnu/libprotobuf.so.9 (0x00007f80198f2000)
	libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f80196d5000)
	libcurl.so.4 => /usr/lib/x86_64-linux-gnu/libcurl.so.4 (0x00007f8019466000)
	libanl.so.1 => /lib/x86_64-linux-gnu/libanl.so.1 (0x00007f8019262000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f8018ee0000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f8018bd7000)
	libgomp.so.1 => /usr/lib/x86_64-linux-gnu/libgomp.so.1 (0x00007f80189b5000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f801879f000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f80183d5000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f801c41d000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f80181bb000)
	libproxy.so.1 => /usr/lib/x86_64-linux-gnu/libproxy.so.1 (0x00007f8017f9a000)
	libgobject-2.0.so.0 => /usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0 (0x00007f8017d47000)
	libglib-2.0.so.0 => /lib/x86_64-linux-gnu/libglib-2.0.so.0 (0x00007f8017a36000)
	libX11.so.6 => /usr/lib/x86_64-linux-gnu/libX11.so.6 (0x00007f80176fc000)
	libpng12.so.0 => /lib/x86_64-linux-gnu/libpng12.so.0 (0x00007f80174d7000)
	libharfbuzz.so.0 => /usr/lib/x86_64-linux-gnu/libharfbuzz.so.0 (0x00007f8017279000)
	libGL.so.1 => /usr/lib/nvidia-384/libGL.so.1 (0x00007f8016f37000)
	libicui18n.so.55 => /usr/lib/x86_64-linux-gnu/libicui18n.so.55 (0x00007f8016ad5000)
	libicuuc.so.55 => /usr/lib/x86_64-linux-gnu/libicuuc.so.55 (0x00007f8016741000)
	libpcre16.so.3 => /usr/lib/x86_64-linux-gnu/libpcre16.so.3 (0x00007f80164db000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f80162d7000)
	librt.so.1 => /lib/x86_64-linux-gnu/librt.so.1 (0x00007f80160cf000)
	libidn.so.11 => /usr/lib/x86_64-linux-gnu/libidn.so.11 (0x00007f8015e9c000)
	librtmp.so.1 => /usr/lib/x86_64-linux-gnu/librtmp.so.1 (0x00007f8015c80000)
	libssl.so.1.0.0 => /lib/x86_64-linux-gnu/libssl.so.1.0.0 (0x00007f8015a17000)
	libcrypto.so.1.0.0 => /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 (0x00007f80155d3000)
	libgssapi_krb5.so.2 => /usr/lib/x86_64-linux-gnu/libgssapi_krb5.so.2 (0x00007f8015389000)
	liblber-2.4.so.2 => /usr/lib/x86_64-linux-gnu/liblber-2.4.so.2 (0x00007f801517a000)
	libldap_r-2.4.so.2 => /usr/lib/x86_64-linux-gnu/libldap_r-2.4.so.2 (0x00007f8014f29000)
	libffi.so.6 => /usr/lib/x86_64-linux-gnu/libffi.so.6 (0x00007f8014d21000)
	libpcre.so.3 => /lib/x86_64-linux-gnu/libpcre.so.3 (0x00007f8014ab1000)
	libxcb.so.1 => /usr/lib/x86_64-linux-gnu/libxcb.so.1 (0x00007f801488f000)
	libfreetype.so.6 => /usr/lib/x86_64-linux-gnu/libfreetype.so.6 (0x00007f80145e5000)
	libgraphite2.so.3 => /usr/lib/x86_64-linux-gnu/libgraphite2.so.3 (0x00007f80143bf000)
	libnvidia-tls.so.384.111 => /usr/lib/nvidia-384/tls/libnvidia-tls.so.384.111 (0x00007f80141bb000)
	libnvidia-glcore.so.384.111 => /usr/lib/nvidia-384/libnvidia-glcore.so.384.111 (0x00007f80122fe000)
	libXext.so.6 => /usr/lib/x86_64-linux-gnu/libXext.so.6 (0x00007f80120ec000)
	libicudata.so.55 => /usr/lib/x86_64-linux-gnu/libicudata.so.55 (0x00007f8010635000)
	libgnutls.so.30 => /usr/lib/x86_64-linux-gnu/libgnutls.so.30 (0x00007f8010305000)
	libhogweed.so.4 => /usr/lib/x86_64-linux-gnu/libhogweed.so.4 (0x00007f80100d2000)
	libnettle.so.6 => /usr/lib/x86_64-linux-gnu/libnettle.so.6 (0x00007f800fe9c000)
	libgmp.so.10 => /usr/lib/x86_64-linux-gnu/libgmp.so.10 (0x00007f800fc1c000)
	libkrb5.so.3 => /usr/lib/x86_64-linux-gnu/libkrb5.so.3 (0x00007f800f94a000)
	libk5crypto.so.3 => /usr/lib/x86_64-linux-gnu/libk5crypto.so.3 (0x00007f800f71b000)
	libcom_err.so.2 => /lib/x86_64-linux-gnu/libcom_err.so.2 (0x00007f800f517000)
	libkrb5support.so.0 => /usr/lib/x86_64-linux-gnu/libkrb5support.so.0 (0x00007f800f30c000)
	libresolv.so.2 => /lib/x86_64-linux-gnu/libresolv.so.2 (0x00007f800f0f1000)
	libsasl2.so.2 => /usr/lib/x86_64-linux-gnu/libsasl2.so.2 (0x00007f800eed6000)
	libgssapi.so.3 => /usr/lib/x86_64-linux-gnu/libgssapi.so.3 (0x00007f800ec95000)
	libXau.so.6 => /usr/lib/x86_64-linux-gnu/libXau.so.6 (0x00007f800ea91000)
	libXdmcp.so.6 => /usr/lib/x86_64-linux-gnu/libXdmcp.so.6 (0x00007f800e88b000)
	libp11-kit.so.0 => /usr/lib/x86_64-linux-gnu/libp11-kit.so.0 (0x00007f800e627000)
	libtasn1.so.6 => /usr/lib/x86_64-linux-gnu/libtasn1.so.6 (0x00007f800e414000)
	libkeyutils.so.1 => /lib/x86_64-linux-gnu/libkeyutils.so.1 (0x00007f800e210000)
	libheimntlm.so.0 => /usr/lib/x86_64-linux-gnu/libheimntlm.so.0 (0x00007f800e007000)
	libkrb5.so.26 => /usr/lib/x86_64-linux-gnu/libkrb5.so.26 (0x00007f800dd7d000)
	libasn1.so.8 => /usr/lib/x86_64-linux-gnu/libasn1.so.8 (0x00007f800dadb000)
	libhcrypto.so.4 => /usr/lib/x86_64-linux-gnu/libhcrypto.so.4 (0x00007f800d8a8000)
	libroken.so.18 => /usr/lib/x86_64-linux-gnu/libroken.so.18 (0x00007f800d692000)
	libwind.so.0 => /usr/lib/x86_64-linux-gnu/libwind.so.0 (0x00007f800d469000)
	libheimbase.so.1 => /usr/lib/x86_64-linux-gnu/libheimbase.so.1 (0x00007f800d25a000)
	libhx509.so.5 => /usr/lib/x86_64-linux-gnu/libhx509.so.5 (0x00007f800d00f000)
	libsqlite3.so.0 => /usr/lib/x86_64-linux-gnu/libsqlite3.so.0 (0x00007f800cd3a000)
	libcrypt.so.1 => /lib/x86_64-linux-gnu/libcrypt.so.1 (0x00007f800cb02000)
```

But it uses OpenSSL 1.1.0d sources during build process. To run it you should [update](https://techlist.top/upgrade-openssl-version-1-1-0-ubuntu-server/)  your system OpenSSL binaries to 1.1.0d or run komodo-qt as ```komodo-qt -rootcertificates= &``` to avoid crash.

### Developers of Qt wallet ###

- Main developer: [@Ocean](https://komodo-platform.slack.com/team/U8BRG09EV)
- IT Expert / Sysengineer: [@Decker](https://komodo-platform.slack.com/messages/D5UHJMCJ3)