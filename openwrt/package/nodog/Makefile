include $(TOPDIR)/rules.mk

PKG_NAME:=nodog
PKG_VERSION:=1.0.5
PKG_RELEASE:=9

PKG_SOURCE:=ULib-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.unirel.com/ULib
PKG_MD5SUM:=33ac5f89ba40a3c6fe80b134ff968095

PKG_BUILD_DIR:=$(BUILD_DIR)/ULib-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
STAMP_CONFIGURED:=$(STAMP_CONFIGURED)_$(call confvar, CONFIG_NODOG_DEBUG)

define Package/nodog
   TITLE:=nocat enhanced
   URL:=http://www.unirel.com/ULib
   DEPENDS:= +libstdcpp +libopenssl +zlib $(if $(CONFIG_NODOG_UUID),+libuuid)

endef

define Package/nodog/Description
   NoDog is a web capture system. It is similar to wireless access systems used in some coffee shops and airports.
	In commercial environments, web capture is used to authorise and pay for network use. Within SEPS, it's used for authentication purposes
endef

define Package/nodog/config
   source "$(SOURCE)/Config.in"
endef

#define Package/nodog/conffiles
#/etc/nodog.cnf
#endef

NODOG_OPTIONS:= --disable-static --enable-final --with-ssl --with-libz --with-libuuid $(if $(CONFIG_NODOG_DEBUG),--enable-debug)

define Build/Configure
	$(call Build/Configure/Default, \
		--prefix=$(PKG_INSTALL_DIR)/usr \
		$(NODOG_OPTIONS))
endef

define Build/Compile
	rm -rf $(PKG_INSTALL_DIR)
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		AR="$(TARGET_CROSS)ar r" \
		RANLIB="$(TARGET_CROSS)ranlib"
endef

define Package/nodog/install
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) DESTDIR="$(PKG_INSTALL_DIR)" install
	$(INSTALL_DIR) $(1)/etc $(1)/usr/sbin $(1)/usr/libexec/ulib $(1)/usr/lib/nodog
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libulib* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/ulib/mod_http* $(PKG_INSTALL_DIR)/usr/lib/ulib/mod_nocat* $(1)/usr/libexec/ulib/
	$(CP) $(PKG_BUILD_DIR)/examples/userver/.libs/userver_tcp $(PKG_BUILD_DIR)/examples/uclient/.libs/uclient $(1)/usr/sbin
	$(CP) -r $(PKG_BUILD_DIR)/tests/examples/nocat/* 	 $(1)/usr/lib/nodog
	$(CP) -r $(PKG_BUILD_DIR)/tests/examples/nocat/.ht* $(1)/usr/lib/nodog
	$(CP) -r  $(1)/usr/lib/nodog/etc  						 $(1)/
	$(RM) -rf $(1)/usr/lib/nodog/etc
endef

$(eval $(call BuildPackage,nodog))
