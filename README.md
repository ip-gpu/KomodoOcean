## KomodoOcean (komodo-qt) ##

![](./doc/images/komodo-qt-promo-10.png)

### How to build? ###

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

To build (Ubuntu 16.04):

```
cd ~
git clone https://github.com/DeckerSU/KomodoOcean.git
cd KomodoOcean
git checkout Linux
zcutil/build.sh -j$(nproc)
cd src/qt
./komodo-qt -rootcertificates= & # launch
```

To build (Ubuntu 18.x):

```
cd ~
git clone https://github.com/DeckerSU/KomodoOcean.git
cd KomodoOcean
git checkout Linux
zcutil/build.sh -j$(nproc)
cd src/qt
./komodo-qt & # launch (rootcertificates key don't needed)
```

If during build you get error like "fatal error: sodium.h: No such file or directory compilation terminated.", try to install libsodium-dev:

```
sudo apt install libsodium-dev
```

### Additional info ###

**komodo-qt** under Ubuntu 16.04 currently linked dynamically with following dependencies:

```
	linux-vdso.so.1 =>  (0x00007fff4aabc000)
	librt.so.1 => /lib/x86_64-linux-gnu/librt.so.1 (0x00007f352c5dc000)
	libQt5Network.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Network.so.5 (0x00007f352dee2000)
	libQt5Widgets.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5 (0x00007f352bf4f000)
	libQt5Gui.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Gui.so.5 (0x00007f352ba07000)
	libQt5Core.so.5 => /usr/lib/x86_64-linux-gnu/libQt5Core.so.5 (0x00007f352b531000)
	libprotobuf.so.9 => /usr/lib/x86_64-linux-gnu/libprotobuf.so.9 (0x00007f352b213000)
	libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f352aff6000)
	libcurl.so.4 => /usr/lib/x86_64-linux-gnu/libcurl.so.4 (0x00007f352ad87000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f352ab83000)
	libanl.so.1 => /lib/x86_64-linux-gnu/libanl.so.1 (0x00007f352a97f000)
	libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f352a5fd000)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f352a2f4000)
	libgomp.so.1 => /usr/lib/x86_64-linux-gnu/libgomp.so.1 (0x00007f352a0d2000)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f3529ebc000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f3529af2000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f352de4a000)
	libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x00007f35298d8000)
	libproxy.so.1 => /usr/lib/x86_64-linux-gnu/libproxy.so.1 (0x00007f35296b7000)
	libgobject-2.0.so.0 => /usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0 (0x00007f3529464000)
	libglib-2.0.so.0 => /lib/x86_64-linux-gnu/libglib-2.0.so.0 (0x00007f3529153000)
	libX11.so.6 => /usr/lib/x86_64-linux-gnu/libX11.so.6 (0x00007f3528e19000)
	libpng12.so.0 => /lib/x86_64-linux-gnu/libpng12.so.0 (0x00007f3528bf4000)
	libharfbuzz.so.0 => /usr/lib/x86_64-linux-gnu/libharfbuzz.so.0 (0x00007f3528996000)
	libGL.so.1 => /usr/lib/nvidia-384/libGL.so.1 (0x00007f3528654000)
	libicui18n.so.55 => /usr/lib/x86_64-linux-gnu/libicui18n.so.55 (0x00007f35281f2000)
	libicuuc.so.55 => /usr/lib/x86_64-linux-gnu/libicuuc.so.55 (0x00007f3527e5e000)
	libpcre16.so.3 => /usr/lib/x86_64-linux-gnu/libpcre16.so.3 (0x00007f3527bf8000)
	libidn.so.11 => /usr/lib/x86_64-linux-gnu/libidn.so.11 (0x00007f35279c5000)
	librtmp.so.1 => /usr/lib/x86_64-linux-gnu/librtmp.so.1 (0x00007f35277a9000)
	libssl.so.1.0.0 => /lib/x86_64-linux-gnu/libssl.so.1.0.0 (0x00007f3527540000)
	libcrypto.so.1.0.0 => /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 (0x00007f35270fb000)
	libgssapi_krb5.so.2 => /usr/lib/x86_64-linux-gnu/libgssapi_krb5.so.2 (0x00007f3526eb1000)
	liblber-2.4.so.2 => /usr/lib/x86_64-linux-gnu/liblber-2.4.so.2 (0x00007f3526ca2000)
	libldap_r-2.4.so.2 => /usr/lib/x86_64-linux-gnu/libldap_r-2.4.so.2 (0x00007f3526a51000)
	libffi.so.6 => /usr/lib/x86_64-linux-gnu/libffi.so.6 (0x00007f3526849000)
	libpcre.so.3 => /lib/x86_64-linux-gnu/libpcre.so.3 (0x00007f35265d9000)
	libxcb.so.1 => /usr/lib/x86_64-linux-gnu/libxcb.so.1 (0x00007f35263b7000)
	libfreetype.so.6 => /usr/lib/x86_64-linux-gnu/libfreetype.so.6 (0x00007f352610d000)
	libgraphite2.so.3 => /usr/lib/x86_64-linux-gnu/libgraphite2.so.3 (0x00007f3525ee7000)
	libnvidia-tls.so.384.130 => /usr/lib/nvidia-384/tls/libnvidia-tls.so.384.130 (0x00007f3525ce3000)
	libnvidia-glcore.so.384.130 => /usr/lib/nvidia-384/libnvidia-glcore.so.384.130 (0x00007f3523e27000)
	libXext.so.6 => /usr/lib/x86_64-linux-gnu/libXext.so.6 (0x00007f3523c15000)
	libicudata.so.55 => /usr/lib/x86_64-linux-gnu/libicudata.so.55 (0x00007f352215e000)
	libgnutls.so.30 => /usr/lib/x86_64-linux-gnu/libgnutls.so.30 (0x00007f3521e2e000)
	libhogweed.so.4 => /usr/lib/x86_64-linux-gnu/libhogweed.so.4 (0x00007f3521bfb000)
	libnettle.so.6 => /usr/lib/x86_64-linux-gnu/libnettle.so.6 (0x00007f35219c5000)
	libgmp.so.10 => /usr/lib/x86_64-linux-gnu/libgmp.so.10 (0x00007f3521745000)
	libkrb5.so.3 => /usr/lib/x86_64-linux-gnu/libkrb5.so.3 (0x00007f3521473000)
	libk5crypto.so.3 => /usr/lib/x86_64-linux-gnu/libk5crypto.so.3 (0x00007f3521244000)
	libcom_err.so.2 => /lib/x86_64-linux-gnu/libcom_err.so.2 (0x00007f3521040000)
	libkrb5support.so.0 => /usr/lib/x86_64-linux-gnu/libkrb5support.so.0 (0x00007f3520e35000)
	libresolv.so.2 => /lib/x86_64-linux-gnu/libresolv.so.2 (0x00007f3520c1a000)
	libsasl2.so.2 => /usr/lib/x86_64-linux-gnu/libsasl2.so.2 (0x00007f35209ff000)
	libgssapi.so.3 => /usr/lib/x86_64-linux-gnu/libgssapi.so.3 (0x00007f35207be000)
	libXau.so.6 => /usr/lib/x86_64-linux-gnu/libXau.so.6 (0x00007f35205ba000)
	libXdmcp.so.6 => /usr/lib/x86_64-linux-gnu/libXdmcp.so.6 (0x00007f35203b4000)
	libp11-kit.so.0 => /usr/lib/x86_64-linux-gnu/libp11-kit.so.0 (0x00007f3520150000)
	libtasn1.so.6 => /usr/lib/x86_64-linux-gnu/libtasn1.so.6 (0x00007f351ff3d000)
	libkeyutils.so.1 => /lib/x86_64-linux-gnu/libkeyutils.so.1 (0x00007f351fd39000)
	libheimntlm.so.0 => /usr/lib/x86_64-linux-gnu/libheimntlm.so.0 (0x00007f351fb30000)
	libkrb5.so.26 => /usr/lib/x86_64-linux-gnu/libkrb5.so.26 (0x00007f351f8a6000)
	libasn1.so.8 => /usr/lib/x86_64-linux-gnu/libasn1.so.8 (0x00007f351f604000)
	libhcrypto.so.4 => /usr/lib/x86_64-linux-gnu/libhcrypto.so.4 (0x00007f351f3d1000)
	libroken.so.18 => /usr/lib/x86_64-linux-gnu/libroken.so.18 (0x00007f351f1bb000)
	libwind.so.0 => /usr/lib/x86_64-linux-gnu/libwind.so.0 (0x00007f351ef92000)
	libheimbase.so.1 => /usr/lib/x86_64-linux-gnu/libheimbase.so.1 (0x00007f351ed83000)
	libhx509.so.5 => /usr/lib/x86_64-linux-gnu/libhx509.so.5 (0x00007f351eb38000)
	libsqlite3.so.0 => /usr/lib/x86_64-linux-gnu/libsqlite3.so.0 (0x00007f351e863000)
	libcrypt.so.1 => /lib/x86_64-linux-gnu/libcrypt.so.1 (0x00007f351e62b000)
	
```

`libssl.so.1.0.0` dependency exists here and that's the reason why we need to use `-rootcertificates=` key during launch. On Ubuntu 18.x it's don't needed bcz it uses OpenSSL 1.1.0 system-wide.

### Qt Versions Notes ###

By default Ubuntu 16.04 uses old version of Qt 5.5.1 (it's installed from by default from Xenial repos), if you want to change this behaviour and build against Qt 5.9.1 you'll need to do the following:

```
wget http://download.qt.io/official_releases/qt/5.9/5.9.1/qt-opensource-linux-x64-5.9.1.run
chmod +x qt-opensource-linux-x64-5.9.1.run
./qt-opensource-linux-x64-5.9.1.run # during installation leave install folder "AS IS", i.e. $HOME/Qt5.9.1
qtchooser -install -local qt5.9.1 $HOME/Qt5.9.1/5.9.1/gcc_64/bin/qmake 
# this will create $HOME/.config/qtchooser/qt5.9.1.conf 
...
git clone https://github.com/DeckerSU/KomodoOcean
cd KomodoOcean
git checkout Linux
git pull
QT_LDFLAGS="-Wl,-rpath=$HOME/Qt5.9.1/5.9.1/gcc_64/lib" PKG_CONFIG_PATH="$(pwd)/depends/x86_64-unknown-linux-gnu/lib/pkgconfig:$HOME/Qt5.9.1/5.9.1/gcc_64/lib/pkgconfig" zcutil/build.sh -j$(nproc) # override configure check for Qt package and build
./src/qt/komodo-qt -rootcertificates= & # for launch
```
And seems in this case you will need also build and link protobuf 2.6.1 package manually, or include it to `packages.mk` . 

So, recommended way is build system-installed Qt.

### Developers of Qt wallet ###

- Main developer: [@Ocean](https://komodo-platform.slack.com/team/U8BRG09EV)
- IT Expert / Sysengineer: [@Decker](https://komodo-platform.slack.com/messages/D5UHJMCJ3)