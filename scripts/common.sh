RED='\033[0;31m'
GREEN='\033[1;32m'
NC='\033[0m'

function has_tool {
  if ! command -v $1 &> /dev/null 
    then
      echo -e "No ${RED}$1${NC} is found."
      exit
	else
	  echo -e "${GREEN}$1${NC} found."
  fi
}

function check_tools {
  has_tool g++
  has_tool make
  has_tool git
  has_tool cmake
  cmake_major=3
  cmake_minior=10
  cmake_version=`cmake --version | head -n1 | awk '{print $3}'`
  cmake_match=`echo $cmake_version | awk -F. '{ if ($1 < 3 || $1 == 3 && $2 < 10) print(0); else print(1) }'`
  if [ $cmake_match -eq 0 ]; then
	echo -e "${RED}require cmake version >= $cmake_major.$cmake_minior but get $cmake_version${NC}"
	exit
  fi
}

function contains {
  local list="$1"
  local item="$2"
  if [[ $list =~ (^|[[:space:]])"$item"($|[[:space:]]) ]] ; then
    # yes, list include item
    result=0
  else
    result=1
  fi
  return $result
}
WORK_DIR=`pwd`
BUILD_DIR=$WORK_DIR/build
DEPS_DIR=$WORK_DIR/deps
LOGS_DIR=$WORK_DIR/logs
mkdir -p $BUILD_DIR
mkdir -p $DEPS_DIR

# change the ip if running remotely
SERVER_IP=127.0.0.1
SERVER_PORT=12345

# fixed-point scale
FXP_SCALE=12
# secret sharing bit length
SS_BITLEN=37
# number of threads
NUM_THREADS=1
# Chunk size for triple generation
CSIZE=32000

# Default number of triples for buffer
## SQNet 32 bits
### SecONNds 2-exponent ring
NTRIPS_SQNET_2_32=290843352
NTRIPS_SQNET_2_LR_32=418672656
### SecONNds prime ring
NTRIPS_SQNET_P_32=725229680
NTRIPS_SQNET_P_LR_32=1044535856
## SQNet 37 bits
### SecONNds 2-exponent ring
NTRIPS_SQNET_2_37=338163872
NTRIPS_SQNET_2_LR_37=489655936
### SecONNds prime ring
NTRIPS_SQNET_P_37=820904720
NTRIPS_SQNET_P_LR_37=1186548416
## RESNET50 32 bits
### SecONNds 2-exponent ring
NTRIPS_RESNET50_2_32=0
NTRIPS_RESNET50_2_LR_32=0
### SecONNds prime ring
NTRIPS_RESNET50_P_32=0
NTRIPS_RESNET50_P_LR_32=0
## RESNET50 37 bits
### SecONNds 2-exponent ring
NTRIPS_RESNET50_2_37=768575032
NTRIPS_RESNET50_2_LR_37=1108285240
### SecONNds prime ring
NTRIPS_RESNET50_P_37=3916088496
NTRIPS_RESNET50_P_LR_37=5646698192