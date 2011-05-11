#!/bin/sh

mkdir DIST_FILES
cd DIST_FILES
svn co https://appset.svn.sourceforge.net/svnroot/appset "appset-qt-$1-sources"
cd appset-qt-$1-sources
rm -rf `find ./ -name .svn*`
rm -rf tests
mv dist ../
rm -rf appsetconf/appset/Fedora
rm -rf appsetconf/appset/Ubuntu
cd ..
tar czf "appset-qt-$1-sources.tar.gz" "appset-qt-$1-sources/"
rm -rf "appset-qt-$1-sources"
mv dist "appset-qt-$1-DIST"
MD5=`md5sum "appset-qt-$1-sources.tar.gz" | awk '{ print $1 }'`
sed -i "/^md5sums/{s/)/$MD5)/}" "appset-qt-$1-DIST/AppSet-Qt/Arch/PKGBUILD"
sed -i "/^source/{s/)/http:\/\/sourceforge.net\/projects\/appset\/files\/appset-qt\/$3\/$2\/appset-qt-$1-sources.tar.gz)/}" "appset-qt-$1-DIST/AppSet-Qt/Arch/PKGBUILD"
sed -i "/^pkgver/{s/=/=$1/}" "appset-qt-$1-DIST/AppSet-Qt/Arch/PKGBUILD"
if [ $4 == "Arch" ]; then
	cp "appset-qt-$1-sources.tar.gz" "appset-qt-$1-DIST/AppSet-Qt/Arch/"
	cd "appset-qt-$1-DIST/AppSet-Qt/Arch/"
	makepkg -s
	makepkg --source
fi
