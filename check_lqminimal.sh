#!/bin/bash
echo "#define QT_STATICPLUGIN 1" >>confdefs.h
  cat confdefs.h - <<_ACEOF >conftest.cpp
/* end confdefs.h.  */

    #define QT_STATICPLUGIN
    #include <QtPlugin>
    Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin)
int
main ()
{
return 0;
  ;
  return 0;
}
_ACEOF

# Bitcoin:
# -I/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/include/QtNetwork 
# -I/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/include/QtWidgets 
# -I/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/include/QtGui 
# -I/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/include/QtCore 
# -I/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/share/../include/  
# -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS 
# -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/share/../lib  conftest.cpp -lqminimal 
# -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib 
# -L//home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib 
# -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib 
# -L//home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib 
# -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib 
# -lQt5XcbQpa -lQt5ServiceSupport -lQt5ThemeSupport -lQt5DBus -lQt5EventDispatcherSupport -lQt5FontDatabaseSupport -lfontconfig -lfreetype -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -lpthread -lX11-xcb -lX11 -lxcb-static -lxcb -ldl -lQt5ServiceSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5ThemeSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5DBus -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -lQt5EventDispatcherSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5FontDatabaseSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lfontconfig -lfreetype -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib -lX11-xcb -lX11 -lpthread -lxcb -lXau -lQt5FbSupport -lQt5AccessibilitySupport -lQt5DeviceDiscoverySupport -lQt5ThemeSupport -lQt5EventDispatcherSupport -lQt5FontDatabaseSupport -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib -lQt5Network -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lz -lQt5Widgets -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/share/../plugins/platforms

#g++ -m64 -o conftest  -std=c++11 -pipe -O1   -Wformat -Wformat-security -Wstack-protector -fstack-protector-all -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtNetwork -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtWidgets -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtGui -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtCore -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/share/../include/  -DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -L/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/share/../lib   -Wl,-z,relro -Wl,-z,now -pie conftest.cpp -lqminimal -L/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/lib -lQt5Network -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lz -lQt5Widgets -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -lanl

# -lQt5XcbQpa -lQt5ServiceSupport -lQt5ThemeSupport -lQt5DBus -lQt5EventDispatcherSupport -lQt5FontDatabaseSupport -lfontconfig -lfreetype -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -lpthread -lX11-xcb -lX11 -lxcb-static -lxcb -ldl -lQt5ServiceSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5ThemeSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5DBus -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -lQt5EventDispatcherSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5FontDatabaseSupport -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lfontconfig -lfreetype -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib -lX11-xcb -lX11 -lpthread -lxcb -lXau -lQt5FbSupport -lQt5AccessibilitySupport -lQt5DeviceDiscoverySupport -lQt5ThemeSupport -lQt5EventDispatcherSupport -lQt5FontDatabaseSupport -L/home/decker/bitcoin/depends/x86_64-pc-linux-gnu/lib -lQt5Network -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lz -lQt5Widgets -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 \
# -lQt5Network -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lz -lQt5Widgets -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -lanl \

g++ -m64 -std=c++11 -o conftest -fPIC -pipe -O2 \
-I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtNetwork -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtWidgets \
-I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtGui -I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/include/QtCore \
-I/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/share/../include/ \
-DHAVE_BUILD_INFO -D__STDC_FORMAT_MACROS  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 \
-L/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/share/../lib \
-Wl,-z,relro -Wl,-z,now -pie conftest.cpp -lqminimal \
-lQt5EventDispatcherSupport \
-lQt5Network -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lz -lQt5Widgets -lQt5Gui -lqtlibpng -lqtharfbuzz -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lQt5Gui -lQt5Core -lm -lz -lqtpcre2 -ldl -lpthread -lqtlibpng -lqtharfbuzz -lz -lQt5Core -lpthread -lm -lz -lqtpcre2 -ldl -lanl \
-L/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/lib \
-L/home/decker/KomodoOcean/depends/x86_64-unknown-linux-gnu/plugins/platforms 