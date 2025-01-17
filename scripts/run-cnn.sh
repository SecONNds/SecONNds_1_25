. scripts/common.sh

BASE_FWORK=cheetah # Base Framework
NTHREADS=$NUM_THREADS
SNN=0
NTT=0
NTRIPS=0
ROLE=client
ROLENUM=2
PRIVINP=0
MILL_LR=0 # Millionaires' with lower rounds

echo -e "=============================================================================="
echo -e " "

if [ $# -lt 3 ]; then
  echo -e "${RED}Invalid number of arguments.${NC}"
  echo -e "Usage: run-cnn.sh [server|client] [seconnds_2|seconnds_p|cheetah|SCI_HE] [sqnet|resnet50|densenet121] [(optional) log-file tag]"
  exit 1
fi

if ! contains "server client" $1; then
  echo -e "Usage: run-cnn.sh ${RED}[server|client]${NC} [seconnds_2|seconnds_p|cheetah|SCI_HE] [sqnet|resnet50|densenet121] [(optional) log-file tag]"
  exit 1
fi

if ! contains "cheetah SCI_HE seconnds_2 seconnds_p" $2; then
  echo -e "Usage: run-cnn.sh [server|client] ${RED}[seconnds_2|seconnds_p|cheetah|SCI_HE]${NC} [sqnet|resnet50|densenet121] [(optional) log-file tag]"
  exit 1
fi

if ! contains "sqnet resnet50 densenet121" $3; then
  echo -e "Usage: run-cnn.sh [server|client] [seconnds_2|seconnds_p|cheetah|SCI_HE] ${RED}[sqnet|resnet50|densenet121]${NC} [(optional) log-file tag]"
  exit 1
fi

ROLE=$1
FWORK=$2
DNN=$3

echo -e "${GREEN}$ROLE${NC}: Running ${GREEN}$DNN${NC} with ${GREEN}$FWORK${NC}..."
echo -e " "

if [[ "$*" == *"--mill_low_rnd"* ]] || [[ "$*" == *"-mlr"* ]]; then
  if [ "$FWORK" == "seconnds_2" ] || [ "$FWORK" == "seconnds_p" ]; then
    MILL_LR=1
    echo -e "${GREEN}--mill_low_rnd/-mlr${NC}: Using Millionaires' with lower rounds."
    echo -e "${RED}(Must be set for both server and client.)${NC}"
    echo -e " "
  fi
fi

if [[ "$*" == *"--conv_ntt"* ]] || [[ "$*" == *"-ntt"* ]]; then
  NTT=1
  echo -e "${GREEN}--conv_ntt/-ntt${NC}: Server is using convolution with NTT preprocessing."
  echo -e " "
fi

if [[ "$*" == *"-bl="* ]]; then
  SS_BITLEN=$(echo $* | grep -o -P '(?<=-bl=)\d+' | head -1)
  echo -e "${GREEN}-bl=$SS_BITLEN${NC}: Setting secret sharing bit length to ${GREEN}$SS_BITLEN${NC}."
  echo -e "${RED}(Must be same for both server and client.)${NC}"
  echo -e " "
fi

if [[ "$*" == *"-j="* ]]; then
  NTHREADS=$(echo $* | grep -o -P '(?<=-j=)\d+' | head -1)
  echo -e "${GREEN}-j=$NTHREADS${NC}: Setting number of threads to ${GREEN}$NTHREADS${NC}."
  echo -e "${RED}(Must be same for both server and client.)${NC}"
  echo -e " "
fi

if [ "$ROLE" = "server" ]; then
  ROLENUM=1
  PRIVINP=pretrained/$3_model_scale12.inp
else
  ROLENUM=2
  PRIVINP=pretrained/$3_input_scale12_pred*.inp
fi

if [ "$FWORK" = "SCI_HE" ]; then
  BASE_FWORK=SCI_HE
fi

if [ "$DNN" = "resnet50" ]; then
  SS_BITLEN=37
fi

if [ "$SS_BITLEN" == "32" ]; then
  if [ "$FWORK" = "seconnds_2" ]; then
    BASE_FWORK=cheetah
    SNN=1
    NTT=1
    if [ "$DNN" = "sqnet" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_SQNET_2_LR_32
      else
        NTRIPS=$NTRIPS_SQNET_2_32
      fi
    fi
    if [ "$DNN" = "resnet50" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_RESNET50_2_LR_32
      else
        NTRIPS=$NTRIPS_RESNET50_2_32
      fi
    fi
  fi
  if [ "$FWORK" = "seconnds_p" ]; then
    BASE_FWORK=SCI_HE
    SNN=1
    NTT=1
    if [ "$DNN" = "sqnet" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_SQNET_P_LR_32
      else
        NTRIPS=$NTRIPS_SQNET_P_32
      fi
    fi
    if [ "$DNN" = "resnet50" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_RESNET50_P_LR_32
      else
        NTRIPS=$NTRIPS_RESNET50_P_32
      fi
    fi
  fi
fi


if [ "$SS_BITLEN" == "37" ]; then
  if [ "$FWORK" = "seconnds_2" ]; then
    BASE_FWORK=cheetah
    SNN=1
    NTT=1
    if [ "$DNN" = "sqnet" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_SQNET_2_LR_37
      else
        NTRIPS=$NTRIPS_SQNET_2_37
      fi
    fi
    if [ "$DNN" = "resnet50" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_RESNET50_2_LR_37
      else
        NTRIPS=$NTRIPS_RESNET50_2_37
      fi
    fi
  fi

  if [ "$FWORK" = "seconnds_p" ]; then
    BASE_FWORK=SCI_HE
    SNN=1
    NTT=1
    if [ "$DNN" = "sqnet" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_SQNET_P_LR_37
      else
        NTRIPS=$NTRIPS_SQNET_P_37
      fi
    fi
    if [ "$DNN" = "resnet50" ]; then
      if [ "$MILL_LR" = 1 ]; then
        NTRIPS=$NTRIPS_RESNET50_P_LR_37
      else
        NTRIPS=$NTRIPS_RESNET50_P_37
      fi
    fi
  fi
fi

if [[ "$*" == *"--disable_buffer"* ]] || [[ "$*" == *"-nobuff"* ]]; then
  NTRIPS=0
  echo -e "${GREEN}--disable_buffer/-nobuff${NC}: Disabling Bit Triple Buffer."
  echo -e "${RED}(Must be set for both server and client.)${NC}"
  echo -e " "
fi

# create a data/ to store the Ferret output
mkdir -p data

if [[ "$*" == *"-debug"* ]]; then
  echo -e "${GREEN}-debug${NC}: Running in Debug mode (with GDB)."
  echo -e " "
  gdb build/bin/$DNN-$BASE_FWORK -ex "run r=$ROLENUM k=$FXP_SCALE ell=$SS_BITLEN nt=$NTHREADS ip=$SERVER_IP p=$SERVER_PORT snn=$SNN ntt=$NTT ntrips=$NTRIPS csize=$CSIZE mlr=$MILL_LR < $PRIVINP"
else
  if [[ "$*" == *"--no_log"* ]] || [[ "$*" == *"-nl"* ]]; then
    echo -e "${GREEN}--no_log/-nl${NC}: No log file will be generated."
    echo -e " "
    cat $PRIVINP | build/bin/$DNN-$BASE_FWORK r=$ROLENUM k=$FXP_SCALE ell=$SS_BITLEN nt=$NTHREADS ip=$SERVER_IP p=$SERVER_PORT snn=$SNN ntt=$NTT ntrips=$NTRIPS csize=$CSIZE mlr=$MILL_LR
  else
    mkdir -p $LOGS_DIR

    LOGFILE="$DNN-bl$SS_BITLEN-j$NTHREADS-$FWORK"

    if [ "$MILL_LR" == "1" ]; then
      LOGFILE="$LOGFILE"_"mlr"
    fi

    if [[ "$FWORK" == "SCI_HE"  || "$FWORK" == "cheetah" ]] && [ "$NTT" == "1" ]; then
      LOGFILE="$LOGFILE"_"ntt"
    fi

    if [[ "$*" == *"--disable_buffer"* ]] || [[ "$*" == *"-nobuff"* ]]; then
      LOGFILE="$LOGFILE"_"nobuff"
    fi

    if [[ "$*" == *"-l="* ]]; then
      LOGNUM=$(echo $* | grep -o -P '(?<=-l=)\d+' | head -1)
      LOGFILE="$LOGFILE-$LOGNUM-$ROLE.log"
      echo -e "${GREEN}-l=$LOGNUM${NC}: Log file will be saved as ${GREEN}$LOGFILE${NC}."
    else
      LOGFILE="$LOGFILE-$ROLE.log"
    fi

    echo -e "Date: $(date)" > "$LOGS_DIR/$LOGFILE"
    echo -e "$ROLE: Running $DNN with $FWORK..." >> "$LOGS_DIR/$LOGFILE"
    echo -e " " >> "$LOGS_DIR/$LOGFILE"
    cat $PRIVINP | build/bin/$DNN-$BASE_FWORK r=$ROLENUM k=$FXP_SCALE ell=$SS_BITLEN nt=$NTHREADS ip=$SERVER_IP p=$SERVER_PORT snn=$SNN ntt=$NTT ntrips=$NTRIPS csize=$CSIZE mlr=$MILL_LR >> "$LOGS_DIR/$LOGFILE"
    echo -e "${GREEN}Done!${NC} Log saved to:"
    echo -e "${GREEN}$LOGS_DIR/$LOGFILE${NC}"
    echo -e " "
  fi
fi

echo -e "=============================================================================="
