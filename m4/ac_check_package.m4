dnl @synopsis AC_CHECK_PACKAGE
dnl Macros that add compilation options to a `configure' script.
dnl AC_CHECK_PACKAGE

AC_DEFUN([AC_CHECK_PACKAGE],[
	AC_MSG_CHECKING(if LIBZ library is wanted)
	wanted=1;
	if test -z "$with_libz" ; then
		wanted=0;
		with_libz="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(libz, [  --with-libz             use system     LIBZ library - [[will check /usr /usr/local]] [[default=use if present]]], [
	if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local/; do
			libzdir="$dir"
			if test -f "$dir/include/zlib.h"; then
				found_libz="yes";
				break;
			fi
		done
		if test x_$found_libz != x_yes; then
			msg="Cannot find LIBZ library";
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			echo "${T_MD}LIBZ found in $libzdir${T_ME}"
dnl		printf "LIBZ found in $libzdir\n";
			USE_LIBZ=yes
			AC_DEFINE(USE_LIBZ, 1, [Define if enable libz support])
			libz_version=$(grep ZLIB_VERSION $libzdir/include/zlib.h | head -n1 | cut -d'"' -f2)
			if test -z "${libz_version}"; then
				libz_version="unknown"
			fi
			LIBS="-lz $LIBS";
			if test $libzdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$libzdir/include"
				LDFLAGS="$LDFLAGS -L$libzdir/lib -Wl,-R$libzdir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$libzdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBZ)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if you want to enable build of ZIP support)
	AC_ARG_ENABLE(zip,
				[  --enable-zip            enable build of ZIP support - require libz [[default: use if present libz]]])
	if test "$enable_zip" != "no" && test x_$found_libz = x_yes; then
		enable_zip="yes"
	else
		enable_zip="no"
	fi
	AC_MSG_RESULT([$enable_zip])

	AC_MSG_CHECKING(if you want to enable build of thread support)
	AC_ARG_ENABLE(thread,
				[  --enable-thread         enable build of thread support - require libpthread [[default: use if present libpthread]]])
	if test "$enable_thread" != "no" && test x_$found_libz = x_yes; then
		enable_thread="yes"
	else
		enable_thread="no"
	fi
	AC_MSG_RESULT([$enable_thread])

	AC_MSG_CHECKING(if libuuid library is wanted)
	wanted=1;
	if test -z "$with_libuuid" ; then
		wanted=0;
		with_libuuid="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(libuuid, [  --with-libuuid          use system  libuuid library - [[will check /usr /usr/local]] [[default=use if present]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			libuuiddir="$dir";
			if test -f "$dir/include/uuid/uuid.h"; then
				found_libuuid="yes";
				break;
			fi
		done
		if test x_$found_libuuid != x_yes; then
			msg="Cannot find LIBUUID library";
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			echo "${T_MD}libuuid found in $libuuiddir${T_ME}"
dnl		printf "libuuid found in $libuuiddir\n";
			USE_LIBUUID=yes
			AC_DEFINE(USE_LIBUUID, 1, [Define if enable libuuid support])
			libuuid_version=$(pkg-config --modversion ext2fs)
			if test -z "${libuuid_version}"; then
				libuuid_version="unknown"
			fi
			LIBS="-luuid $LIBS";
			if test $libuuiddir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$libuuiddir/include";
				LDFLAGS="$LDFLAGS -L$libuuiddir/lib -Wl,-R$libuuiddir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$libuuiddir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBUUID)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if MAGIC library is wanted)
	wanted=1;
	if test -z "$with_magic" ; then
		wanted=0;
		with_magic="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(magic, [  --with-magic            use system libmagic library - [[will check /usr /usr/local]] [[default=use if present]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			magicdir="$dir";
			if test -f "$dir/include/magic.h"; then
				found_magic="yes";
				break;
			fi
		done
		if test x_$found_magic != x_yes; then
			msg="Cannot find MAGIC library"
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			echo "${T_MD}libmagic found in $magicdir${T_ME}"
dnl		printf "libmagic found in $magicdir\n";
			USE_LIBMAGIC=yes
			AC_DEFINE(USE_LIBMAGIC, 1, [Define if enable libmagic support])
			libmagic_version=$($magicdir/bin/file --version 2>&1 | head -n 1 | cut -d'-' -f2)
			if test -z "${libmagic_version}"; then
				libmagic_version="unknown"
			fi
			LIBS="-lmagic $LIBS";
			if test $magicdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$magicdir/include";
				LDFLAGS="$LDFLAGS -L$magicdir/lib -Wl,-R$magicdir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$magicdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBMAGIC)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if SSL library is wanted)
	wanted=1;
	if test -z "$with_ssl" ; then
		wanted=0;
		with_ssl="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(ssl, [  --with-ssl              use system      SSL library - [[will check /usr /usr/local]] [[default=use if present]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			ssldir="$dir";
			if test -f "$dir/include/openssl/ssl.h"; then
				found_ssl="yes";
				break;
			fi
			if test -f "$dir/include/cyassl/openssl/ssl.h"; then
				found_ssl="yes";
				found_cyassl="yes";
				break;
			fi
		done
		if test x_$found_ssl != x_yes; then
			msg="Cannot find SSL library";
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			USE_LIBSSL=yes
			AC_DEFINE(USE_LIBSSL, 1, [Define if enable libssl support])
			if test "$found_cyassl" = "yes"; then
				echo "${T_MD}CYASSL found in $ssldir${T_ME}";
				ssl_version=$(grep VERSION $ssldir/include/cyassl/openssl/opensslv.h | cut -d' ' -f3 | tr -d '\r\n');
				LIBS="-lcyassl $LIBS";
			else
				echo "${T_MD}OPENSSL found in $ssldir${T_ME}";
				if test -f "$ssldir/include/openssl/ts.h"; then
					HAVE_SSL_TS="yes";
					AC_DEFINE(HAVE_SSL_TS, 1, [Define if we have time stamp support in openssl])
				fi
				ssl_version=$(pkg-config --modversion openssl);
dnl			ssl_version=$($ssldir/bin/openssl version | cut -d' ' -f2)
				LIBS="-lssl -lcrypto $LIBS";
			fi
			if test -z "${ssl_version}"; then
				ssl_version="unknown";
			fi
			if test $ssldir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$ssldir/include";
				LDFLAGS="$LDFLAGS -L$ssldir/lib -Wl,-R$ssldir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$ssldir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBSSL)
	AC_SUBST(HAVE_SSL_TS)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if PCRE library is wanted)
	wanted=1;
	if test -z "$with_pcre" ; then
		wanted=0;
		with_pcre="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(pcre, [  --with-pcre             use system     PCRE library - [[will check /usr /usr/local]] [[default=use if present]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local/; do
			pcredir="$dir"
			if test -f "$dir/include/pcre.h"; then
				found_pcre="yes";
				break;
			fi
		done
		if test x_$found_pcre != x_yes; then
			msg="Cannot find PCRE library";
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			echo "${T_MD}PCRE found in $pcredir${T_ME}"
dnl		printf "PCRE found in $pcredir\n";
			USE_LIBPCRE=yes
			AC_DEFINE(USE_LIBPCRE, 1, [Define if enable libpcre support])
			pcre_version=$($pcredir/bin/pcre-config --version)
			if test -z "${pcre_version}"; then
				pcre_version="unknown"
			fi
			LIBS="-lpcre $LIBS";
			if test $pcredir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$pcredir/include"
				LDFLAGS="$LDFLAGS -L$pcredir/lib -Wl,-R$pcredir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$pcredir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBPCRE)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if EXPAT library for XML parsing is wanted)
	wanted=1;
	if test -z "$with_expat" ; then
		wanted=0;
		with_expat="${CROSS_ENVIRONMENT}/usr";
	fi
	AC_ARG_WITH(expat, [  --with-expat            use system    EXPAT library - [[will check /usr /usr/local]] [[default=use if present]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local/; do
			expatdir="$dir"
			if test -f "$dir/include/expat.h"; then
				found_expat="yes";
				break;
			fi
		done
		if test x_$found_expat != x_yes; then
			msg="Cannot find EXPAT library"
			if test $wanted = 1; then
				AC_MSG_ERROR($msg)
			else
				AC_MSG_RESULT($msg)
			fi
		else
			echo "${T_MD}EXPAT found in $expatdir${T_ME}"
dnl		printf "EXPAT found in $expatdir\n";
			USE_LIBEXPAT=yes
			AC_DEFINE(USE_LIBEXPAT, 1, [Define if enable libexpat support])
			expat_version=$(strings $expatdir/lib*/libexpat.* | grep "^expat_[[0-9]]" | head -n1 | cut -d'_' -f2)
			if test -z "${expat_version}"; then
				expat_version="unknown"
			fi
			LIBS="-lexpat $LIBS";
			if test $expatdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$expatdir/include"
				LDFLAGS="$LDFLAGS -L$expatdir/lib -Wl,-R$expatdir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$expatdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBEXPAT)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if SSH library is wanted)
	AC_ARG_WITH(ssh, [  --with-ssh              use system      SSH library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			sshdir="$dir";
			if test -f "$dir/include/libssh/libssh.h"; then
				found_ssh="yes";
				break;
			fi
		done
		if test x_$found_ssh != x_yes; then
			AC_MSG_ERROR(Cannot find SSH library)
		else
			echo "${T_MD}libssh found in $sshdir${T_ME}"
dnl		printf "libSSH found in $sshdir\n";
			USE_LIBSSH=yes
			AC_DEFINE(USE_LIBSSH, 1, [Define if enable libssh support])
dnl		libssh_version=$(grep LIBSFTP_VERSION $sshdir/include/libssh/sftp.h | cut -d' ' -f3)
			libssh_version=$(strings $sshdir/lib*/libssh.so | grep 'libssh-[[0-9]]' | head -n1 | cut -d'-' -f4)
			if test -z "${libssh_version}"; then
				libssh_version="unknown"
			fi
			LIBS="-lssh $LIBS";
			if test $sshdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$sshdir/include";
				LDFLAGS="$LDFLAGS -L$sshdir/lib -Wl,-R$sshdir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$sshdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBSSH)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if cURL library is wanted)
	AC_ARG_WITH(curl, [  --with-curl             use system     cURL library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			curldir="$dir";
			if test -f "$dir/include/curl/curl.h"; then
				found_curl="yes";
				break;
			fi
		done
		if test x_$found_curl != x_yes; then
			AC_MSG_ERROR(Cannot find cURL library)
		else
			echo "${T_MD}libcurl found in $curldir${T_ME}"
dnl		printf "cURL found in $curldir\n";
			USE_LIBCURL=yes
			AC_DEFINE(USE_LIBCURL, 1, [Define if enable libcurL support])
			libcurl_version=$($curldir/bin/curl-config --version | sed -e "s/libcurl //g")
			if test -z "${libcurl_version}"; then
				libcurl_version="unknown"
			fi
			LIBS="-lcurl $LIBS";
			if test $curldir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$curldir/include";
				LDFLAGS="$LDFLAGS -L$curldir/lib -Wl,-R$curldir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$curldir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBCURL)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if LDAP library is wanted)
	AC_ARG_WITH(ldap, [  --with-ldap             use system openLDAP library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			ldapdir="$dir"
			if test -f "$dir/include/ldap.h"; then
				found_ldap="yes"
				LDAP_INCS="$ldapdir/include"
				LDAP_LDFLAGS="$ldapdir/lib"
				LDAP_LIBS="-lldap -llber"
				break
			fi
			if test -f "$dir/include/mozilla/ldap/ldap.h"; then
				found_ldap="yes"
				LDAP_INCS="$ldapdir/include/mozilla/ldap"
				LDAP_LDFLAGS="$ldapdir/lib/mozilla"
				LDAP_LIBS="-lldap50"
				break
			fi
			if test -f "$dir/include/winldap.h"; then
				found_ldap="yes"
				LDAP_INCS="$ldapdir/include"
				LDAP_LDFLAGS="$ldapdir/lib"
				LDAP_LIBS="-lwldap32"
				CPPFLAGS="$CPPFLAGS -DHAVE_WINLDAP_H -DHAVE_LDAP_SSL_H"
				break
			fi
		done
		if test x_$found_ldap != x_yes; then
			AC_MSG_ERROR(Cannot find LDAP include)
		else
			echo "${T_MD}LDAP found in $ldapdir${T_ME}"
dnl		printf "LDAP found in $ldapdir\n"
			USE_LIBLDAP=yes
			AC_DEFINE(USE_LIBLDAP, 1, [Define if enable libldap support])
			if test -f "$LDAP_INCS/ldap_ssl.h"; then
				CPPFLAGS="$CPPFLAGS -DHAVE_LDAP_SSL_H"
			fi
dnl		ldap_version=$(strings $LDAP_LDFLAGS/libldap.so | grep "@(#)")
dnl		ldap_version=$(ldapsearch -VV 2>&1 | tail -n1 | cut -d':' -f2 | cut -d')' -f1)
			ldap_version=$(grep LDAP_API_VERSION ${LDAP_INCS}/*ldap*.h | awk '{print $NF}')
			if test -z "${ldap_version}"; then
				ldap_version="unknown"
			fi
			LIBS="$LDAP_LIBS $LIBS"
			if test $ldapdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$LDAP_INCS"
				LDFLAGS="$LDFLAGS -L$LDAP_LDFLAGS -Wl,-R$LDAP_LDFLAGS"
				PRG_LDFLAGS="$PRG_LDFLAGS -L$ldapdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBLDAP)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if MySQL library is wanted)
	AC_ARG_WITH(mysql, [  --with-mysql            use system    MySQL library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			mysqldir="$dir";
			if test -f "$dir/include/mysql/mysql.h"; then
				found_mysql="yes";
				break;
			fi
		done
		if test x_$found_mysql != x_yes; then
			AC_MSG_ERROR(Cannot find MYSQL library)
		else
			echo "${T_MD}MySQL found in $mysqldir${T_ME}"
dnl		printf "MySQL found in $mysqldir\n";
			USE_LIBMYSQL=yes
			AC_DEFINE(USE_LIBMYSQL, 1, [Define if enable libmysql support])
			libmysql_version=$(grep MYSQL_VERSION_ID $mysqldir/include/mysql/mysql_version.h | cut -f3)
			if test -z "${libmysql_version}"; then
				libmysql_version="unknown"
			fi
			LIBS="-lmysqlclient $LIBS";
			if test $mysqldir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$mysqldir/include";
				LDFLAGS="$LDFLAGS -L$mysqldir/lib -Wl,-R$mysqldir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$mysqldir/lib";
			else
				if ! test -f $mysqldir/lib64/libmysqlclient.so && test -f $mysqldir/lib64/mysql/libmysqlclient.so; then
					LDFLAGS="$LDFLAGS -L$mysqldir/lib64/mysql -Wl,-R$mysqldir/lib64/mysql";
					PRG_LDFLAGS="$PRG_LDFLAGS -L$mysqldir/lib64/mysql";
				fi
			fi
		fi
	fi
	AC_SUBST(USE_LIBMYSQL)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if DBI library is wanted)
	AC_ARG_WITH(dbi, [  --with-dbi              use system      DBI library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			dbidir="$dir";
			if test -f "$dir/include/dbi/dbi.h"; then
				found_dbi="yes";
				break;
			fi
		done
		if test x_$found_dbi != x_yes; then
			AC_MSG_ERROR(Cannot find DBI library)
		else
			echo "${T_MD}DBI found in $dbidir${T_ME}"
dnl		printf "DBI found in $dbidir\n";
			USE_LIBDBI=yes
			AC_DEFINE(USE_LIBDBI, 1, [Define if enable libdbi support])
			libdbi_version=$(strings $dbidir/lib*/libdbi.* | grep "^libdbi v[[0-9]]" | cut -d'v' -f2 | head -n1)
			if test -z "${libdbi_version}"; then
				libdbi_version="unknown"
			fi
			LIBS="-ldbi $LIBS";
			if test $dbidir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$dbidir/include";
				LDFLAGS="$LDFLAGS -L$dbidir/lib -Wl,-R$dbidir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$dbidir/lib";
			else
				if ! test -f $dbidir/lib64/libdbi.so && test -f $dbidir/lib64/dbi/libdbi.so; then
					LDFLAGS="$LDFLAGS -L$dbidir/lib64/dbi -Wl,-R$dbidir/lib64/dbi";
					PRG_LDFLAGS="$PRG_LDFLAGS -L$dbidir/lib64/dbi";
				fi
			fi
		fi
	fi
	AC_SUBST(USE_LIBDBI)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if libevent library is wanted)
	AC_ARG_WITH(libevent, [  --with-libevent         use system libevent library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			libeventdir="$dir";
			if test -f "$dir/include/event.h"; then
				found_libevent="yes";
				break;
			fi
		done
		if test x_$found_libevent != x_yes; then
			AC_MSG_ERROR(Cannot find LIBEVENT library)
		else
			echo "${T_MD}libevent found in $libeventdir${T_ME}"
dnl		printf "libevent found in $libeventdir\n";
			USE_LIBEVENT=yes
			AC_DEFINE(USE_LIBEVENT, 1, [Define if enable libevent support])
			libevent_version=$(strings $libeventdir/lib*/libevent* | grep "^libevent-[[0-9]]" | head -n1 | cut -d'-' -f2 | awk -F'.so' '{n=1; print $n}')
			if test -z "${libevent_version}"; then
				libevent_version="unknown"
			fi
			LIBS="-levent $LIBS";
			if test $libeventdir != "${CROSS_ENVIRONMENT}/usr"; then
				CPPFLAGS="$CPPFLAGS -I$libeventdir/include";
				LDFLAGS="$LDFLAGS -L$libeventdir/lib -Wl,-R$libeventdir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$libeventdir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBEVENT)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if libxml2 library is wanted)
	AC_ARG_WITH(libxml2, [  --with-libxml2          use system  libxml2 library - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			libxml2dir="$dir";
			if test -f "$dir/include/libxml2/libxml/valid.h"; then
				found_libxml2="yes";
				break;
			fi
		done
		if test x_$found_libxml2 != x_yes; then
			AC_MSG_ERROR(Cannot find LIBXML2 library)
		else
			echo "${T_MD}libxml2 found in $libxml2dir${T_ME}"
dnl		printf "libxml2 found in $libxml2dir\n";
			USE_LIBXML2=yes
			AC_DEFINE(USE_LIBXML2, 1, [Define if enable libxml2 support])
			libxml2_version=$(pkg-config --modversion libxml-2.0)
			if test -z "${libxml2_version}"; then
				libxml2_version="unknown"
			fi
			LIBS="-lxml2 $LIBS";
			CPPFLAGS="$CPPFLAGS -I$libxml2dir/include/libxml2";
			if test $libxml2dir != "${CROSS_ENVIRONMENT}/usr"; then
				LDFLAGS="$LDFLAGS -L$libxml2dir/lib -Wl,-R$libxml2dir/lib";
				PRG_LDFLAGS="$PRG_LDFLAGS -L$libxml2dir/lib";
			fi
		fi
	fi
	AC_SUBST(USE_LIBXML2)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if you want to use page-speed SDK)
	AC_ARG_WITH(page-speed, [  --with-page-speed       use google page-speed SDK   - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			page_speeddir="$dir";
			if test -d $dir/page-speed-*; then
				found_page_speed="yes";
				break;
			fi
		done
		if test x_$found_page_speed != x_yes; then
			AC_MSG_ERROR("Cannot find page-speed SDK");
		else
			echo "${T_MD}page-speed SDK found in $page_speeddir${T_ME}"
			USE_PAGE_SPEED=yes
			AC_DEFINE(USE_PAGE_SPEED, 1, [Define if enable libpagespeed support])
			page_speed_version=$(ls -1 $page_speeddir | grep page-speed | cut -d'-' -f3)
			PAGESPEED_ROOT_DIR=$page_speeddir/page-speed-$page_speed_version
			AC_SUBST(PAGESPEED_ROOT_DIR)
		fi
	fi
	AC_SUBST(USE_PAGE_SPEED)],
	[AC_MSG_RESULT(no)])

	AC_MSG_CHECKING(if you want to use V8 JavaScript Engine)
	AC_ARG_WITH(v8-javascript, [  --with-v8-javascript    use V8 JavaScript Engine    - [[will check /usr /usr/local]]],
	[if test "$withval" = "no"; then
		AC_MSG_RESULT(no)
	else
		AC_MSG_RESULT(yes)
		for dir in $withval ${CROSS_ENVIRONMENT}/usr ${CROSS_ENVIRONMENT}/usr/local; do
			v8dir="$dir";
			if test -f "$dir/include/v8.h"; then
				found_v8="yes";
				break;
			fi
		done
		if test x_$found_v8 != x_yes; then
			AC_MSG_ERROR("Cannot find V8 JavaScript Engine");
		else
			echo "${T_MD}V8 JavaScript Engine found in $v8dir${T_ME}"
			USE_LIBV8=yes
			AC_DEFINE(USE_LIBV8, 1, [Define if enable libv8 support])
			v8_version=$(strings $v8dir/lib*/libv8.so | grep -i 'libv8' | head -n1 | cut -b10-)
		fi
	fi
	AC_SUBST(USE_LIBV8)],
	[AC_MSG_RESULT(no)])
])
