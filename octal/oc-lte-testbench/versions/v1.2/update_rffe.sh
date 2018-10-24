CURRENT_DIR=$(pwd)

cd ../../../script/rffe
tar --exclude='.git' --exclude='README.md' -czvf rffe.tgz bin

cd $CURRENT_DIR
cp ../../../script/rffe/rffe.tgz .