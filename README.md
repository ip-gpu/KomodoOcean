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
**5a.** Change Build directory in Projects -> Build Settings to your project path (for example `C:\Distr\KomodoOcean`, this is a folder in which we clone repo).
  