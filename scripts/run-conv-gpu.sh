. scripts/common.sh

# cd $DEPS_DIR/troy-nova

# bash scripts/test_conv2d.sh -cheetah -logdir="$LOGS_DIR" $*
# bash scripts/test_conv2d.sh -logdir="$LOGS_DIR" $*

DEV=-D
NTT="--ntt"
CNN="sqnet"
LOGQ="60,49"
VERB="" # Verbose flag

mkdir -p $LOGS_DIR

if [[ $* == *"-resnet50"* ]]; then
    CNN="resnet50"
fi

LOG_FNAME="$CNN-conv2d"

if [[ $* == *"-cpu"* ]]; then
    DEV="-H"
    LOG_FNAME="${LOG_FNAME}-cpu"
else
    LOG_FNAME="${LOG_FNAME}-gpu"
fi

if [[ $* == *"-cheetah"* ]]; then
    NTT=""
    LOG_FNAME="${LOG_FNAME}-cheetah"
else
    LOG_FNAME="${LOG_FNAME}-seconnds_2"
fi

if [[ "$*" == *"-l="* ]]; then
    LOGNUM=$(echo $* | grep -o -P '(?<=-l=)\d+' | head -1)
    LOG_FNAME="$LOG_FNAME-$LOGNUM"
fi

if [[ $* == *"-v"* ]] || [[ $* == *"--verbose"* ]]; then
    VERB="-v"
fi

LOG_FNAME="${LOG_FNAME}.log"

echo -e "Log filename set to $LOG_FNAME"

if [[ "$*" == *"-logdir="* ]]; then
    LOGS_DIR=$(echo $* | grep -o -P '(?<=-logdir=)[^ ]+' | head -1)
fi
echo -e "Log directory set to $LOGS_DIR"
    
if [[ $* == *"-help"* ]]; then
    echo -e "Usage: ./scripts/run-sqnet-conv-gpu.sh [-cpu] [-cheetah] [-resnet50] [-l=<lognum>] [-logdir=<logdir>] [-v/--verbose] [-help]"
    ./deps/troy-nova/build/test/bench_conv2d -h
else
    if [[ $CNN == "sqnet" ]]; then
        echo -e "Running SQNet Conv2d benchmarks..."
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 227 -iw 227 -ic 3 -kh 3 -kw 3 -oc 64 > $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 16 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 16 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 16 -kh 3 -kw 3 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 128 -kh 1 -kw 1 -oc 16 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 16 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 16 -kh 3 -kw 3 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 128 -kh 1 -kw 1 -oc 32 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 32 -kh 1 -kw 1 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 32 -kh 3 -kw 3 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 256 -kh 1 -kw 1 -oc 32 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 32 -kh 1 -kw 1 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 27 -iw 27 -ic 32 -kh 3 -kw 3 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 256 -kh 1 -kw 1 -oc 48 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 48 -kh 1 -kw 1 -oc 192 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 48 -kh 3 -kw 3 -oc 192 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 384 -kh 1 -kw 1 -oc 48 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 48 -kh 1 -kw 1 -oc 192 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 48 -kh 3 -kw 3 -oc 192 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 384 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 64 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 512 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 64 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 13 -iw 13 -ic 512 -kh 1 -kw 1 -oc 1000 >> $LOGS_DIR/$LOG_FNAME
    else
        echo -e "Running ResNet50 Conv2d benchmarks..."
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 230 -iw 230 -ic 3 -kh 7 -kw 7 -oc 64 > $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 3 -kw 3 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 256 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 3 -kw 3 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 256 -kh 1 -kw 1 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 3 -kw 3 -oc 64 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 64 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 256 -kh 1 -kw 1 -oc 512 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 56 -iw 56 -ic 256 -kh 1 -kw 1 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 58 -iw 58 -ic 128 -kh 3 -kw 3 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 128 -kh 1 -kw 1 -oc 512 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 512 -kh 1 -kw 1 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 128 -kh 3 -kw 3 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 128 -kh 1 -kw 1 -oc 512 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 512 -kh 1 -kw 1 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 128 -kh 3 -kw 3 -oc 128 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 128 -kh 1 -kw 1 -oc 512 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 512 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 28 -iw 28 -ic 512 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 30 -iw 30 -ic 256 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 1024 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 1024 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 1024 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 1024 -kh 1 -kw 1 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 3 -kw 3 -oc 256 >> $LOGS_DIR/$LOG_FNAME
        ./deps/troy-nova/build/test/bench_conv2d $VERB -q $LOGQ -N $POLYDEG -rt $SSBITLEN -usp $USP $DEV $NTT -bs 1 -ih 14 -iw 14 -ic 256 -kh 1 -kw 1 -oc 1024 >> $LOGS_DIR/$LOG_FNAME
    fi
fi

echo -e "Done! Log file saved at $LOGS_DIR/$LOG_FNAME"