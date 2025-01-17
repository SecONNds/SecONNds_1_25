# SecONNds: Secure Outsourced Neural Network Inference on ImageNet

This branch only contains the code for CPU only execution. For the CUDA version, please switch to the `main` branch.

## Introduction

SecONNds is a framework for secure outsourced neural network inference, particularly focusing on ImageNet models. It allows a client to perform inference on a server's model without revealing the client's input or the server's model weights.

## Prerequisites

Ensure that the following tools are installed on your system:

- `g++`
- `make`
- `git`
- `cmake` (version 3.10 or higher)

## Building the Project

To build SecONNds, run:

```bash
bash scripts/build.sh [options]
```

### Build Options

- `-clean`: Removes the build directory before building. **Default:** Do not clean.
- `-debug`: Builds in Debug mode. **Default:** Release mode.
- `--trip_trials`: Runs triple generation trials. **Default:** Off.
- `--verify_layerwise` or `-verify`: Verifies layer-wise outputs during execution. **Default:** Off.
- `--track_he_noise` or `-noise`: Tracks Homomorphic Encryption (HE) noise during computation. **Default:** Off.
- `--track_mill_time` or `-milltime`: Tracks Millionaires' protocol execution time. **Default:** Off.
- `--track_mill_comm` or `-millcomm`: Tracks Millionaires' protocol bandwidth usage. **Default:** Off.


### Examples

- **Clean build in Release mode:**

  ```bash
  bash scripts/build.sh -clean
  ```

- **Build in Debug mode with HE noise tracking:**

  ```bash
  bash scripts/build.sh -debug -noise
  ```

### Dependencies

The build script will automatically check for and build necessary dependencies if they are not present. These include:

- [Eigen](https://gitlab.com/libeigen/eigen)
- [EMP-Toolkit](https://github.com/emp-toolkit) (`emp-tool`, `emp-ot`)
- [Zstandard](https://github.com/facebook/zstd) (`zstd`)
- [Intel HEXL](https://github.com/intel/hexl) (`hexl`)
- [Microsoft SEAL](https://github.com/microsoft/SEAL) (`SEAL`)
- [SEAL-CUDA](https://github.com/tnishimoto/SEAL-Embedded) (`seal-cuda`)

## Running the Project

To run SecONNds, both the **server** and **client** programs need to be executed. The client supplies the private data, and the server supplies the private model to the secure inference protocol. If running locally, you need to use two terminal sessions: one for the server command and one for the client command.

In one terminal session, run the server command:

```bash
bash scripts/run-cnn.sh server [framework] [network] [options]
```

In another terminal session, run the client command:

```bash
bash scripts/run-cnn.sh client [framework] [network] [options]
```

### Required Arguments

1. **Role**: `server` or `client`
2. **Framework**: Choose from:
   - `seconnds_2`
   - `seconnds_p`
   - `cheetah`
   - `SCI_HE`
3. **Network**: Choose from:
   - `sqnet`
   - `resnet50`
   - `densenet121`

### Optional Arguments

- `--mill_low_rnd` or `-mlr`: Use Millionaires' protocol with lower rounds. **Default:** Off.
  - *Note:* Applicable for `seconnds_2` and `seconnds_p` frameworks.
  - *Note:* Must be consistent between server and client.
- `--conv_ntt` or `-ntt`: Use convolution with NTT preprocessing at the server. **Default:** Off.
  - *Note:* Applicable for `cheetah` and `SCI_HE` frameworks.
- `-bl=[bit length]`: Set the secret sharing bit length. **Default:** `32`.
  - *Note:* Must be consistent between server and client.
- `-j=[number of threads]`: Set the number of threads. **Default:** `1`.
  - *Note:* Must be consistent between server and client.
- `-debug`: Run in Debug mode using GDB. **Default:** Off.
- `--no_log` or `-nl`: Do not generate a log file. **Default:** Log files are generated.
- `-l=[log file number]`: tag the log file name with a number. **Default:** No prefix.

### Examples

- **Run `sqnet` network using `seconnds_2` framework with 16 threads:**

  **Terminal Session 1 (Server):**

  ```bash
  bash scripts/run-cnn.sh server seconnds_2 sqnet -j=16
  ```

  **Terminal Session 2 (Client):**

  ```bash
  bash scripts/run-cnn.sh client seconnds_2 sqnet -j=16
  ```

- **Run `sqnet` network using `seconnds_p` framework with lower rounds Millionaires' protocol:**

  **Terminal Session 1 (Server):**

  ```bash
  bash scripts/run-cnn.sh server seconnds_p sqnet -mlr
  ```

  **Terminal Session 2 (Client):**

  ```bash
  bash scripts/run-cnn.sh client seconnds_p sqnet -mlr
  ```

- **Run `resnet50` network using `cheetah` framework with server-side NTT preprocessing and 37-bit secret sharing:**

  **Terminal Session 1 (Server):**

  ```bash
  bash scripts/run-cnn.sh server cheetah resnet50 -ntt -bl=37
  ```

  **Terminal Session 2 (Client):**

  ```bash
  bash scripts/run-cnn.sh client cheetah resnet50 -bl=37
  ```

## Running All Frameworks

To run all frameworks for a given network, use:

```bash
bash scripts/run-cnn-all-fw.sh [server|client] [network] [options]
```

- This script sequentially runs all frameworks (`seconnds_2`, `seconnds_p`, `cheetah`, `SCI_HE`) with the specified network and options.

### Example

- **Run all frameworks on `sqnet` network:**

  **Terminal Session 1 (Server):**

  ```bash
  bash scripts/run-cnn-all-fw.sh server sqnet
  ```

  **Terminal Session 2 (Client):**

  ```bash
  bash scripts/run-cnn-all-fw.sh client sqnet
  ```

## Common Configuration

The scripts use common configurations defined in `scripts/common.sh`. Important variables include:

- **`SERVER_IP`**: IP address of the server.
  - **Default:** `127.0.0.1`
- **`SERVER_PORT`**: Port number for communication.
  - **Default:** `12345`
- **`FXP_SCALE`**: Fixed-point scaling factor.
  - **Default:** `12`
- **`SS_BITLEN`**: Secret sharing bit length.
  - **Default:** `32`
- **`NUM_THREADS`**: Number of threads for execution.
  - **Default:** `1`
- **`CSIZE`**: Chunk size for triple generation.
  - **Default:** `32000`

*Ensure that `SS_BITLEN` and `NUM_THREADS` are the same on both server and client.*

## Logs

- By default, logs are saved in the `logs/` directory.

- Use `-l=[log number]` to tag the log file with a number.

- The log file name format is:

  ```
  [network]-bl[bit length]-j[threads]-[framework]-[options]-[log number]-[role].log
  ```

### Disable Logging

- Use the `--no_log` or `-nl` option to prevent log file generation.

## Debugging

- Use the `-debug` option to run the program under GDB for debugging purposes.

  ```bash
  bash scripts/run-cnn.sh [server|client] [framework] [network] -debug
  ```

## Additional Information

- **Dependencies are patched and built automatically.**
  - Custom patches are applied to some dependencies to ensure compatibility.
- **Ensure consistent options between server and client.**
  - Options like `-mlr`, `-bl`, and `-j` must be the same on both sides.
- **Data Directory:**
  - Input files and model weights are expected to be in the `pretrained/` directory.
  - The scripts will create a `data/` directory to store intermediate data if needed.

## Troubleshooting

- **CMake Version:**
  - If you encounter issues during the build, ensure that your CMake version is 3.10 or higher.
- **Missing Dependencies:**
  - If a dependency is missing, the build script should automatically attempt to fetch and build it.
  - If problems persist, manually check the `deps/` and `build/` directories.


## Contact

For issues or questions, please contact the authors of the repository.
