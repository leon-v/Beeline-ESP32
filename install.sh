#/bin/sh

cd ./esp-idf
git checkout release/v3.3
git reset --hard HEAD
git pull
cd ..

git submodule update --init --recursive

cd ./esp-idf
python -m pip install --user -r requirements.txt
cd ..

cd toolchain
tar xzf *.tar.gz
cd ..

./idf.sh build