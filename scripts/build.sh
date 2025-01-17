. scripts/common.sh

BUILD_MODE=Release
TRIP_TRIALS=0
TRACK_HE_NOISE=0
VERIFY_LAYERWISE=0
TRACK_MILL_TIME=0
TRACK_MILL_COMM=0
TRACK_MILL_COMP=0

if [[ $* == *"-clean"* ]]; then
  rm -rf $BUILD_DIR
  rm -rf $DEPS_DIR
  echo -e "${GREEN}-clean${NC}: Removed ${BUILD_DIR} and ${DEPS_DIR}."
fi

for deps in eigen3 emp-ot emp-tool hexl SEAL-4.0
do
  if [ ! -d $BUILD_DIR/include/$deps ]; then
	echo -e "${RED}$deps${NC} seems absent in ${BUILD_DIR}/include/"
  echo -e "${GREEN}Building dependencies...${NC}"
  bash scripts/build-deps.sh
  fi
done

for deps in zstd.h 
do
  if [ ! -f $BUILD_DIR/include/$deps ]; then
	echo -e "${RED}$deps${NC} seems absent in ${BUILD_DIR}/include/"
  echo -e "${GREEN}Building dependencies...${NC}"
  bash scripts/build-deps.sh
  fi
done

cd $BUILD_DIR/

if [[ $* == *"-debug"* ]]; then
  BUILD_MODE=Debug
  echo -e "${GREEN}-debug${NC}: Building in Debug mode."
fi

if [[ $* == *"--trip_trials"* ]]; then
  TRIP_TRIALS=1
  echo -e "${GREEN}--trip_trials${NC}: Running triple generation trials."
fi

if [[ $* == *"--track_he_noise"* ]] || [[ "$*" == *"-noise"* ]]; then
  TRACK_HE_NOISE=1
  echo -e "${GREEN}--track_he_noise/-noise${NC}: Tracking HE noise."
fi

if [[ $* == *"--verify_layerwise"* ]] || [[ "$*" == *"-verify"* ]]; then
  VERIFY_LAYERWISE=1
  echo -e "${GREEN}--verify_layerwise/-verify${NC}: Verifying layerwise output."
fi

if [[ $* == *"--track_mill_time"* ]] || [[ "$*" == *"-milltime"* ]]; then
  TRACK_MILL_TIME=1
  echo -e "${GREEN}--track_mill_time/-milltime${NC}: Tracking time for Millionaires' Protocol."
fi

if [[ $* == *"--track_mill_comm"* ]] || [[ "$*" == *"-millcomm"* ]]; then
  TRACK_MILL_COMM=1
  echo -e "${GREEN}--track_mill_comm/-millcomm${NC}: Tracking communication for Millionaires' Protocol."
fi

if [[ $* == *"--track_mill_comp"* ]] || [[ "$*" == *"-millcomp"* ]]; then
  TRACK_MILL_COMP=1
  echo -e "${GREEN}--track_mill_comp/-millcomp${NC}: Tracking computation in Millionaires' Protocol."
fi

cmake .. -DCMAKE_BUILD_TYPE=$BUILD_MODE -DSCI_BUILD_NETWORKS=ON -DSCI_BUILD_TESTS=ON \
          -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DCMAKE_PREFIX_PATH=$BUILD_DIR -DUSE_APPROX_RESHARE=ON \
          -DRUN_TRIP_TRIALS=$TRIP_TRIALS -DTRACK_HE_NOISE=$TRACK_HE_NOISE -DVERIFY_LAYERWISE=$VERIFY_LAYERWISE \
          -DMILL_PRINT_TIME=$TRACK_MILL_TIME -DMILL_PRINT_COMM=$TRACK_MILL_COMM -DMILL_PRINT_COMP=$TRACK_MILL_COMP

for net in resnet50 sqnet densenet121
do
    make ${net}-cheetah -j16
    make ${net}-SCI_HE -j16
done
