TEMPLATE = app
TARGET = KomodoOceanGUI
VERSION = 0.1.0.0

INCLUDEPATH += src src\qt src\libsnark src\secp256k1 src\secp256k1\include src\leveldb\include src\leveldb\helpers\memenv src\leveldb src\univalue\include src\libevent\include src\libevent\compat

MINIUPNPC_INCLUDE_PATH = 

windows:INCLUDEPATH += D:\libgmp_6.1.1_msvc14\include
windows:INCLUDEPATH += D:\BDB_6.2.32\include D:\db-6.2.23\build_windows
windows:INCLUDEPATH += D:\libsodium-1.0.15-msvc\include
windows:INCLUDEPATH += D:\boost_1_65_1 D:\boost_1_65_1\boost
windows:INCLUDEPATH += D:\pthreads-master
windows:INCLUDEPATH += D:\openssl-1.1.0f-vs2015\include64 D:\openssl\crypto

windows:BOOST_LIB_PATH = D:\boost_1_65_1\lib64-msvc-14.0
windows:OPENSSL_LIB_PATH = D:\openssl-1.1.0f-vs2015\lib64

QT_VERSION = 0x050902
QT += network widgets

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0 ENABLE_WALLET ENABLE_MINING
DEFINES += BOOST_THREAD_USE_LIB BOOST_HAS_PTHREADS
DEFINES += _TIMESPEC_DEFINED BINARY_OUTPUT MONTGOMERY_OUTPUT NO_PT_COMPRESSION
DEFINES += MULTICORE NO_PROCPS NO_GTEST NO_DOCS STATIC NO_SUPERCOP
DEFINES += _AMD64_ _MT __STDC_FORMAT_MACROS __amd64__ __x86_64__ HAVE_WORKING_BOOST_SLEEP_FOR
DEFINES += HAVE_CONFIG_H HAVE_DECL_STRNLEN
DEFINES += CURVE_ALT_BN128 _REENTRANT __USE_MINGW_ANSI_STDIO=1 LEVELDB_ATOMIC_PRESENT

windows:DEFINES += _WINDOWS 
windows:DEFINES +=  OS_WIN LEVELDB_PLATFORM_WINDOWS

CONFIG += no_include_pwd thread no_batch

MINGW_THREAD_BUGFIX = 0

Release:DESTDIR = release
Release:OBJECTS_DIR = build_release
Release:MOC_DIR = build_release
Release:RCC_DIR = build_release
Release:UI_DIR = build_release

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = build_debug
Debug:MOC_DIR = build_debug
Debug:RCC_DIR = build_debug
Debug:UI_DIR = build_debug

# Mac: compile for maximum compatibility (10.5, 32-bit)
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -arch x86_64 -isysroot /Developer/SDKs/MacOSX10.5.sdk

!windows:!macx {
     # Linux: static link
     LIBS += -Wl,-Bstatic
#     LIBS += -Wl,-Bdynamic
}

!windows {
# for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
QMAKE_CXXFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
QMAKE_LFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
}

# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP MINIUPNP_STATICLIB STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH

    #SOURCES += src\miniupnpc\connecthostport.c \
    #src\miniupnpc\igd_desc_parse.c \
    #src\miniupnpc\minisoap.c \
    #src\miniupnpc\minissdpc.c \
    #src\miniupnpc\miniupnpc.c \
    #src\miniupnpc\miniwget.c \
    #src\miniupnpc\minixml.c \
    #src\miniupnpc\portlistingparse.c \
    #src\miniupnpc\upnpcommands.c \
    #src\miniupnpc\upnpdev.c \
    #src\miniupnpc\upnperrors.c \
    #src\miniupnpc\upnpreplyparse.c

    windows:LIBS += -liphlpapi
}

# use: qmake "USE_DBUS=1" or qmake "USE_DBUS=0"
linux:count(USE_DBUS, 0) {
    USE_DBUS=1
}

contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}

windows {
    # make an educated guess about what the ranlib command is called
    isEmpty(QMAKE_RANLIB) {
        QMAKE_RANLIB = $$replace(QMAKE_STRIP, strip, ranlib)
    }
    LIBS += -lshlwapi
}

# regenerate build.h
!windows|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
    message(Building O3 optimization flag)
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS += -O3
    QMAKE_CFLAGS += -O3
}

*-g++-32 {
    message("32 platform, adding -msse2 flag")

    QMAKE_CXXFLAGS += -msse2
    QMAKE_CFLAGS += -msse2
}

linux:QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wno-ignored-qualifiers -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector

# Input
DEPENDPATH += .
HEADERS += src\komodo_globals.h \
     src\qt\komodooceangui.h \
     src\qt\transactiontablemodel.h \
     src\qt\addresstablemodel.h \
     src\qt\optionsdialog.h \
     src\qt\coincontroldialog.h \
     src\qt\coincontroltreewidget.h \
     src\qt\sendcoinsdialog.h \
     src\qt\addressbookpage.h \
     src\qt\signverifymessagedialog.h \
     src\qt\editaddressdialog.h \
     src\qt\komodoaddressvalidator.h \
     src\qt\clientmodel.h \
     src\qt\guiutil.h \
     src\qt\optionsmodel.h \
     src\qt\trafficgraphwidget.h \
     src\qt\transactiondesc.h \
     src\qt\transactiondescdialog.h \
     src\qt\komodoamountfield.h \
     src\qt\transactionfilterproxy.h \
     src\qt\transactionview.h \
     src\qt\walletmodel.h \
     src\qt\overviewpage.h \
     src\qt\csvmodelwriter.h \
     src\qt\sendcoinsentry.h \
     src\qt\qvalidatedlineedit.h \
     src\qt\komodounits.h \
     src\qt\qvaluecombobox.h \
     src\qt\askpassphrasedialog.h \
     src\qt\intro.h \
     src\qt\splashscreen.h \
     src\qt\utilitydialog.h \
     src\qt\notificator.h \
     src\qt\modaloverlay.h \
     src\qt\openuridialog.h \
     src\qt\walletframe.h \
     src\qt\paymentserver.h \
     src\qt\rpcconsole.h \
     src\qt\bantablemodel.h \
     src\qt\peertablemodel.h \
     src\qt\recentrequeststablemodel.h \
     src\qt\walletview.h \
     src\qt\receivecoinsdialog.h \
     src\qt\receiverequestdialog.h \
     src\streams.h \
     src\komodo_komodod.h \
     src\komodo_utils.h

SOURCES += src\addrdb.cpp \
    src\addrman.cpp \
    src\alert.cpp \
    src\amount.cpp \
    src\arith_uint256.cpp \
    src\asyncrpcoperation.cpp \
    src\asyncrpcqueue.cpp \
    src\base58.cpp \
    src\bech32.cpp \
    src\bloomfilter.cpp \
    src\chain.cpp \
    src\chainparamsbase.cpp \
    src\checkpoints.cpp \
    src\clientversion.cpp \
    src\coins.cpp \
    src\compat\glibc_sanity.cpp \
    src\compat\glibcxx_sanity.cpp \
    src\compressor.cpp \
    src\core_read.cpp \
    src\core_write.cpp \
    src\crypto\equihash.cpp \
    src\crypto\hmac_sha256.cpp \
    src\crypto\hmac_sha512.cpp \
    src\crypto\ripemd160.cpp \
    src\crypto\sha1.cpp \
    src\crypto\sha256.cpp \
    src\crypto\sha512.cpp \
    src\eccryptoverify.cpp \
    src\ecwrapper.cpp \
    src\fs.cpp \
    src\hash_komodo.cpp \
    src\httprpc.cpp \
    src\httpserver.cpp \
    src\init.cpp \
    src\key.cpp \
    src\keystore.cpp \
    src\leveldb\db\builder.cc \
    src\leveldb\db\db_impl.cc \
    src\leveldb\db\db_iter.cc \
    src\leveldb\db\dbformat.cc \
    src\leveldb\db\filename.cc \
    src\leveldb\db\log_reader.cc \
    src\leveldb\db\log_writer.cc \
    src\leveldb\db\memtable.cc \
    src\leveldb\db\table_cache.cc \
    src\leveldb\db\version_edit.cc \
    src\leveldb\db\version_set.cc \
    src\leveldb\db\write_batch.cc \
    src\leveldb\helpers\memenv\memenv.cc \
    src\leveldb\port\port_posix_sse.cc \
    src\leveldb\port\port_win.cc \
    src\leveldb\table\block.cc \
    src\leveldb\table\block_builder.cc \
    src\leveldb\table\filter_block.cc \
    src\leveldb\table\format.cc \
    src\leveldb\table\iterator.cc \
    src\leveldb\table\merger.cc \
    src\leveldb\table\table.cc \
    src\leveldb\table\table_builder.cc \
    src\leveldb\table\two_level_iterator.cc \
    src\leveldb\util\arena_leveldb.cc \
    src\leveldb\util\bloom.cc \
    src\leveldb\util\cache.cc \
    src\leveldb\util\coding.cc \
    src\leveldb\util\comparator.cc \
    src\leveldb\util\crc32c.cc \
    src\leveldb\util\env.cc \
    src\leveldb\util\env_win.cc \
    src\leveldb\util\filter_policy.cc \
    src\leveldb\util\hash.cc \
    src\leveldb\util\logging.cc \
    src\leveldb\util\options.cc \
    src\leveldb\util\status_leveldb.cc \
    src\leveldbwrapper.cpp \
    src\libevent\buffer.c \
    src\libevent\buffer_iocp.c \
    src\libevent\bufferevent.c \
    src\libevent\bufferevent_async.c \
    src\libevent\bufferevent_ratelim.c \
    src\libevent\bufferevent_sock.c \
    src\libevent\event.c \
    src\libevent\event_iocp.c \
    src\libevent\evmap.c \
    src\libevent\evthread.c \
    src\libevent\evthread_win32.c \
    src\libevent\evutil.c \
    src\libevent\evutil_rand.c \
    src\libevent\evutil_time.c \
    src\libevent\http.c \
    src\libevent\listener.c \
    src\libevent\log.c \
    src\libevent\signal.c \
    src\libevent\strlcpy.c \
    src\libevent\win32select.c \
    src\libsnark\algebra\curves\alt_bn128\alt_bn128_g1.cpp \
    src\libsnark\algebra\curves\alt_bn128\alt_bn128_g2.cpp \
    src\libsnark\algebra\curves\alt_bn128\alt_bn128_init.cpp \
    src\libsnark\algebra\curves\alt_bn128\alt_bn128_pairing.cpp \
    src\libsnark\algebra\curves\alt_bn128\alt_bn128_pp.cpp \
    src\libsnark\common\profiling.cpp \
    src\libsnark\common\utils.cpp \
    src\main.cpp \
    src\merkleblock.cpp \
    src\metrics.cpp \
    src\miner.cpp \
    src\net.cpp \
    src\netbase.cpp \
    src\policy\fees.cpp \
    src\policy\policy.cpp \
    src\pow.cpp \
    src\primitives\block_komodo.cpp \
    src\primitives\transaction.cpp \
    src\protocol.cpp \
    src\pubkey.cpp \
    src\qt\addressbookpage.cpp \
    src\qt\addresstablemodel.cpp \
    src\qt\askpassphrasedialog.cpp \
    src\qt\bantablemodel.cpp \
    src\qt\clientmodel.cpp \
    src\qt\coincontroldialog.cpp \
    src\qt\coincontroltreewidget.cpp \
    src\qt\csvmodelwriter.cpp \
    src\qt\editaddressdialog.cpp \
    src\qt\guiutil.cpp \
    src\qt\intro.cpp \
    src\qt\komodo.cpp \
    src\qt\komodoaddressvalidator.cpp \
    src\qt\komodoamountfield.cpp \
    src\qt\komodooceangui.cpp \
    src\qt\komodounits.cpp \
    src\qt\modaloverlay.cpp \
    src\qt\networkstyle.cpp \
    src\qt\notificator.cpp \
    src\qt\openuridialog.cpp \
    src\qt\optionsdialog.cpp \
    src\qt\optionsmodel.cpp \
    src\qt\overviewpage.cpp \
    src\qt\paymentrequest.pb.cc \
    src\qt\paymentrequestplus.cpp \
    src\qt\paymentserver.cpp \
    src\qt\peertablemodel.cpp \
    src\qt\platformstyle.cpp \
    src\qt\qvalidatedlineedit.cpp \
    src\qt\qvaluecombobox.cpp \
    src\qt\receivecoinsdialog.cpp \
    src\qt\receiverequestdialog.cpp \
    src\qt\recentrequeststablemodel.cpp \
    src\qt\rpcconsole.cpp \
    src\qt\sendcoinsdialog.cpp \
    src\qt\sendcoinsentry.cpp \
    src\qt\signverifymessagedialog.cpp \
    src\qt\splashscreen.cpp \
    src\qt\trafficgraphwidget.cpp \
    src\qt\transactiondesc.cpp \
    src\qt\transactiondescdialog.cpp \
    src\qt\transactionfilterproxy.cpp \
    src\qt\transactionrecord.cpp \
    src\qt\transactiontablemodel.cpp \
    src\qt\transactionview.cpp \
    src\qt\utilitydialog.cpp \
    src\qt\walletframe.cpp \
    src\qt\walletmodel.cpp \
    src\qt\walletmodeltransaction.cpp \
    src\qt\walletview.cpp \
    src\qt\winshutdownmonitor.cpp \
    src\random.cpp \
    src\rest.cpp \
    src\rpcblockchain.cpp \
    src\rpcclient.cpp \
    src\rpcmining.cpp \
    src\rpcmisc.cpp \
    src\rpcnet.cpp \
    src\rpcprotocol.cpp \
    src\rpcrawtransaction.cpp \
    src\rpcserver.cpp \
    src\scheduler.cpp \
    src\script\interpreter.cpp \
    src\script\script.cpp \
    src\script\script_error.cpp \
    src\script\sigcache.cpp \
    src\script\sign.cpp \
    src\script\standard.cpp \
    src\secp256k1\src\secp256k1.c \
    src\sendalert.cpp \
    src\support\cleanse.cpp \
    src\support\pagelocker.cpp \
    src\sync.cpp \
    src\sys\time.cpp \
    src\threadinterrupt.cpp \
    src\timedata.cpp \
    src\torcontrol.cpp \
    src\txdb.cpp \
    src\txmempool.cpp \
    src\uint256.cpp \
    src\univalue\lib\univalue.cpp \
    src\univalue\lib\univalue_read.cpp \
    src\univalue\lib\univalue_write.cpp \
    src\util.cpp \
    src\utilmoneystr.cpp \
    src\utilstrencodings.cpp \
    src\utiltest.cpp \
    src\utiltime.cpp \
    src\validationinterface.cpp \
    src\versionbits.cpp \
    src\wallet\asyncrpcoperation_sendmany.cpp \
    src\wallet\crypter.cpp \
    src\wallet\db.cpp \
    src\wallet\rpcdump.cpp \
    src\wallet\rpcwallet.cpp \
    src\wallet\wallet.cpp \
    src\wallet\wallet_fees.cpp \
    src\wallet\wallet_ismine.cpp \
    src\wallet\walletdb.cpp \
    src\zcash\Address.cpp \
    src\zcash\IncrementalMerkleTree.cpp \
    src\zcash\JoinSplit.cpp \
    src\zcash\Note.cpp \
    src\zcash\NoteEncryption.cpp \
    src\zcash\prf.cpp \
    src\zcash\Proof.cpp \
    src\zcash\util_zcash.cpp \
    src\zcbenchmarks.cpp \
    src\chainparams.cpp

RESOURCES += \
    src\qt\komodo.qrc \
    src\qt\komodo_locale.qrc

FORMS += \
    src\qt\forms\coincontroldialog.ui \
    src\qt\forms\sendcoinsdialog.ui \
    src\qt\forms\addressbookpage.ui \
    src\qt\forms\signverifymessagedialog.ui \
    src\qt\forms\editaddressdialog.ui \
    src\qt\forms\transactiondescdialog.ui \
    src\qt\forms\overviewpage.ui \
    src\qt\forms\sendcoinsentry.ui \
    src\qt\forms\askpassphrasedialog.ui \
    src\qt\forms\debugwindow.ui \
    src\qt\forms\helpmessagedialog.ui \
    src\qt\forms\intro.ui \
    src\qt\forms\modaloverlay.ui \
    src\qt\forms\openuridialog.ui \
    src\qt\forms\receivecoinsdialog.ui \
    src\qt\forms\receiverequestdialog.ui \
    src\qt\forms\optionsdialog.ui

CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to komodo.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/komodo_*.ts)

isEmpty(QMAKE_LRELEASE) {
    windows:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
    doc/*.rst doc/*.txt doc/README README.md src/qt/res/komodo-qt-res.rc

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_LIB_SUFFIX) {
    macx:BOOST_LIB_SUFFIX = -mt
    Release:windows:BOOST_LIB_SUFFIX = -vc140-mt-1_65_1
    Debug:windows:BOOST_LIB_SUFFIX = -vc140-mt-gd-1_65_1
}

isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    windows:BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
    else:BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

windows:RC_FILE = src/qt/res/komodo-qt-res.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

macx:HEADERS += macdockiconhandler.h macnotificationhandler.h
macx:OBJECTIVE_SOURCES += macdockiconhandler.mm macnotificationhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0
macx:ICON = src/qt/res/icons/komodo.icns
macx:TARGET = "KomodoOceanGUI"
macx:QMAKE_CFLAGS_THREAD += -pthread
macx:QMAKE_CXXFLAGS_THREAD += -pthread
macx:QMAKE_LFLAGS_THREAD += -pthread

# Set libraries and includes at end, to use platform-defined defaults if not overridden
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,)
LIBS += $$BDB_LIB_SUFFIX
windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32 -luser32
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX
windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX

Release:LIBS += -lD:\libgmp_6.1.1_msvc14\lib\x64\gmp
Release:LIBS += -lD:\libsodium-1.0.15-msvc\x64\Release\v140\dynamic\libsodium
Release:LIBS += -lD:\libcurl-master\lib\dll-release-x64\libcurl
Release:LIBS += -lD:\db-6.2.23\build_windows\x64\Release\libdb62
Release:LIBS += -llibcryptoMD -llibsslMD
Release:LIBS += -lD:\pthreads-master\dll\x64\Release\pthreads

Debug:LIBS += -lD:\libgmp_6.1.1_msvc14\lib\x64\gmp
Debug:LIBS += -lD:\libsodium-1.0.15-msvc\x64\Debug\v140\dynamic\libsodium
Debug:LIBS += -lD:\libcurl-master\lib\dll-debug-x64\libcurl_debug
Debug:LIBS += -lD:\db-6.2.23\build_windows\x64\Debug\libdb62d
Debug:LIBS += -llibcryptoMDd -llibsslMDd
Debug:LIBS += -lD:\pthreads-master\dll\x64\Debug\pthreads

!windows:!macx {
    DEFINES += LINUX
    LIBS += -lrt -ldl
}

QMAKE_CXXFLAGS += -O2 
QMAKE_CFLAGS += -O2 

system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
