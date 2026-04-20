__check() {
    if [ $? -ne 0 ]; then
        echo "error"
        exit 1
    fi
}

# if input arg --run, no compile
if [ "$1" = "--compile" ]; then
    mkdir tmp/ -p

    sh compile.sh client

    mv client tmp/client

    __check

    echo "Compile done"
fi

if [ ! -f tmp/client ]; then
    echo "binary files not exist. rerun with flag --compile"
    exit 1
fi

echo "enter a number in [0, 3]"

cd tmp

./client > /dev/null

__check
