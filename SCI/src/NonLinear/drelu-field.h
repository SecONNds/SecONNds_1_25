/*
Authors: Mayank Rathee, Deevashwer Rathee
Modified by Zhicong Huang
Copyright:
Copyright (c) 2020 Microsoft Research
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef DRELU_FIELD_H__
#define DRELU_FIELD_H__

#include <cmath>

#include "Millionaire/millionaire.h"
#include "OT/emp-ot.h"
#include "utils/emp-tool.h"

template <typename IO>
class DReLUFieldProtocol {
 public:
  IO *io = nullptr;
  sci::OTPack<IO> *otpack;
  TripleGenerator<IO> *triple_gen;
  MillionaireProtocol<IO> *millionaire;
  int party;
  int l, r, log_alpha, beta, beta_pow;
  int num_digits, num_triples_corr, num_triples_std, log_num_digits;
  int num_triples;
  uint8_t mask_beta, mask_r, take_lsb;
  uint64_t p, p_2;

  DReLUFieldProtocol(int party, int bitlength, int log_radix_base,
                     uint64_t prime_mod, IO *io, sci::OTPack<IO> *otpack) {
    assert(log_radix_base <= 8);
    assert(bitlength <= 64);
    this->party = party;
    this->l = bitlength;
    this->beta = log_radix_base;
    this->io = io;
    this->p = prime_mod;
    this->otpack = otpack;
    this->millionaire = new MillionaireProtocol<IO>(party, io, otpack,
                                                    bitlength, log_radix_base);
    this->triple_gen = millionaire->triple_gen;
    configure();
  }

  DReLUFieldProtocol(int party, int bitlength, int log_radix_base,
                     uint64_t prime_mod, IO *io, sci::OTPack<IO> *otpack,
                     TripleGenerator<IO> *triplegen,
                     bool use_low_round = false) {
    if(triplegen->isBufferEnabled()) log_radix_base = 1;
    assert(log_radix_base <= 8);
    assert(bitlength <= 64);
    this->party = party;
    this->l = bitlength;
    this->beta = log_radix_base;
    this->io = io;
    this->p = prime_mod;
    this->otpack = otpack;
    this->triple_gen = triplegen;
    this->millionaire = new MillionaireProtocol<IO>(party, io, otpack, triplegen, 
      use_low_round, bitlength, log_radix_base);
    configure();
  }

  void configure() {
    this->num_digits = ceil((double)l / beta);
    this->r = l % beta;
    this->log_alpha = sci::bitlen(num_digits) - 1;
    this->log_num_digits = log_alpha + 1;
    this->num_triples_corr = 2 * num_digits - 2 - 2 * log_num_digits;
    this->num_triples_std = log_num_digits;
    this->num_triples = num_triples_std + num_triples_corr;
    if (beta == 8)
      this->mask_beta = -1;
    else
      this->mask_beta = (1 << beta) - 1;
    this->mask_r = (1 << r) - 1;
    this->beta_pow = 1 << beta;
    this->take_lsb = 1;
    this->p_2 = (p - 1) / 2;
  }

  ~DReLUFieldProtocol() { delete millionaire; }

  void compute_drelu(uint8_t *drelu, uint64_t *share, int num_relu) {
    if (triple_gen->isBufferEnabled()) {
      compute_drelu_new(drelu, share, num_relu);
    } else {
      compute_drelu_old(drelu, share, num_relu);
    }
  }

  void compute_drelu_old(uint8_t *drelu, uint64_t *share, int num_relu) {

#if MILL_PRINT_TIME
    int _w1 = 8;
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    log_time << "-OLD-MILL-";
    log_time << "TIME";
#endif
#if MILL_PRINT_COMM
    int _w2 = 8;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    log_comm << "-OLD-MILL-";
    log_comm << "COMM";
#endif

    int num_cmps = 2 * num_relu;
    uint8_t *digits;        // num_digits * num_cmps
    uint8_t *leaf_res_cmp;  // num_digits * num_cmps
    uint8_t *leaf_res_eq;   // num_digits * num_cmps

    // To save number of rounds in WAN over slight increase in communication
// #if defined(WAN_EXEC) || USE_CHEETAH
    Triple triples_std((num_triples)*num_cmps, true);
// #else
//     Triple triples_corr(num_triples_corr * num_cmps, true, num_cmps);
//     Triple triples_std(num_triples_std * num_cmps, true);
// #endif
    digits = new uint8_t[num_digits * num_cmps];
    leaf_res_cmp = new uint8_t[num_digits * num_cmps];
    leaf_res_eq = new uint8_t[num_digits * num_cmps];

    // Extract radix-digits from data
    assert((beta <= 8) && "Base beta > 8 is not implemented");
    if (party == sci::ALICE) {
      for (int j = 0; j < num_relu; j++) {
        uint64_t input_wrap_cmp = (p - 1 - share[j]) + p_2;
        uint64_t input_drelu_cmp;
        if (share[j] > p_2) {
          input_drelu_cmp = 2 * p - 1 - share[j];
        } else {
          input_drelu_cmp = p - 1 - share[j];
        }
        for (int i = 0; i < num_digits; i++) {  // Stored from LSB to MSB
          if ((i == num_digits - 1) &&
              (r != 0)) {  // A partially full last digit
            digits[i * num_relu * 2 + j * 2 + 0] =
                (uint8_t)(input_wrap_cmp >> i * beta) & mask_r;
            digits[i * num_relu * 2 + j * 2 + 1] =
                (uint8_t)(input_drelu_cmp >> i * beta) & mask_r;
          } else {
            digits[i * num_relu * 2 + j * 2 + 0] =
                (uint8_t)(input_wrap_cmp >> i * beta) & mask_beta;
            digits[i * num_relu * 2 + j * 2 + 1] =
                (uint8_t)(input_drelu_cmp >> i * beta) & mask_beta;
          }
        }
      }
    } else  // party = sci::BOB
    {
      for (int j = 0; j < num_relu; j++) {
        uint64_t input_cmp = p_2 + share[j];
        for (int i = 0; i < num_digits; i++) {  // Stored from LSB to MSB
          if ((i == num_digits - 1) &&
              (r != 0)) {  // A partially full last digit
            digits[i * num_relu + j * 1 + 0] =
                (uint8_t)(input_cmp >> i * beta) & mask_r;
          } else {
            digits[i * num_relu + j * 1 + 0] =
                (uint8_t)(input_cmp >> i * beta) & mask_beta;
          }
        }
      }
    }

    if (party == sci::ALICE) {

      uint8_t *
          *leaf_ot_messages;  // (num_digits * num_relu) X beta_pow (=2^beta)
      // Do the two relu_comparisons together in 1 leaf OT.
      leaf_ot_messages = new uint8_t *[num_digits * num_relu];
      for (int i = 0; i < num_digits * num_relu; i++)
        leaf_ot_messages[i] = new uint8_t[beta_pow];

      // Set Leaf OT messages
      triple_gen->prg->random_bool((bool *)leaf_res_cmp,
                                   num_digits * num_relu * 2);
      triple_gen->prg->random_bool((bool *)leaf_res_eq,
                                   num_digits * num_relu * 2);
      for (int i = 0; i < num_digits; i++) {
        for (int j = 0; j < num_relu; j++) {
          if (i == 0) {
            set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                 digits + i * num_relu * 2 + j * 2, beta_pow,
                                 leaf_res_cmp + i * num_relu * 2 + j * 2, 0,
                                 false);
          } else if (i == (num_digits - 1) && (r > 0)) {
#if defined(WAN_EXEC) || USE_CHEETAH
            set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                 digits + i * num_relu * 2 + j * 2, beta_pow,
                                 leaf_res_cmp + i * num_relu * 2 + j * 2,
                                 leaf_res_eq + i * num_relu * 2 + j * 2);
#else
            set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                 digits + i * num_relu * 2 + j * 2, 1 << r,
                                 leaf_res_cmp + i * num_relu * 2 + j * 2,
                                 leaf_res_eq + i * num_relu * 2 + j * 2);
#endif
          } else {
            set_leaf_ot_messages(leaf_ot_messages[i * num_relu + j],
                                 digits + i * num_relu * 2 + j * 2, beta_pow,
                                 leaf_res_cmp + i * num_relu * 2 + j * 2,
                                 leaf_res_eq + i * num_relu * 2 + j * 2);
          }
        }
      }

      // Perform Leaf OTs

      // Each ReLU has the first digit of which equality is not required.
#if defined(WAN_EXEC) || USE_CHEETAH
      otpack->kkot[beta - 1]->send(leaf_ot_messages, num_relu * (num_digits),
                                   2 * 2);
#else
      otpack->kkot[beta - 1]->send(leaf_ot_messages, num_relu, 1 * 2);
      if (r == 1) {
        // For the last digit (MSB), use IKNP because it is just 1-bit
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                     num_relu * (num_digits - 2), 2 * 2);
        otpack->iknp_straight->send(
            leaf_ot_messages + num_relu * (num_digits - 1), num_relu, 2 * 2);
      } else if (r != 0) {
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                     num_relu * (num_digits - 2), 2 * 2);
        if (r == 2) {
          otpack->kkot[1]->send(leaf_ot_messages + num_relu * (num_digits - 1),
                                num_relu, 2 * 2);
        } else if (r == 3) {
          otpack->kkot[2]->send(leaf_ot_messages + num_relu * (num_digits - 1),
                                num_relu, 2 * 2);
        } else if (r == 4) {
          otpack->kkot[3]->send(leaf_ot_messages + num_relu * (num_digits - 1),
                                num_relu, 2 * 2);
        } else {
          throw std::invalid_argument("Not yet implemented!");
        }
      } else
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_relu,
                                     num_relu * (num_digits - 1), 2 * 2);
#endif
      // Cleanup
      for (int i = 0; i < num_digits * num_relu; i++)
        delete[] leaf_ot_messages[i];
      delete[] leaf_ot_messages;
      // Alice's shares are: cmp1, cmp2 and cmp3 are at leaf_res_cmp[0+3*j],
      // leaf_res_cmp[1+3*j] and leaf_res_cmp[2+3*j]
    } else  // party = sci::BOB
    {
      // Perform Leaf OTs
      uint8_t *leaf_ot_recvd = new uint8_t[num_digits * num_relu];
#if defined(WAN_EXEC) || USE_CHEETAH
      otpack->kkot[beta - 1]->recv(leaf_ot_recvd, digits,
                                   num_relu * (num_digits), 2 * 2);
#else
      otpack->kkot[beta - 1]->recv(leaf_ot_recvd, digits, num_relu, 1 * 2);
      if (r == 1) {
        otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                     digits + num_relu,
                                     num_relu * (num_digits - 2), 2 * 2);
        otpack->iknp_straight->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                    digits + num_relu * (num_digits - 1),
                                    num_relu, 2 * 2);
      } else if (r != 0) {
        otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                     digits + num_relu,
                                     num_relu * (num_digits - 2), 2 * 2);
        if (r == 2) {
          otpack->kkot[1]->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                digits + num_relu * (num_digits - 1), num_relu,
                                2 * 2);
        } else if (r == 3) {
          otpack->kkot[2]->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                digits + num_relu * (num_digits - 1), num_relu,
                                2 * 2);
        } else if (r == 4) {
          otpack->kkot[3]->recv(leaf_ot_recvd + num_relu * (num_digits - 1),
                                digits + num_relu * (num_digits - 1), num_relu,
                                2 * 2);
        } else {
          throw std::invalid_argument("Not yet implemented!");
        }
      } else
        otpack->kkot[beta - 1]->recv(leaf_ot_recvd + num_relu,
                                     digits + num_relu,
                                     num_relu * (num_digits - 1), 2 * 2);
#endif

      // Extract equality result from leaf_res_cmp
      for (int i = 0; i < num_relu; i++) {
        leaf_res_cmp[2 * i] = (leaf_ot_recvd[i] & (1 << 1)) >> 1;
        leaf_res_cmp[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 0)) >> 0;
      }
      for (int i = num_relu; i < num_digits * num_relu; i++) {
        leaf_res_cmp[2 * i] = (leaf_ot_recvd[i] & (1 << 3)) >> 3;
        leaf_res_cmp[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 2)) >> 2;

        leaf_res_eq[2 * i] = (leaf_ot_recvd[i] & (1 << 1)) >> 1;
        leaf_res_eq[2 * i + 1] = (leaf_ot_recvd[i] & (1 << 0)) >> 0;
      }
      delete[] leaf_ot_recvd;
    }

#if MILL_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> leaf_ot_time = end - (start + total_time);
    total_time += leaf_ot_time;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | LUT-cmp: " << std::setw(_w1) << leaf_ot_time.count() * 1000 << " ms" ;
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    uint64_t comm_bit_lt = io->counter - (comm_start + comm_total);
    comm_total += comm_bit_lt;
    // log_comm << "P" << party << " COMM";
    log_comm << " | LUT-cmp: " << std::setw(_w2) << double(comm_bit_lt) / 1024 << " KB";
    // log_comm << std::endl;
#endif

    // Generate required Bit-Triples and traverse tree to compute the results of
    // comparsions
    millionaire->traverse_and_compute_ANDs(num_cmps, leaf_res_eq, leaf_res_cmp);

#if MILL_PRINT_TIME
    // get running time of traverse_and_compute_ANDs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> and_time = end - (start + total_time);
    total_time += and_time;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | tcANDs: " << std::setw(_w1) << and_time.count() * 1000 << " ms";
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    uint64_t comm_tANDs = io->counter - (comm_start + comm_total);
    comm_total += comm_tANDs;
    // log_comm << "P" << party << " COMM";
    log_comm << " | tcANDs: " << std::setw(_w2) << double(comm_tANDs) / 1024 << " KB";
    // log_comm << std::endl;
#endif

    assert(num_relu % 8 == 0 && "Number of ReLUs should be a multiple of 8");

    if (party == sci::ALICE) {
      uint8_t **mux_ot_messages = new uint8_t *[num_relu];
      for (int j = 0; j < num_relu; j++) {
        mux_ot_messages[j] = new uint8_t[4];
      }
      triple_gen->prg->random_bool((bool *)drelu, num_relu);
      for (int j = 0; j < num_relu; j++) {
        // drelu[j] = 0;
        bool neg_share = (share[j] > p_2);
        set_mux_ot_messages(mux_ot_messages[j], leaf_res_cmp + j * 2, drelu[j],
                            neg_share);
      }
      otpack->kkot[1]->send(mux_ot_messages, num_relu, 1);
      for (int j = 0; j < num_relu; j++) {
        delete[] mux_ot_messages[j];
      }
      delete[] mux_ot_messages;
    } else  // sci::BOB
    {
      uint8_t *mux_ot_selection = new uint8_t[num_relu];
      for (int j = 0; j < num_relu; j++) {
        mux_ot_selection[j] =
            (leaf_res_cmp[j * 2 + 1] << 1) | leaf_res_cmp[j * 2];
      }
      otpack->kkot[1]->recv(drelu, mux_ot_selection, num_relu, 1);
      delete[] mux_ot_selection;
    }

#if MILL_PRINT_TIME
    // get running time of the entire function in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time_ = end - start;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | Total: " << std::setw(_w1) << total_time_.count() * 1000 << " ms";
    log_time << std::endl;
    this->triple_gen->addMillTime(total_time_.count());
#endif
#if MILL_PRINT_COMM
    uint64_t comm_total_1 = comm_total;
    // log_comm << "P" << party << " COMM";
    log_comm << " | Total: " << std::setw(_w2) << double(comm_total_1) / 1024 << " KB";
    log_comm << std::endl;
    this->triple_gen->addMillComm(comm_total_1);
#endif
#if MILL_PRINT_TIME
    std::cout << log_time.str();
#endif
#if MILL_PRINT_COMM  
    std::cout << log_comm.str();
#endif

    delete[] digits;
    delete[] leaf_res_cmp;
    delete[] leaf_res_eq;
  }

  void set_leaf_ot_messages(uint8_t *ot_messages, uint8_t *digits, int N,
                            uint8_t *additive_mask_cmp,
                            uint8_t *additive_mask_eq, bool eq = true) {
    for (int i = 0; i < N; i++) {
      if (eq) {
        ot_messages[i] = 0;
        // For comparisons
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[0] < i) ^ additive_mask_cmp[0]);
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[1] < i) ^ additive_mask_cmp[1]);
        // For equality
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[0] == i) ^ additive_mask_eq[0]);
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[1] == i) ^ additive_mask_eq[1]);
      } else {
        ot_messages[i] = 0;
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[0] < i) ^ additive_mask_cmp[0]);
        ot_messages[i] =
            (ot_messages[i] << 1) | ((digits[1] < i) ^ additive_mask_cmp[1]);
      }
    }
  }

  void set_mux_ot_messages(uint8_t *ot_messages, uint8_t *cmp_results,
                           uint8_t drelu_mask, bool neg_share) {
    uint8_t bits_i[2];
    for (int i = 0; i < 4; i++) {
      sci::uint8_to_bool(bits_i, i, 2);  // wrap_cmp || !drelu_cmp
      uint8_t drelu_cmp = bits_i[1] ^ cmp_results[1];
      uint8_t wrap = bits_i[0] ^ cmp_results[0];
      if (neg_share) {
        ot_messages[i] = ((1 ^ drelu_cmp) & wrap) ^ 1;
      } else {
        ot_messages[i] = drelu_cmp ^ (drelu_cmp & wrap);
      }
      ot_messages[i] ^= drelu_mask;
    }
  }


//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//                                                                                                    //
//                                            SecONNds                                                //
//                                                                                                    //
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//


  void compute_drelu_new(uint8_t *drelu, uint64_t *share, int num_relu) {
#if MILL_PRINT_TIME
    int _w1 = 8;
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    log_time << "-NEW-MILL-";
    log_time << "TIME";
#endif
#if MILL_PRINT_COMM
    int _w2 = 8;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    log_comm << "-NEW-MILL-";
    log_comm << "COMM";
#endif

    assert(beta == 1 && "SecONNds requires beta = 1");

    int num_cmps = 2 * num_relu;
    uint8_t *bit_res_cmp;  // l * num_cmps
    uint8_t *bit_res_eql;   // l * num_cmps
    bit_res_cmp = new uint8_t[l * num_cmps];
    bit_res_eql = new uint8_t[l * num_cmps];

    uint8_t *bits_rel0;        // l * num_relu
    uint8_t *bits_rel1;        // l * num_relu
    uint8_t *bit_rel0_cmp;  // l * num_cmps
    uint8_t *bit_rel1_cmp;  // l * num_cmps
    uint8_t *bit_rel0_eql;   // l * num_cmps
    uint8_t *bit_rel1_eql;   // l * num_cmps
    bits_rel0 = new uint8_t[l * num_relu];
    bits_rel1 = new uint8_t[l * num_relu];
    bit_rel0_cmp = new uint8_t[l * num_relu];
    bit_rel1_cmp = new uint8_t[l * num_relu];
    bit_rel0_eql = new uint8_t[l * num_relu];
    bit_rel1_eql = new uint8_t[l * num_relu];

    // Extract bits from data
    if (party == sci::ALICE) {
      for (int j = 0; j < num_relu; j++) {
        uint64_t input_wrap_cmp = (p - 1 - share[j]) + p_2;
        uint64_t input_drelu_cmp;
        if (share[j] > p_2) {
          input_drelu_cmp = 2 * p - 1 - share[j];
        } else {
          input_drelu_cmp = p - 1 - share[j];
        }
        for (int i = 0; i < l; i++) {  // Stored from LSB to MSB
          if ((i == l - 1) &&
              (r != 0)) {  // A partially full last digit
            bits_rel0[i * num_relu + j] =
                (uint8_t)(input_wrap_cmp >> i * beta) & mask_r;
            bits_rel1[i * num_relu + j] =
                (uint8_t)(input_drelu_cmp >> i * beta) & mask_r;
          } else {
            bits_rel0[i * num_relu + j] =
                (uint8_t)(input_wrap_cmp >> i * beta) & mask_beta;
            bits_rel1[i * num_relu + j] =
                (uint8_t)(input_drelu_cmp >> i * beta) & mask_beta;
          }
        }
      }
    } else  {// party = sci::BOB
      for (int j = 0; j < num_relu; j++) {
        uint64_t input_cmp = p_2 + share[j];
        for (int i = 0; i < l; i++) {  // Stored from LSB to MSB
          if ((i == l - 1) &&
              (r != 0)) {  // A partially full last digit
            bits_rel0[i * num_relu + j] =
                (uint8_t)(input_cmp >> i * beta) & mask_r;
          } else {
            bits_rel0[i * num_relu + j] =
                (uint8_t)(input_cmp >> i * beta) & mask_beta;
          }
        }
      }
    }

    // make secret shares of the input and share with the other party
    bool greater_than = false;
    uint64_t n_bits   = l * num_relu;
    uint64_t n_cmps_8 = num_relu >> 3;
    uint64_t n_bytes  = l * n_cmps_8;

    uint8_t *inp_bits_rel0_0 = new uint8_t [n_bits];
    uint8_t *inp_bits_rel1_0 = new uint8_t [n_bits];
    uint8_t *inp_bits_rel0_1 = new uint8_t [n_bits];
    uint8_t *inp_bits_rel1_1 = new uint8_t [n_bits];
    std::fill_n(inp_bits_rel0_0, n_bits, 0);
    std::fill_n(inp_bits_rel1_0, n_bits, 0);
    std::fill_n(inp_bits_rel0_1, n_bits, 0);
    std::fill_n(inp_bits_rel1_1, n_bits, 0);
    
    if (party == sci::ALICE) {
      for (int i = 0; i < l; i++) {
        for (int j = 0; j < num_relu; j++) {
          inp_bits_rel0_0[i*num_relu+j] = bits_rel0[i*num_relu+j];
          inp_bits_rel1_0[i*num_relu+j] = bits_rel1[i*num_relu+j];
          if(!greater_than){
            inp_bits_rel0_0[i*num_relu+j] ^= 1;
            inp_bits_rel1_0[i*num_relu+j] ^= 1;
          }
        }
      }
    } else { // party = sci::BOB
      for (int i = 0; i < l; i++) {
        for (int j = 0; j < num_relu; j++) {
          inp_bits_rel0_1[i*num_relu+j] = bits_rel0[i*num_relu+j];
          if(greater_than) inp_bits_rel0_1[i*num_relu+j] ^= 1;
        }
      }
    }

    millionaire->compute_bit_eql_cmp(num_relu, l, inp_bits_rel0_0, inp_bits_rel0_1, bit_rel0_eql, bit_rel0_cmp);
    millionaire->compute_bit_eql_cmp(num_relu, l, inp_bits_rel1_0, inp_bits_rel0_1, bit_rel1_eql, bit_rel1_cmp);

    for (int j = 0; j < num_relu; j++) {
      for (int i = 0; i < l; i++) {  // Stored from LSB to MSB
        bit_res_eql[i * num_relu * 2 + j * 2 + 0] = bit_rel0_eql[i * num_relu + j];
        bit_res_eql[i * num_relu * 2 + j * 2 + 1] = bit_rel1_eql[i * num_relu + j];
        bit_res_cmp[i * num_relu * 2 + j * 2 + 0] = bit_rel0_cmp[i * num_relu + j];
        bit_res_cmp[i * num_relu * 2 + j * 2 + 1] = bit_rel1_cmp[i * num_relu + j];
      }
    }

#if MILL_PRINT_TIME
    // get running time of Bit Comparisons in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> leaf_ot_time = end - (start + total_time);
    total_time += leaf_ot_time;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | Bit-cmp: " << std::setw(_w1) << leaf_ot_time.count() * 1000 << " ms" ;
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    uint64_t comm_bit_lt = io->counter - (comm_start + comm_total);
    comm_total += comm_bit_lt;
    // log_comm << "P" << party << " COMM";
    log_comm << " | Bit-cmp: " << std::setw(_w2) << double(comm_bit_lt) / 1024 << " KB";
    // log_comm << std::endl;
#endif

    // Generate required Bit-Triples and traverse tree to compute the results of
    // comparsions
    millionaire->traverse_and_compute_ANDs(num_cmps, bit_res_eql, bit_res_cmp);

#if MILL_PRINT_TIME
    // get running time of traverse_and_compute_ANDs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> and_time = end - (start + total_time);
    total_time += and_time;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | tcANDs: " << std::setw(_w1) << and_time.count() * 1000 << " ms";
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    uint64_t comm_tANDs = io->counter - (comm_start + comm_total);
    comm_total += comm_tANDs;
    // log_comm << "P" << party << " COMM";
    log_comm << " | tcANDs: " << std::setw(_w2) << double(comm_tANDs) / 1024 << " KB";
    // log_comm << std::endl;
#endif

    assert(num_relu % 8 == 0 && "Number of ReLUs should be a multiple of 8");

    if (party == sci::ALICE) {
      uint8_t **mux_ot_messages = new uint8_t *[num_relu];
      for (int j = 0; j < num_relu; j++) {
        mux_ot_messages[j] = new uint8_t[4];
      }
      triple_gen->prg->random_bool((bool *)drelu, num_relu);
      for (int j = 0; j < num_relu; j++) {
        // drelu[j] = 0;
        bool neg_share = (share[j] > p_2);
        set_mux_ot_messages(mux_ot_messages[j], bit_res_cmp + j * 2, drelu[j],
                            neg_share);
      }
      otpack->kkot[1]->send(mux_ot_messages, num_relu, 1);
      for (int j = 0; j < num_relu; j++) {
        delete[] mux_ot_messages[j];
      }
      delete[] mux_ot_messages;
    } else  // sci::BOB
    {
      uint8_t *mux_ot_selection = new uint8_t[num_relu];
      for (int j = 0; j < num_relu; j++) {
        mux_ot_selection[j] =
            (bit_res_cmp[j * 2 + 1] << 1) | bit_res_cmp[j * 2];
      }
      otpack->kkot[1]->recv(drelu, mux_ot_selection, num_relu, 1);
      delete[] mux_ot_selection;
    }

#if MILL_PRINT_TIME
    // get running time of the entire function in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time_ = end - start;
    // log_time << "P" << party << " TIME | " << f_tag;
    log_time << " | Total: " << std::setw(_w1) << total_time_.count() * 1000 << " ms";
    log_time << std::endl;
    this->triple_gen->addMillTime(total_time_.count());
#endif
#if MILL_PRINT_COMM
    uint64_t comm_total_1 = comm_total;
    // log_comm << "P" << party << " COMM";
    log_comm << " | Total: " << std::setw(_w2) << double(comm_total_1) / 1024 << " KB";
    log_comm << std::endl;
    this->triple_gen->addMillComm(comm_total_1);
#endif
#if MILL_PRINT_TIME
    std::cout << log_time.str();
#endif
#if MILL_PRINT_COMM  
    std::cout << log_comm.str();
#endif
    delete[] bit_res_cmp;
    delete[] bit_res_eql;
  }
};
#endif  // DRELU_FIELD_H__
