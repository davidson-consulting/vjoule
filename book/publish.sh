URL="git@github.com:davidson-consulting/vjoule.git"
echo $#

if [ $# -lt 1 ]
then 
    echo "usage message"
else
    echo "Publishing to gh-pages at $URL"
    SRC=$(pwd)

    cd v0.2/
    mdbook build
    if [ -d saves ]; then        
	cp -r saves/* book/
    fi

    cd ../v1.0
    cd src/images
    
    make clean
    make all
    rm -rf .build
    
    cd ../..
    
    mdbook build    

    if [ -d saves ]; then        
	cp -r saves/* book/
    fi

    TEMP=$(mktemp -d)
    echo $TEMP
    cd ..

    cd $TEMP
    git clone git@github.com:davidson-consulting/vjoule.git

    cd $TEMP/vjoule
    git checkout gh-pages
    rm -rf v0.2
    rm -rf v1.0

    cp -r $SRC/v0.2/book v0.2
    cp -r $SRC/v1.0/book v1.0
    cp $SRC/index.html .

    git status .
    read -p 'Publish ? (y/N)' ok
    if [ "$ok" = "y" ]; then
	echo "Publishing "
    
	git add .
	git commit -m $1
	git push origin gh-pages
    else
	echo "Abort."
    fi
fi
