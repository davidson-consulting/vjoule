mkdir .build
cd .build
cmake ..
make -j8
make install DESTDIR=/tmp/vjoule_deb
mkdir /tmp/vjoule_deb/DEBIAN
cp ../deb/control /tmp/vjoule_deb/DEBIAN/control
cp ../deb/postinst /tmp/vjoule_deb/DEBIAN/postinst
cp ../deb/postrm /tmp/vjoule_deb/DEBIAN/postrm
cp ../deb/prerm /tmp/vjoule_deb/DEBIAN/prerm
cp ../deb/preinst /tmp/vjoule_deb/DEBIAN/preinst
chmod 755 /tmp/vjoule_deb/DEBIAN/*

dpkg-deb -Zxz --build /tmp/vjoule_deb
mv /tmp/vjoule_deb.deb ./vjoule-tools_1.0.1.deb



