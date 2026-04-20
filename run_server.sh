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
    sh compile.sh server
    sh compile.sh client_spj
    sh compile.sh server_spj

    mv client tmp/client
    mv server tmp/server
    mv client_spj tmp/spj_client
    mv server_spj tmp/spj_server

    echo "Compile done"
fi

if [ ! -f tmp/client ] || [ ! -f tmp/server ] || [ ! -f tmp/spj_client ] || [ ! -f tmp/spj_server ]; then
    echo "binary files not exist"
    exit 1
fi

cd tmp/
./client > client.log

__check

NO_FILE=/dev/null

./spj_client $NO_FILE client.log $NO_FILE tmp.log spj_client.log

first_char=$(head -c 1 tmp.log)
if [ "$first_char" = "1" ]; then
    echo "Accepted"
    rm tmp.log
else
    echo "Wrong Answer"
    cat spj_client.log
    echo ""
    exit 1
fi

__check

./server < client.log > server.log

__check

./spj_server $NO_FILE server.log $NO_FILE $NO_FILE spj_server.log
