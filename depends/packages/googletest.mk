package=googletest
$(package)_version=1.8.1
$(package)_download_path=https://github.com/google/$(package)/archive/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_download_file=release-$($(package)_version).tar.gz
$(package)_sha256_hash=9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c

define $(package)_set_vars
$(package)_cxxflags+=-std=c++11
$(package)_cxxflags_linux=-fPIC
endef

ifeq ($(build_os),darwin)
define $(package)_set_vars
    $(package)_build_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)" CXX="$($(package)_cxx)" CXXFLAGS="$($(package)_cxxflags)"
endef
endif

ifeq ($(build_os),darwin)
$(package)_install=ginstall
define $(package)_build_cmds
    $(MAKE) -C googlemock/make gmock.a && \
    $(MAKE) -C googletest/make gtest.a
endef
else
$(package)_install=install

ifeq ($(build_os)$(host_os),linuxdarwin) # cross-compile for MacOS from Linux
define $(package)_build_cmds
  $(MAKE) -C googlemock/make CC="$(build_prefix)/bin/$($(package)_cc)" CXX="$(build_prefix)/bin/$($(package)_cxx)" AR="$(build_prefix)/bin/$($(package)_ar)" CXXFLAGS="$($(package)_cxxflags)" gmock.a && \
  $(MAKE) -C googletest/make CC="$(build_prefix)/bin/$($(package)_cc)" CXX="$(build_prefix)/bin/$($(package)_cxx)" AR="$(build_prefix)/bin/$($(package)_ar)" CXXFLAGS="$($(package)_cxxflags)" gtest.a
endef
else
define $(package)_build_cmds
  $(MAKE) -C googlemock/make CC="$($(package)_cc)" CXX="$($(package)_cxx)" AR="$($(package)_ar)" CXXFLAGS="$($(package)_cxxflags)" gmock.a && \
  $(MAKE) -C googletest/make CC="$($(package)_cc)" CXX="$($(package)_cxx)" AR="$($(package)_ar)" CXXFLAGS="$($(package)_cxxflags)" gtest.a
endef
endif
endif

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_dir)$(host_prefix)/lib && \
  install ./googlemock/make/gmock.a $($(package)_staging_dir)$(host_prefix)/lib/libgmock.a && \
  install ./googletest/make/gtest.a $($(package)_staging_dir)$(host_prefix)/lib/libgtest.a && \
  cp -a ./googlemock/include $($(package)_staging_dir)$(host_prefix)/ && \
  cp -a ./googletest/include $($(package)_staging_dir)$(host_prefix)/
endef
