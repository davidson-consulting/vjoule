mkdir .build
cd .build
cmake ..
make -j8
make install DESTDIR=/tmp/anon_deb
mkdir /tmp/anon_deb/DEBIAN
cp ../deb/control /tmp/anon_deb/DEBIAN/control
cp ../deb/postinst /tmp/anon_deb/DEBIAN/postinst
cp ../deb/postrm /tmp/anon_deb/DEBIAN/postrm
cp ../deb/prerm /tmp/anon_deb/DEBIAN/prerm
cp ../deb/preinst /tmp/anon_deb/DEBIAN/preinst
chmod 755 /tmp/anon_deb/DEBIAN/*

dpkg-deb -Zxz --build /tmp/anon_deb
mv /tmp/anon_deb.deb ./anon-tools_1.0.1.deb



