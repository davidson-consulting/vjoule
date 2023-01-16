# create the deb file for the formula/sensor etc

mkdir .build
cd .build
cmake ..
make -j8
make install DESTDIR=/tmp/vjoule
mkdir /tmp/vjoule/DEBIAN/
cp ../deb/control /tmp/vjoule/DEBIAN/control
cp ../deb/postinst /tmp/vjoule/DEBIAN/postinst
cp ../deb/postrm /tmp/vjoule/DEBIAN/postrm
cp ../deb/prerm /tmp/vjoule/DEBIAN/prerm
chmod 755 /tmp/vjoule/DEBIAN/postinst
chmod 755 /tmp/vjoule/DEBIAN/postrm
chmod 755 /tmp/vjoule/DEBIAN/prerm

dpkg-deb -Zxz --build /tmp/vjoule
mv /tmp/vjoule.deb ./vjoule_0.1.deb

