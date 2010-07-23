#!/bin/bash

 PACKAGE=$HOME/rpmbuild			# fedora
#PACKAGE=/usr/src/packages		# SUSE

VERSION=1.0.5

run_build() {

	mv ULib-$VERSION $1-$VERSION
	tar czf $PACKAGE/SOURCES/$1-$VERSION.tar.gz $1-$VERSION

	cp $1-$VERSION/examples/$2/$3.spec $PACKAGE/SPECS/

	rpmbuild -ba --clean --rmsource $1-$VERSION/examples/$2/$3.spec >>rpm.log 2>&1

	mv $1-$VERSION ULib-$VERSION
}

ULIB_TO_REMOVE=`ls -d ULib-*`

if [ -n "$ULIB_TO_REMOVE" ]; then
	for i in "$ULIB_TO_REMOVE"; do
		if [ "$i" != "ULib-$VERSION.tar.gz" ]; then
			rm -rf $i
		fi
	done
fi

tar xzf ULib-$VERSION.tar.gz
cd ULib-$VERSION

#------------------------------------
# SUSE
#------------------------------------
# mv rpmpkgreq.lst.suse rpmpkgreq.lst
#------------------------------------

./configure -C --enable-static --enable-shared

cd ..
tar czf ULib-$VERSION.tar.gz ULib-$VERSION

cp ULib-$VERSION.tar.gz $PACKAGE/SOURCES/

cp ULib-$VERSION/ULib.spec				$PACKAGE/SPECS/
cp ULib-$VERSION/examples/*/*.start $PACKAGE/SOURCES/

rpmbuild -ba --clean --rmsource ULib-$VERSION/ULib.spec >rpm.log 2>&1

run_build lcsp					lcsp			lcsp
run_build lcsp_rpc			lcsp			lcsp_rpc
run_build cspserver			userver		cspserver
run_build cspclient			csp			cspclient
run_build cspclient_rpc		csp			cspclient_rpc
run_build rsignserver		userver		rsignserver
run_build rsignclient		rsign			rsignclient
run_build rsignclient_rpc	rsign			rsignclient_rpc
run_build tsaserver			userver		tsaserver
run_build web_server			userver		web_server
run_build wagsmserver		userver		wagsmserver
run_build searchengine-bin IR				searchengine-bin
run_build xml2txt				xml2txt		xml2txt
run_build workflow			workflow		workflow
run_build PEC_log				PEC_log		PEC_log
run_build doc_parse			doc_parse	doc_parse
