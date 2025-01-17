// Author: Zhichong Huang

#define TRUNC_PRINT_TIME 0
#define TRUNC_PRINT_COMM 0
#if TRUNC_PRINT_TIME || TRUNC_PRINT_COMM
#include <iomanip>
#endif


void Truncation::truncate(int32_t dim, uint64_t *inA, uint64_t *outB,
                          int32_t shift, int32_t bw, bool signed_arithmetic,
                          uint8_t *msb_x, bool apply_msb0_heuristic) {
  if (msb_x != nullptr)
    return truncate(dim, inA, outB, shift, bw, signed_arithmetic, msb_x);

  if (shift == 0) {
    memcpy(outB, inA, sizeof(uint64_t) * dim);
    return;
  }
  assert((bw - shift) > 0 && "Truncation shouldn't truncate the full bitwidth");
  assert((signed_arithmetic && (bw - shift - 1 >= 0)) || !signed_arithmetic);
  assert(inA != outB);

  uint64_t mask_bw = (bw == 64 ? -1 : ((1ULL << bw) - 1));
  uint64_t mask_upper =
      ((bw - shift) == 64 ? -1 : ((1ULL << (bw - shift)) - 1));

  if (apply_msb0_heuristic) {
    // Ref "Secure evaluation of quantized neural networks"
    // https://eprint.iacr.org/2019/131.pdf
    if (party == sci::BOB) {
      const int m = bw - 3;
      std::vector<uint64_t> adjust(dim);
      uint64_t big_positive = 1UL << m;
      std::transform(inA, inA + dim, adjust.data(),
                     [&](uint64_t x) { return (x + big_positive) & mask_bw; });

      truncate_msb0(dim, adjust.data(), outB, shift, bw, signed_arithmetic);
      // Tr(x + 2^m, 2^f) = x/2^f + 2^{m - f}
      uint64_t offset = 1UL << (m - shift);
      std::transform(outB, outB + dim, outB,
                     [&](uint64_t x) { return (x - offset) & mask_bw; });
    } else {
      truncate_msb0(dim, inA, outB, shift, bw, signed_arithmetic);
    }
    return;
  }

  uint64_t *inA_orig = new uint64_t[dim];
  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      inA_orig[i] = inA[i];
      inA[i] = ((inA[i] + (1ULL << (bw - 1))) & mask_bw);
    }
  }

  uint64_t *inA_upper = new uint64_t[dim];
  uint8_t *wrap_upper = new uint8_t[dim];
  for (int i = 0; i < dim; i++) {
    inA_upper[i] = inA[i] & mask_bw;
    if (party == sci::BOB) {
      inA_upper[i] = (mask_bw - inA_upper[i]) & mask_bw;
    }
  }

  this->mill->compare(wrap_upper, inA_upper, dim, bw);

  uint64_t *arith_wrap_upper = new uint64_t[dim];
  this->aux->B2A(wrap_upper, arith_wrap_upper, dim, shift);
  io->flush();

  for (int i = 0; i < dim; i++) {
    outB[i] = (((inA[i] >> shift) & mask_upper) -
               (1ULL << (bw - shift)) * arith_wrap_upper[i]) &
              mask_bw;
  }

  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      outB[i] = ((outB[i] - (1ULL << (bw - shift - 1))) & mask_bw);
      inA[i] = inA_orig[i];
    }
  }
  delete[] inA_orig;
  delete[] inA_upper;
  delete[] wrap_upper;
  delete[] arith_wrap_upper;

  return;
}

void Truncation::truncate_msb(int32_t dim, uint64_t *inA, uint64_t *outB,
                              int32_t shift, int32_t bw, bool signed_arithmetic,
                              uint8_t *msb_x) {
  if (shift == 0) {
    memcpy(outB, inA, sizeof(uint64_t) * dim);
    return;
  }
  assert((bw - shift) > 0 && "Truncation shouldn't truncate the full bitwidth");
  assert((signed_arithmetic && (bw - shift - 1 >= 0)) || !signed_arithmetic);
  assert(inA != outB);

  uint64_t mask_bw = (bw == 64 ? -1 : ((1ULL << bw) - 1));
  uint64_t mask_shift = (shift == 64 ? -1 : ((1ULL << shift) - 1));
  uint64_t mask_upper =
      ((bw - shift) == 64 ? -1 : ((1ULL << (bw - shift)) - 1));

  uint64_t *inA_orig = new uint64_t[dim];

  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      inA_orig[i] = inA[i];
      inA[i] = ((inA[i] + (1ULL << (bw - 1))) & mask_bw);
    }
  }

  uint64_t *inA_upper = new uint64_t[dim];
  uint8_t *wrap_upper = new uint8_t[dim];
  for (int i = 0; i < dim; i++) {
    inA_upper[i] = (inA[i] >> shift) & mask_upper;
    if (party == sci::BOB) {
      inA_upper[i] = (mask_upper - inA_upper[i]) & mask_upper;
    }
  }

  if (signed_arithmetic) {
    uint8_t *inv_msb_x = new uint8_t[dim];
    for (int i = 0; i < dim; i++) {
      inv_msb_x[i] = msb_x[i] ^ (party == sci::ALICE ? 1 : 0);
    }
    this->aux->MSB_to_Wrap(inA, inv_msb_x, wrap_upper, dim, bw);
    delete[] inv_msb_x;
  } else {
    this->aux->MSB_to_Wrap(inA, msb_x, wrap_upper, dim, bw);
  }

  uint64_t *arith_wrap_upper = new uint64_t[dim];
  this->aux->B2A(wrap_upper, arith_wrap_upper, dim, shift);
  io->flush();

  for (int i = 0; i < dim; i++) {
    outB[i] = (((inA[i] >> shift) & mask_upper) -
               (1ULL << (bw - shift)) * arith_wrap_upper[i]) &
              mask_bw;
  }

  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      outB[i] = ((outB[i] - (1ULL << (bw - shift - 1))) & mask_bw);
      inA[i] = inA_orig[i];
    }
  }
  delete[] inA_orig;
  delete[] inA_upper;
  delete[] wrap_upper;
  delete[] arith_wrap_upper;

  return;
}

// Truncate (right-shift) by shift in the same ring (round towards -inf)
// All elements have msb equal to 0.
void Truncation::truncate_msb0(
    // Size of vector
    int32_t dim,
    // input vector
    uint64_t *inA,
    // output vector
    uint64_t *outB,
    // right shift amount
    int32_t shift,
    // Input and output bitwidth
    int32_t bw,
    // signed truncation?
    bool signed_arithmetic) {

#if TRUNC_PRINT_TIME || TRUNC_PRINT_COMM 
    std::string f_tag = "Cheetah | Trunc";
#endif  
#if TRUNC_PRINT_TIME
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag;
    log_time << ": dim = " << dim;
    log_time << ", shift = " << shift;
    log_time << ", bw = " << bw;
    log_time << ", signed_arithmetic = " << std::boolalpha << signed_arithmetic;
    log_time << std::endl;
#endif
#if TRUNC_PRINT_COMM
    int _w2 = 10;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    log_comm << "P" << party << " COMM | ";
    log_comm << f_tag;
    log_comm << ": dim = " << dim;
    log_comm << ", shift = " << shift;
    log_comm << ", bw = " << bw;
    log_comm << ", signed_arithmetic = " << std::boolalpha << signed_arithmetic;
    log_comm << std::endl;
#endif


  if (shift == 0) {
    memcpy(outB, inA, sizeof(uint64_t) * dim);
    return;
  }
  assert((bw - shift) > 0 && "Truncation shouldn't truncate the full bitwidth");
  assert((signed_arithmetic && (bw - shift - 1 >= 0)) || !signed_arithmetic);
  assert(inA != outB);

  uint64_t mask_bw = (bw == 64 ? -1 : ((1ULL << bw) - 1));
  uint64_t mask_shift = (shift == 64 ? -1 : ((1ULL << shift) - 1));
  uint64_t mask_upper =
      ((bw - shift) == 64 ? -1 : ((1ULL << (bw - shift)) - 1));

  uint64_t *inA_orig = new uint64_t[dim];

  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      inA_orig[i] = inA[i];
      inA[i] = ((inA[i] + (1ULL << (bw - 1))) & mask_bw);
    }
  }

  uint8_t *wrap_upper = new uint8_t[dim];

  if (signed_arithmetic)
    this->aux->msb1_to_wrap(inA, wrap_upper, dim, bw);
  else
    this->aux->msb0_to_wrap(inA, wrap_upper, dim, bw);

#if TRUNC_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> wrap_time = end - (start + total_time);
    total_time += wrap_time;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | Wrap : " << wrap_time.count() * 1000;
    log_time << " ms" << std::endl;
#endif
#if TRUNC_PRINT_COMM
    uint64_t comm_wrap = io->counter - (comm_start + comm_total);
    comm_total += comm_wrap;
    log_comm << "P" << party << " COMM | ";
    log_comm << f_tag << " | Wrap : " << std::setw(_w2) << comm_wrap;
    log_comm << std::endl;
#endif

  uint64_t *arith_wrap_upper = new uint64_t[dim];
  this->aux->B2A(wrap_upper, arith_wrap_upper, dim, shift);
  io->flush();

#if TRUNC_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> b2a_time = end - (start + total_time);
    total_time += b2a_time;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | B2A  : " << b2a_time.count() * 1000;
    log_time << " ms" << std::endl;
#endif
#if TRUNC_PRINT_COMM
    uint64_t comm_b2a = io->counter - (comm_start + comm_total);
    comm_total += comm_b2a;
    log_comm << "P" << party << " COMM | ";
    log_comm << f_tag << " | B2A  : " << std::setw(_w2) << comm_b2a;
    log_comm << std::endl;
#endif

  for (int i = 0; i < dim; i++) {
    outB[i] = (((inA[i] >> shift) & mask_upper) -
               (1ULL << (bw - shift)) * arith_wrap_upper[i]) &
              mask_bw;
  }

  if (signed_arithmetic && (party == sci::ALICE)) {
    for (int i = 0; i < dim; i++) {
      outB[i] = ((outB[i] - (1ULL << (bw - shift - 1))) & mask_bw);
      inA[i] = inA_orig[i];
    }
  }

#if TRUNC_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> full_time = end - start;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | Total: " << full_time.count() * 1000;
    log_time << " ms" << std::endl;
    std::cout << log_time.str();
#endif
#if TRUNC_PRINT_COMM
    uint64_t comm_full = io->counter - comm_start;
    log_comm << "P" << party << " COMM | ";
    log_comm << f_tag << " | Total: " << std::setw(_w2) << comm_full;
    log_comm << std::endl;
    std::cout << log_comm.str();
#endif

  delete[] inA_orig;
  delete[] wrap_upper;
  delete[] arith_wrap_upper;

  return;
}
