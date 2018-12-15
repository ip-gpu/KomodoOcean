## KomodoOcean (komodo-qt) ##

![](./images/image00.png)

This is KomodoOcean / komodo-qt GUI qt-based wallet for Komodo (KMD). Probably (not sure) is a **world first Qt-Wallet** for ZCash based cryptocurrency forks. Now it available only for Windows (64-bit) and soon for Linux (64-bit) platforms.

Latest version already have [sapling](https://komodoplatform.com/sapling-unifying-blockchain-technology/) support from Dec 15th, 2018 and release **g27**.

**NB!** This repo have two branches:

- *master* for Windows
- *Linux* for Linux

![](./images/image01.png)

### System requirements ###

- Windows 8 / 8.1 / 10 (64-bit) based PC.
- At least 4 Gb of RAM (8 or 16 Gb RAM recommended).
- About 3.1 Gb of storage space to sync full blockchain (SSD drive is recommended)

Before you start create the following configuration file, named `komodo.conf` in %APPDATA%\Komodo folder:

    rpcuser=bitcoinrpc
    rpcpassword=password
    txindex=1
    addnode=5.9.102.210
    addnode=78.47.196.146
    addnode=178.63.69.164
    addnode=88.198.65.74
    addnode=5.9.122.241
    addnode=144.76.94.38
    addnode=89.248.166.91

And download following files in %APPDATA%\ZcashParams folder: 

- [sprout-proving.key](https://z.cash/downloads/sprout-proving.key)
- [sprout-verifying.key](https://z.cash/downloads/sprout-verifying.key)
- [sapling-spend.params](https://z.cash/downloads/sapling-spend.params)
- [sapling-output.params](https://z.cash/downloads/sapling-output.params)
- [sprout-groth16.params](https://z.cash/downloads/sprout-groth16.params)

Also you can use `fetch-params.cmd` batch file in `release` folder to download all needed automatically.

If you already have installed Agama Wallet - you already have all needed. 

For more information about Komodo Platform, please visit official site: [https://komodoplatform.com/](https://komodoplatform.com/) . 

Komodo-Qt (KomodoOcean) wallet also have official channel in Komodo Discord - [#wallet-ocean-qt](https://discord.gg/U5WWaJR). If you have any questions or just want to be a part of community, please [join](https://discord.gg/U5WWaJR).

### Project Status ###

Currenly komodo-qt is *under developement*. Use it at your own risk - do not use for mining or merchant applications, also we don't recommend now to use it with your main wallet. Before use you can copy your main wallet.dat in a secure place or use `-datadir` command-line key to select other data folder for use with komodo-qt.

![](./images/image02.png)

![](./images/image03.png)

Features:

- Native QT wallet written in C/C++ and Rust (librustzcash library).
- Receiving and sending coins.
- Extended coin control features, selecting "Inputs..." (UTXOs) when sending coins.
- Supporting assetchains (launch KomodoOceanGUI.exe with needed `-ac_name`, `-ac_supply` and `-addnode` command-line keys to launch wallet on needed assetchain or use existing batch files in repo).
- **Sapling support**.


Existing issues:

- You tell us ... ;)

### How to build? ###

System requirements:

- OS Windows 7 / 8 / 8.1 / 10 (**64-bit**)
- [Git for Windows](https://git-scm.com/download/win)
- Microsoft Visual Studio 2015 Version 14.0.25431.01 Update 3
- CMake 3.10.2 ([cmake-3.10.2-win64-x64.msi](https://cmake.org/files/v3.10/cmake-3.10.2-win64-x64.msi)), add it to system path for all users during install.
- Qt 5.9.2 ([qt-opensource-windows-x86-5.9.2.exe](https://download.qt.io/official_releases/qt/5.9/5.9.2/qt-opensource-windows-x86-5.9.2.exe))
- [Far Manager v3.0 build 5100 x64](https://www.farmanager.com/download.php?l=en) (not necessary, but very convenient to use command line and execute commands)

**1.** Download and install all needed software. Qt should be installed into it's default path `C:\Qt\Qt5.9.2` . 
**1a.** Don't forget to select `msvc2015 64-bit` component during install:

![](./images/install_02.png)

**2.** Clone sources repository using `git clone https://github.com/ip-gpu/KomodoOcean` or `git clone https://github.com/DeckerSU/KomodoOcean` .
**3.** Make sure that you are on Windows (master) branch `git checkout master` .
**4.** Unpack the content of `KomodoOcean\depends\depends.zip` archive in `KomodoOcean\depends` folder:

![](./images/install_01.png)

**5.** Launch Qt Creator and choose Open Project. Navigate to `KomodoOcean` directory and open `KomodoOceanGUI.pro` project file.
**5a.** Make sure that Build directory in Projects -> Build Settings corresponds to your project path (for example `C:\Distr\KomodoOcean`, this is a folder in which we clone repo).

![](./images/install_03.png)

**6.** Select Build -> Build Project "KomodoOceanGUI" from Top Menu or press Ctrl-B to start build.

**7.** After build is finished you will see ```KomodoOceanGUI.exe``` in ```release``` folder.

Congratulations! You built Komodo-Qt (KomodoOcean) for Windows. Now you can run it. 

### Dependencies ###

This repo has prebuilt version (`rustzcash.lib`) of [librustzcash](https://github.com/zcash/librustzcash) library. If you want to build it by yourself, follow next steps:

**1.** [Install](https://www.rust-lang.org/tools/install) Rust language for Windows, you'll need [x86_64-pc-windows-msvc](https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe) version. 

**2.** Add `%USERPROFILE%\.cargo\bin` in `%PATH%` environment variable after install.

**3.** If you have https -> ssh substitution in your `.gitconfig`, like :

	[url "ssh://git@github.com/"]
	insteadOf = https://github.com/

You'll need to add following lines in `.cargo/config` file:

    [net]
    git-fetch-with-cli = true
    
To get rust work correctly with ssh connection, otherwise you will have "error authenticating: failed connecting agent; class=Ssh (23)" and "attempted ssh-agent authentication, but none of the usernames `git` succeeded" errors, during download rust deps.

If you don't set-up any substitutions https -> ssh for git earlier, no additional actions required.

**4.** Clone librustzcash repo and build library:

    git clone https://github.com/zcash/librustzcash
    cd librustzcash
    cargo build # to build debug version
	cargo build --release  # to build release version

![](./images/install_04.png)

Your `*.lib` will be available in `target/(release/debug)` folders relatively.

### Developers of Qt wallet ###

- Main developer: [Ocean](https://github.com/ip-gpu)
- IT Expert / Sysengineer: [Decker](https://deckersu.github.io/) 