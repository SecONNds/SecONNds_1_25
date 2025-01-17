. scripts/common.sh

check_tools

# Function to clone or update a repository
clone_or_update_repo() {
  local repo_url=$1
  local target_dir=$2
  local commit_hash=$3

  if [ -d "$target_dir/.git" ]; then
    echo "Updating $target_dir"
    cd $target_dir
    git fetch origin
    git checkout $commit_hash
    git pull origin $commit_hash
    cd - > /dev/null
  else
    echo "Cloning $repo_url into $target_dir"
    git clone $repo_url $target_dir
    cd $target_dir
    git checkout $commit_hash
    cd - > /dev/null
  fi
}

mkdir -p $DEPS_DIR

# Repositories and their respective commit hashes
repos=(
  "https://github.com/emp-toolkit/emp-tool.git|$DEPS_DIR/emp-tool|44b1dde"
  "https://github.com/emp-toolkit/emp-ot.git|$DEPS_DIR/emp-ot|7f3d4f0"
  "https://github.com/libigl/eigen.git|$DEPS_DIR/eigen|1f05f51"
  "https://github.com/facebook/zstd.git|$DEPS_DIR/zstd|master"
  "https://github.com/intel/hexl.git|$DEPS_DIR/hexl|343acab"
  "https://github.com/microsoft/SEAL.git|$DEPS_DIR/SEAL|a0fc0b7"
  "https://github.com/lightbulb128/troy-nova.git|$DEPS_DIR/troy-nova|3354734"
)

# Clone or update each repository
for repo in "${repos[@]}"; do
  IFS="|" read -r repo_url target_dir commit_hash <<< "$repo"
  clone_or_update_repo $repo_url $target_dir $commit_hash
done

target=emp-tool
echo "Building dependency: $target"
cd $DEPS_DIR/$target
patch --quiet --no-backup-if-mismatch -N -p1 -i $WORK_DIR/patch/$target.patch -d $DEPS_DIR/$target
mkdir -p $BUILD_DIR/deps/$target
cd $BUILD_DIR/deps/$target
cmake $DEPS_DIR/$target -DCMAKE_INSTALL_PREFIX=$BUILD_DIR
make install -j8

target=emp-ot
echo "Building dependency: $target"
cd $DEPS_DIR/$target
mkdir -p $BUILD_DIR/deps/$target
cd $BUILD_DIR/deps/$target
cmake $DEPS_DIR/$target -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DCMAKE_PREFIX_PATH=$BUILD_DIR
make install -j8

target=eigen
echo "Building dependency: $target"
cd $DEPS_DIR/$target
mkdir -p $BUILD_DIR/deps/$target
cd $BUILD_DIR/deps/$target
cmake $DEPS_DIR/$target -DCMAKE_INSTALL_PREFIX=$BUILD_DIR
make install -j8

target=zstd
echo "Building dependency: $target"
cd $DEPS_DIR/$target
cmake $DEPS_DIR/$target/build/cmake -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_SHARED=OFF\
                                      -DZLIB_BUILD_STATIC=ON -DZSTD_BUILD_TESTS=OFF -DZSTD_MULTITHREAD_SUPPORT=OFF
make install -j8

target=hexl
echo "Building dependency: $target"
cd $DEPS_DIR/$target
cmake $DEPS_DIR/$target -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DHEXL_BENCHMARK=OFF -DHEXL_COVERAGE=OFF -DHEXL_TESTING=OFF
make install -j8

target=SEAL
echo "Building dependency: $target"
cd $DEPS_DIR/$target
patch --quiet --no-backup-if-mismatch -N -p1 -i $WORK_DIR/patch/$target.patch -d $DEPS_DIR/$target/
mkdir -p $BUILD_DIR/deps/$target
cd $BUILD_DIR/deps/$target
cmake $DEPS_DIR/$target -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DCMAKE_PREFIX_PATH=$BUILD_DIR -DSEAL_USE_MSGSL=OFF -DSEAL_USE_ZLIB=OFF\
	                    -DSEAL_USE_ZSTD=ON -DCMAKE_BUILD_TYPE=Release -DSEAL_USE_INTEL_HEXL=ON -DSEAL_BUILD_DEPS=OFF\
                        -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=ON
make install -j8

# Troy-nova
target=troy-nova
echo "Building dependency: $target"
cd $DEPS_DIR/$target
patch --quiet --no-backup-if-mismatch -N -p1 -i $WORK_DIR/patch/$target.patch -d $DEPS_DIR/$target/
bash scripts/build.sh -install -prefix=$BUILD_DIR

for deps in eigen3 emp-ot emp-tool hexl SEAL-4.0 troy
do
  if [ ! -d $BUILD_DIR/include/$deps ] 
  then
	echo -e "${RED}$deps${NC} seems absent in ${BUILD_DIR}/include/, please re-run scripts/build-deps.sh"
	exit 1
  fi
done

for deps in zstd.h 
do
  if [ ! -f $BUILD_DIR/include/$deps ] 
  then
	echo -e "${RED}$deps${NC} seems absent in ${BUILD_DIR}/include/, please re-run scripts/build-deps.sh"
	exit 1
  fi
done
