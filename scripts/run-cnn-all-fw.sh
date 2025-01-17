. scripts/common.sh

if [ $# -lt 2 ]; then
    echo -e "${RED}Invalid number of arguments.${NC}"
    echo "Usage: run-cnn-all-fw [server|client] [sqnet|resnet50|densenet121]"
    exit 1
fi

if ! contains "server client" $1; then
    echo -e "Usage: run-cnn-all-fw ${RED}[server|client]${NC} [sqnet|resnet50|densenet121]"
    exit 1
fi

if ! contains "sqnet resnet50 densenet121" $2; then
    echo -e "Usage: run-cnn-all-fw [server|client] ${RED}[sqnet|resnet50|densenet121]${NC}"
    exit 1
fi

ROLE=$1
DNN=$2

# remove $1 $2 from the arguments
shift 2

for FW in seconnds_2 seconnds_p cheetah SCI_HE 
do
    bash scripts/run-cnn.sh $ROLE $FW $DNN $*
done

if [[ $* != *"-mlr"* ]] && [[ $* != *"--mill_low_rnd"* ]]; then
    for FW in seconnds_2 seconnds_p
    do
        bash scripts/run-cnn.sh $ROLE $FW $DNN "-mlr" $*
    done
fi

if [[ $* != *"-ntt"* ]] && [[ $* != *"--conv_ntt"* ]]; then
    for FW in cheetah SCI_HE
    do
        bash scripts/run-cnn.sh $ROLE $FW $DNN "-ntt" $*
    done
fi