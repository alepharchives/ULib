include $(TOPDIR)/rules.mk

PKG_NAME:=nodog
PKG_VERSION:=1.1.0
PKG_RELEASE:=12

PKG_SOURCE:=ULib-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.unirel.com/ULib

PKG_BUILD_DIR:=$(BUILD_DIR)/ulib-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

STAMP_CONFIGURED:=$(STAMP_CONFIGURED)_$(call confvar, CONFIG_NODOG_DEBUG)

define Package/nodog
   TITLE:=nocat enhanced
   URL:=http://www.unirel.com/ULib
   DEPENDS:= +libstdcpp +libopenssl +libpcre +zlib $(if $(CONFIG_NODOG_UUID),+libuuid)
## DEPENDS:= +libstdcpp             +libpcre +zlib $(if $(CONFIG_NODOG_UUID),+libuuid) \
##																	$(if $(CONFIG_NODOG_OPENSSL),+libopenssl) $(if $(CONFIG_NODOG_CYASSL),+libcyassl) \
##   																+iptables-mod-ipopt +iptables-mod-nat +iptables-mod-nat-extra
endef

define Package/nodog/Description
  NoDog is a web capture system. It is similar to wireless access systems used in some coffee shops and airports.
  In commercial environments, web capture is used to authorise and pay for network use. Within SEPS, it's used for authentication purposes
  When run on a gateway/router on a network, all web requests are redirected until the client either logs in or clicks "I Accept" to an AUP.
  The gateway daemon then changes the firewall rules on the gateway to pass traffic for that client (based on IP address and MAC address).
endef

define Package/nodog/config
   source "$(SOURCE)/Config.in"
endef

define Package/nodog/conffiles
/etc/nodog.key
/etc/nodog.portal
/etc/uclient.conf
/etc/init.d/nodog
/usr/lib/nodog/firewall/nodog.fw
/usr/lib/nodog/firewall/tcrules.awk
endef

NODOG_OPTIONS:= --with-ssl --with-libz \
					 --without-expat --without-magic \
					 --disable-static --disable-new-ldflags --disable-zip --disable-LFS \
					 --disable-memory-pool --disable-gcc-optimized --disable-thread --disable-HCRS \
					 --enable-static-module="mod_proxy mod_nocat mod_http" \
					 $(if $(CONFIG_NODOG_UUID),--with-libuuid,--without-libuuid) $(if $(CONFIG_NODOG_DEBUG),--enable-debug)

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
	$(INSTALL_DIR) $(1)/etc $(1)/usr/sbin $(1)/usr/lib/ulib $(1)/usr/lib/nodog
##	$(CP) -a  $(PKG_INSTALL_DIR)/usr/lib/ulib/mod_proxy* $(1)/usr/lib/ulib
	$(CP) -a  $(PKG_INSTALL_DIR)/usr/lib/libulib* $(1)/usr/lib/
	$(CP)     $(PKG_INSTALL_DIR)/usr/bin/uclient $(PKG_INSTALL_DIR)/usr/bin/userver_tcp $(1)/usr/sbin/
	$(CP) -r  $(PKG_BUILD_DIR)/tests/examples/nocat/* \
				 $(PKG_BUILD_DIR)/tests/examples/nocat/.ht* $(1)/usr/lib/nodog
	$(CP) -r  $(1)/usr/lib/nodog/etc  						  $(1)/
	$(RM) -rf $(1)/usr/lib/nodog/etc
endef

$(eval $(call BuildPackage,nodog))
