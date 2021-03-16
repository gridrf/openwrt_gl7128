#
# Copyright (C) 2008-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk


PKG_NAME:=lora-station
PKG_VERSION:=2021-03-06
PKG_RELEASE:=1

PKG_LICENSE:=LGPL-2.1
PKG_LICENSE_FILES:=LICENSE

PKG_MAINTAINER:=billue jeams <gsj0791@126.com>

include $(INCLUDE_DIR)/package.mk

# set to 1 to enable debugging
DEBUG=

define Package/lora-station
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=lora station
  DEPENDS:=+libwebsockets-openssl +libstdcpp +libpthread +libjson-c
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(TOOLCHAIN_DIR)/lib -L$(STAGING_DIR)/usr/lib


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/lora-station/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/lora-station $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/lora-station.conf $(1)/etc/config/lora-station
	$(INSTALL_DIR) $(1)/usr/share/websocket
	$(INSTALL_DATA) ./files/index.html $(1)/usr/share/websocket/index.html
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lora-station.init $(1)/etc/init.d/lora
endef

$(eval $(call BuildPackage,lora-station))
