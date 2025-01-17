/*
Authors: Deevashwer Rathee, Mayank Rathee
Modified by Zhicong Huang

Copyright:
Copyright (c) 2021 Microsoft Research
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

#ifndef MILLIONAIRE_H__
#define MILLIONAIRE_H__
#include "Millionaire/bit-triple-generator.h"
#include "OT/emp-ot.h"
#include "utils/emp-tool.h"
#include <cmath>

#if MILL_PRINT_COMP || MILL_PRINT_COMM || MILL_PRINT_TIME
#include <iomanip>
#endif

#define MILL_PARAM 4
template <typename IO> class MillionaireProtocol {
public:
  IO *io = nullptr;
  sci::OTPack<IO> *otpack;
  TripleGenerator<IO> *triple_gen;
  bool del_trip_gen = false;
  int party;
  int l, r, log_alpha, beta, beta_pow;
  int num_digits, num_triples_corr, num_triples_std, log_num_digits;
  int num_triples;
  uint8_t mask_beta, mask_r;

  bool use_low_round = false;

  MillionaireProtocol(int party, IO *io, sci::OTPack<IO> *otpack,
                      int bitlength = 32, int radix_base = MILL_PARAM) {
    this->party = party;
    this->io = io;
    this->otpack = otpack;
    this->triple_gen = new TripleGenerator<IO>(party, io, otpack);
    del_trip_gen = true;
    configure(bitlength, radix_base);
  }

  
  MillionaireProtocol(int party, IO *io, sci::OTPack<IO> *otpack, 
                      TripleGenerator<IO> *triplegen,
                      bool use_low_round = false,
                      int bitlength = 32, int radix_base = MILL_PARAM) {
    this->party = party;
    this->io = io;
    this->otpack = otpack;
    this->triple_gen = triplegen;
    this->use_low_round = use_low_round;
    configure(bitlength, radix_base);
  }

  void configure(int bitlength, int radix_base = MILL_PARAM) {
    assert(radix_base <= 8);
    assert(bitlength <= 64);
    this->l = bitlength;
    this->beta = radix_base;

    this->num_digits = ceil((double)l / beta);
    this->r = l % beta;
    this->log_alpha = sci::bitlen(num_digits) - 1;
    this->log_num_digits = log_alpha + 1;
    this->num_triples_corr = 2 * num_digits - 2 - 2 * log_num_digits;
    this->num_triples_std = log_num_digits; // = log_alpha + 1 = sci::bitlen(ceil((double) bitlength / radix_base))
    this->num_triples = num_triples_std + num_triples_corr; 
    /* 
    num_triples  = sci::bitlen(ceil((double) bitlength)) + 2 * bitlength - 2 - 2 * sci::bitlen(sci::bitlen(ceil((double) bitlength)))
                 = 2 * bitlength - 2 - sci::bitlen(sci::bitlen(ceil((double) bitlength)))
    */
    if (beta == 8)
      this->mask_beta = -1;
    else
      this->mask_beta = (1 << beta) - 1;
    this->mask_r = (1 << r) - 1;
    this->beta_pow = 1 << beta;
  }

  ~MillionaireProtocol() { 
    if (del_trip_gen) {
      delete this->triple_gen;
    }
  }

  void compare(uint8_t *res, uint64_t *data, int num_cmps, int bitlength,
               bool greater_than = true, bool equality = false,
               int radix_base = MILL_PARAM){
    if(this->triple_gen->isBufferEnabled()){
      compare_new(res, data, num_cmps, bitlength, greater_than, equality, radix_base);
    } else{
      compare_old(res, data, num_cmps, bitlength, greater_than, equality, radix_base);
    }
  }
  
  void traverse_and_compute_ANDs(int num_cmps, uint8_t *leaf_res_eq,
                                 uint8_t *leaf_res_cmp){
    if(this->triple_gen->isBufferEnabled()){
      traverse_and_compute_ANDs_new(num_cmps, leaf_res_eq, leaf_res_cmp);
    } else{
      traverse_and_compute_ANDs_old(num_cmps, leaf_res_eq, leaf_res_cmp);
    }
  }

  void compare_old(uint8_t *res, uint64_t *data, int num_cmps, int bitlength,
               bool greater_than = true, bool equality = false,
               int radix_base = MILL_PARAM) {
    
    configure(bitlength, radix_base);

#if MILL_PRINT_TIME
    int _w1 = 8;
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    // log_time << "P" << party;
    log_time << "-OLD-MILL-";
    log_time << "TIME";
    // log_time << ": num_cmps = " << num_cmps;
    // log_time << ", bitlength = " << bitlength;
    // log_time << ", greater_than = " << std::boolalpha << greater_than;
    // log_time << ", equality = " << std::boolalpha << equality;
    // log_time << ", radix_base = " << radix_base;
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    int _w2 = 8;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    // log_comm << "P" << party;
    log_comm << "-OLD-MILL-";
    log_comm << "COMM";
    // log_comm << ": num_cmps = " << num_cmps;
    // log_comm << ", bitlength = " << bitlength;
    // log_comm << ", greater_than = " << std::boolalpha << greater_than;
    // log_comm << ", equality = " << std::boolalpha << equality;
    // log_comm << ", radix_base = " << radix_base;
    // log_comm << std::endl;
#endif

    if (bitlength <= beta) {
      uint8_t N = 1 << bitlength;
      uint8_t mask = N - 1;
      if (party == sci::ALICE) {
        sci::PRG128 prg;
        prg.random_data(res, num_cmps * sizeof(uint8_t));
        uint8_t **leaf_messages = new uint8_t *[num_cmps];
        for (int i = 0; i < num_cmps; i++) {
          res[i] &= 1;
          leaf_messages[i] = new uint8_t[N];
          for (int j = 0; j < N; j++) {
            if (greater_than) {
              leaf_messages[i][j] = ((uint8_t(data[i] & mask) > j) ^ res[i]);
            } else {
              leaf_messages[i][j] = ((uint8_t(data[i] & mask) < j) ^ res[i]);
            }
          }
        }
        if (bitlength > 1) {
          otpack->kkot[bitlength - 1]->send(leaf_messages, num_cmps, 1);
        } else {
          otpack->iknp_straight->send(leaf_messages, num_cmps, 1);
        }

        for (int i = 0; i < num_cmps; i++)
          delete[] leaf_messages[i];
        delete[] leaf_messages;
      } else { // party == BOB
        uint8_t *choice = new uint8_t[num_cmps];
        for (int i = 0; i < num_cmps; i++) {
          choice[i] = data[i] & mask;
        }
        if (bitlength > 1) {
          otpack->kkot[bitlength - 1]->recv(res, choice, num_cmps, 1);
        } else {
          otpack->iknp_straight->recv(res, choice, num_cmps, 1);
        }

        delete[] choice;
      }
      return;
    }

    int old_num_cmps = num_cmps;
    // num_cmps should be a multiple of 8
    num_cmps = ceil(num_cmps / 8.0) * 8;

    uint64_t *data_ext;
    if (old_num_cmps == num_cmps)
      data_ext = data;
    else {
      data_ext = new uint64_t[num_cmps];
      memcpy(data_ext, data, old_num_cmps * sizeof(uint64_t));
      memset(data_ext + old_num_cmps, 0,
             (num_cmps - old_num_cmps) * sizeof(uint64_t));
    }

    uint8_t *digits;       // num_digits * num_cmps
    uint8_t *leaf_res_cmp; // num_digits * num_cmps
    uint8_t *leaf_res_eq;  // num_digits * num_cmps

    digits = new uint8_t[num_digits * num_cmps];
    leaf_res_cmp = new uint8_t[num_digits * num_cmps];
    leaf_res_eq = new uint8_t[num_digits * num_cmps];

    // Extract radix-digits from data
    for (int i = 0; i < num_digits; i++){ // Stored from LSB to MSB
      for (int j = 0; j < num_cmps; j++){
        if ((i == num_digits - 1) && (r != 0))
          digits[i * num_cmps + j] =
              (uint8_t)(data_ext[j] >> i * beta) & mask_r;
        else
          digits[i * num_cmps + j] =
              (uint8_t)(data_ext[j] >> i * beta) & mask_beta;
      }
    }


    if (party == sci::ALICE) {
      uint8_t *
          *leaf_ot_messages; // (num_digits * num_cmps) X beta_pow (=2^beta)
      leaf_ot_messages = new uint8_t *[num_digits * num_cmps];
      for (int i = 0; i < num_digits * num_cmps; i++)
        leaf_ot_messages[i] = new uint8_t[beta_pow];

      // Set Leaf OT messages
      triple_gen->prg->random_bool((bool *)leaf_res_cmp, num_digits * num_cmps);
      triple_gen->prg->random_bool((bool *)leaf_res_eq, num_digits * num_cmps);

      for (int i = 0; i < num_digits; i++) {
        for (int j = 0; j < num_cmps; j++) {
          if (i == 0) {
            set_leaf_ot_messages(leaf_ot_messages[i * num_cmps + j],
                                 digits[i * num_cmps + j], beta_pow,
                                 leaf_res_cmp[i * num_cmps + j], 0,
                                 greater_than, false);
          } else if (i == (num_digits - 1) && (r > 0)) {
#if defined(WAN_EXEC) || USE_CHEETAH
            set_leaf_ot_messages(leaf_ot_messages[i * num_cmps + j],
                                 digits[i * num_cmps + j], beta_pow,
                                 leaf_res_cmp[i * num_cmps + j],
                                 leaf_res_eq[i * num_cmps + j], greater_than);
#else
            set_leaf_ot_messages(leaf_ot_messages[i * num_cmps + j],
                                 digits[i * num_cmps + j], 1 << r,
                                 leaf_res_cmp[i * num_cmps + j],
                                 leaf_res_eq[i * num_cmps + j], greater_than);
#endif
          } else {
            set_leaf_ot_messages(leaf_ot_messages[i * num_cmps + j],
                                 digits[i * num_cmps + j], beta_pow,
                                 leaf_res_cmp[i * num_cmps + j],
                                 leaf_res_eq[i * num_cmps + j], greater_than);
          }
        }
      }

      // Perform Leaf OTs
#if defined(WAN_EXEC) || USE_CHEETAH
      // otpack->kkot_beta->send(leaf_ot_messages, num_cmps*(num_digits), 2);
      otpack->kkot[beta - 1]->send(leaf_ot_messages, num_cmps * (num_digits),
                                   2);
#else
      // otpack->kkot_beta->send(leaf_ot_messages, num_cmps, 1);
      otpack->kkot[beta - 1]->send(leaf_ot_messages, num_cmps, 1);
      if (r == 1) {
        // otpack->kkot_beta->send(leaf_ot_messages+num_cmps,
        // num_cmps*(num_digits-2), 2);
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_cmps,
                                     num_cmps * (num_digits - 2), 2);
        otpack->iknp_straight->send(
            leaf_ot_messages + num_cmps * (num_digits - 1), num_cmps, 2);
      } else if (r != 0) {
        // otpack->kkot_beta->send(leaf_ot_messages+num_cmps,
        // num_cmps*(num_digits-2), 2);
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_cmps,
                                     num_cmps * (num_digits - 2), 2);
        otpack->kkot[r - 1]->send(
            leaf_ot_messages + num_cmps * (num_digits - 1), num_cmps, 2);
        /*
                            if(r == 2){
                                    otpack->kkot_4->send(leaf_ot_messages+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else if(r == 3){
                                    otpack->kkot_8->send(leaf_ot_messages+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else if(r == 4){
                                    otpack->kkot_16->send(leaf_ot_messages+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else{
                                    throw std::invalid_argument("Not yet
           implemented!");
                            }
        */
      } else {
        // otpack->kkot_beta->send(leaf_ot_messages+num_cmps,
        // num_cmps*(num_digits-1), 2);
        otpack->kkot[beta - 1]->send(leaf_ot_messages + num_cmps,
                                     num_cmps * (num_digits - 1), 2);
      }
#endif

      // Cleanup
      for (int i = 0; i < num_digits * num_cmps; i++)
        delete[] leaf_ot_messages[i];
      delete[] leaf_ot_messages;
    } else // party = sci::BOB
    {
      // Perform Leaf OTs
#if defined(WAN_EXEC) || USE_CHEETAH
      // otpack->kkot_beta->recv(leaf_res_cmp, digits, num_cmps*(num_digits),
      // 2);
      otpack->kkot[beta - 1]->recv(leaf_res_cmp, digits,
                                   num_cmps * (num_digits), 2);
#else
      // otpack->kkot_beta->recv(leaf_res_cmp, digits, num_cmps, 1);
      otpack->kkot[beta - 1]->recv(leaf_res_cmp, digits, num_cmps, 1);
      if (r == 1) {
        // otpack->kkot_beta->recv(leaf_res_cmp+num_cmps, digits+num_cmps,
        // num_cmps*(num_digits-2), 2);
        otpack->kkot[beta - 1]->recv(leaf_res_cmp + num_cmps, digits + num_cmps,
                                     num_cmps * (num_digits - 2), 2);
        otpack->iknp_straight->recv(leaf_res_cmp + num_cmps * (num_digits - 1),
                                    digits + num_cmps * (num_digits - 1),
                                    num_cmps, 2);
      } else if (r != 0) {
        // otpack->kkot_beta->recv(leaf_res_cmp+num_cmps, digits+num_cmps,
        // num_cmps*(num_digits-2), 2);
        otpack->kkot[beta - 1]->recv(leaf_res_cmp + num_cmps, digits + num_cmps,
                                     num_cmps * (num_digits - 2), 2);
        otpack->kkot[r - 1]->recv(leaf_res_cmp + num_cmps * (num_digits - 1),
                                  digits + num_cmps * (num_digits - 1),
                                  num_cmps, 2);
        /*
                            if(r == 2){
                                    otpack->kkot_4->recv(leaf_res_cmp+num_cmps*(num_digits-1),
                                                    digits+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else if(r == 3){
                                    otpack->kkot_8->recv(leaf_res_cmp+num_cmps*(num_digits-1),
                                                    digits+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else if(r == 4){
                                    otpack->kkot_16->recv(leaf_res_cmp+num_cmps*(num_digits-1),
                                                    digits+num_cmps*(num_digits-1),
           num_cmps, 2);
                            }
                            else{
                                    throw std::invalid_argument("Not yet
           implemented!");
                            }
        */
      } else {
        // otpack->kkot_beta->recv(leaf_res_cmp+num_cmps, digits+num_cmps,
        // num_cmps*(num_digits-1), 2);
        otpack->kkot[beta - 1]->recv(leaf_res_cmp + num_cmps, digits + num_cmps,
                                     num_cmps * (num_digits - 1), 2);
      }
#endif

      // Extract equality result from leaf_res_cmp
      for (int i = num_cmps; i < num_digits * num_cmps; i++) {
        leaf_res_eq[i] = leaf_res_cmp[i] & 1;
        leaf_res_cmp[i] >>= 1;
      }
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

    traverse_and_compute_ANDs(num_cmps, leaf_res_eq, leaf_res_cmp);

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

    for (int i = 0; i < old_num_cmps; i++)
      res[i] = leaf_res_cmp[i];

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

    // Cleanup
    if (old_num_cmps != num_cmps)
      delete[] data_ext;
    delete[] digits;
    delete[] leaf_res_cmp;
    delete[] leaf_res_eq;
  }

  void set_leaf_ot_messages(uint8_t *ot_messages, uint8_t digit, int N,
                            uint8_t mask_cmp, uint8_t mask_eq,
                            bool greater_than, bool eq = true) {
    for (int i = 0; i < N; i++) {
      if (greater_than) {
        ot_messages[i] = ((digit > i) ^ mask_cmp);
      } else {
        ot_messages[i] = ((digit < i) ^ mask_cmp);
      }
      if (eq) {
        ot_messages[i] = (ot_messages[i] << 1) | ((digit == i) ^ mask_eq);
      }
    }
  }

  /**************************************************************************************************
   *                         AND computation related functions
   **************************************************************************************************/

  void traverse_and_compute_ANDs_old(int num_cmps, uint8_t *leaf_res_eq,
                                 uint8_t *leaf_res_cmp) {
#if defined(WAN_EXEC) || USE_CHEETAH
    Triple triples_std((num_triples)*num_cmps, true);
#else
    Triple triples_corr(num_triples_corr * num_cmps, true, num_cmps);
    Triple triples_std(num_triples_std * num_cmps, true);
#endif
    // Generate required Bit-Triples
#if USE_CHEETAH
    triple_gen->generate(party, &triples_std, _2ROT);
#elif defined(WAN_EXEC)
    // std::cout<<"Running on WAN_EXEC; Skipping correlated triples"<<std::endl;
    triple_gen->generate(party, &triples_std, _16KKOT_to_4OT);
#else
    triple_gen->generate(party, &triples_corr, _8KKOT);
    triple_gen->generate(party, &triples_std, _16KKOT_to_4OT);
#endif
    // std::cout << "Bit Triples Generated" << std::endl;

    // Combine leaf OT results in a bottom-up fashion
    int counter_std = 0, old_counter_std = 0;
    int counter_corr = 0, old_counter_corr = 0;
    int counter_combined = 0, old_counter_combined = 0;
    uint8_t *ei = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *fi = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *e = new uint8_t[(num_triples * num_cmps) / 8];
    uint8_t *f = new uint8_t[(num_triples * num_cmps) / 8];

    for (int i = 1; i < num_digits; i *= 2) {
      for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i) {
        if (j == 0) {
#if defined(WAN_EXEC) || USE_CHEETAH
          AND_step_1(
              ei + (counter_std * num_cmps) / 8,
              fi + (counter_std * num_cmps) / 8, leaf_res_cmp + j * num_cmps,
              leaf_res_eq + (j + i) * num_cmps,
              (triples_std.ai) + (counter_combined * num_cmps) / 8,
              (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
          counter_std++;
          counter_combined++;
#else
          AND_step_1(ei + (counter_std * num_cmps) / 8,
                     fi + (counter_std * num_cmps) / 8,
                     leaf_res_cmp + j * num_cmps,
                     leaf_res_eq + (j + i) * num_cmps,
                     (triples_std.ai) + (counter_std * num_cmps) / 8,
                     (triples_std.bi) + (counter_std * num_cmps) / 8, num_cmps);
          counter_std++;
#endif
        } else {
#if defined(WAN_EXEC) || USE_CHEETAH
          AND_step_1(
              ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
              fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
              leaf_res_cmp + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
              (triples_std.ai) + (counter_combined * num_cmps) / 8,
              (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
          counter_combined++;
          AND_step_1(
              ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              leaf_res_eq + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
              (triples_std.ai) + (counter_combined * num_cmps) / 8,
              (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
          counter_combined++;
          counter_corr++;
#else
          AND_step_1(
              ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
              fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
              leaf_res_cmp + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
              (triples_corr.ai) + (2 * counter_corr * num_cmps) / 8,
              (triples_corr.bi) + (2 * counter_corr * num_cmps) / 8, num_cmps);
          AND_step_1(
              ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              leaf_res_eq + j * num_cmps, leaf_res_eq + (j + i) * num_cmps,
              (triples_corr.ai) + ((2 * counter_corr + 1) * num_cmps) / 8,
              (triples_corr.bi) + ((2 * counter_corr + 1) * num_cmps) / 8,
              num_cmps);
          counter_corr++;
#endif
        }
      }
      int offset_std = (old_counter_std * num_cmps) / 8;
      int size_std = ((counter_std - old_counter_std) * num_cmps) / 8;
      int offset_corr =
          ((num_triples_std + 2 * old_counter_corr) * num_cmps) / 8;
      int size_corr = (2 * (counter_corr - old_counter_corr) * num_cmps) / 8;

      if (party == sci::ALICE) {
        io->send_data(ei + offset_std, size_std);
        io->send_data(ei + offset_corr, size_corr);
        io->send_data(fi + offset_std, size_std);
        io->send_data(fi + offset_corr, size_corr);
        io->recv_data(e + offset_std, size_std);
        io->recv_data(e + offset_corr, size_corr);
        io->recv_data(f + offset_std, size_std);
        io->recv_data(f + offset_corr, size_corr);
      } else // party = sci::BOB
      {
        io->recv_data(e + offset_std, size_std);
        io->recv_data(e + offset_corr, size_corr);
        io->recv_data(f + offset_std, size_std);
        io->recv_data(f + offset_corr, size_corr);
        io->send_data(ei + offset_std, size_std);
        io->send_data(ei + offset_corr, size_corr);
        io->send_data(fi + offset_std, size_std);
        io->send_data(fi + offset_corr, size_corr);
      }
      for (int i = 0; i < size_std; i++) {
        e[i + offset_std] ^= ei[i + offset_std];
        f[i + offset_std] ^= fi[i + offset_std];
      }
      for (int i = 0; i < size_corr; i++) {
        e[i + offset_corr] ^= ei[i + offset_corr];
        f[i + offset_corr] ^= fi[i + offset_corr];
      }

      counter_std = old_counter_std;
      counter_corr = old_counter_corr;
#if defined(WAN_EXEC) || USE_CHEETAH
      counter_combined = old_counter_combined;
#endif
      for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i) {
        if (j == 0) {
#if defined(WAN_EXEC) || USE_CHEETAH
          AND_step_2(
              leaf_res_cmp + j * num_cmps, e + (counter_std * num_cmps) / 8,
              f + (counter_std * num_cmps) / 8,
              ei + (counter_std * num_cmps) / 8,
              fi + (counter_std * num_cmps) / 8,
              (triples_std.ai) + (counter_combined * num_cmps) / 8,
              (triples_std.bi) + (counter_combined * num_cmps) / 8,
              (triples_std.ci) + (counter_combined * num_cmps) / 8, num_cmps);
          counter_combined++;
#else
          AND_step_2(leaf_res_cmp + j * num_cmps,
                     e + (counter_std * num_cmps) / 8,
                     f + (counter_std * num_cmps) / 8,
                     ei + (counter_std * num_cmps) / 8,
                     fi + (counter_std * num_cmps) / 8,
                     (triples_std.ai) + (counter_std * num_cmps) / 8,
                     (triples_std.bi) + (counter_std * num_cmps) / 8,
                     (triples_std.ci) + (counter_std * num_cmps) / 8, num_cmps);
#endif
          for (int k = 0; k < num_cmps; k++)
            leaf_res_cmp[j * num_cmps + k] ^=
                leaf_res_cmp[(j + i) * num_cmps + k];
          counter_std++;
        } else {
#if defined(WAN_EXEC) || USE_CHEETAH
          AND_step_2(leaf_res_cmp + j * num_cmps,
                     e + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     f + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     (triples_std.ai) + (counter_combined * num_cmps) / 8,
                     (triples_std.bi) + (counter_combined * num_cmps) / 8,
                     (triples_std.ci) + (counter_combined * num_cmps) / 8,
                     num_cmps);
          counter_combined++;
          AND_step_2(
              leaf_res_eq + j * num_cmps,
              e + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              f + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              (triples_std.ai) + (counter_combined * num_cmps) / 8,
              (triples_std.bi) + (counter_combined * num_cmps) / 8,
              (triples_std.ci) + (counter_combined * num_cmps) / 8, num_cmps);
          counter_combined++;
#else
          AND_step_2(leaf_res_cmp + j * num_cmps,
                     e + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     f + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                     (triples_corr.ai) + (2 * counter_corr * num_cmps) / 8,
                     (triples_corr.bi) + (2 * counter_corr * num_cmps) / 8,
                     (triples_corr.ci) + (2 * counter_corr * num_cmps) / 8,
                     num_cmps);
          AND_step_2(
              leaf_res_eq + j * num_cmps,
              e + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              f + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
              (triples_corr.ai) + ((2 * counter_corr + 1) * num_cmps) / 8,
              (triples_corr.bi) + ((2 * counter_corr + 1) * num_cmps) / 8,
              (triples_corr.ci) + ((2 * counter_corr + 1) * num_cmps) / 8,
              num_cmps);
#endif
          for (int k = 0; k < num_cmps; k++)
            leaf_res_cmp[j * num_cmps + k] ^=
                leaf_res_cmp[(j + i) * num_cmps + k];
          counter_corr++;
        }
      }
      old_counter_std = counter_std;
      old_counter_corr = counter_corr;
#if defined(WAN_EXEC) || USE_CHEETAH
      old_counter_combined = counter_combined;
#endif
    }

#if defined(WAN_EXEC) || USE_CHEETAH
    assert(counter_combined == num_triples);
#else
    assert(counter_std == num_triples_std);
    assert(2 * counter_corr == num_triples_corr);
#endif

    // cleanup
    delete[] ei;
    delete[] fi;
    delete[] e;
    delete[] f;
  }

  void AND_step_1(uint8_t *ei, // evaluates batch of 8 ANDs
                  uint8_t *fi, uint8_t *xi, uint8_t *yi, uint8_t *ai,
                  uint8_t *bi, int num_ANDs) {
    assert(num_ANDs % 8 == 0);
    for (int i = 0; i < num_ANDs; i += 8) {
      ei[i / 8] = ai[i / 8];
      fi[i / 8] = bi[i / 8];
      ei[i / 8] ^= sci::bool_to_uint8(xi + i, 8);
      fi[i / 8] ^= sci::bool_to_uint8(yi + i, 8);
    }
  }
  void AND_step_2(uint8_t *zi, // evaluates batch of 8 ANDs
                  uint8_t *e, uint8_t *f, uint8_t *ei, uint8_t *fi, uint8_t *ai,
                  uint8_t *bi, uint8_t *ci, int num_ANDs) {
    assert(num_ANDs % 8 == 0);
    for (int i = 0; i < num_ANDs; i += 8) {
      uint8_t temp_z;
      if (party == sci::ALICE)
        temp_z = e[i / 8] & f[i / 8];
      else
        temp_z = 0;
      temp_z ^= f[i / 8] & ai[i / 8];
      temp_z ^= e[i / 8] & bi[i / 8];
      temp_z ^= ci[i / 8];
      sci::uint8_to_bool(zi + i, temp_z, 8);
    }
  }
    

//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//                                                                                                    //
//                                            SecONNds                                                //
//                                                                                                    //
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//
//,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"^~,_,~^"//



  // TESTING SecONNds
  void compare_new(uint8_t *res, uint64_t *data, int num_cmps, int bitlength,
                bool greater_than = true, bool equality = false,
                int radix_base = MILL_PARAM){
    
    //~~~~~~~~~~~~~ SecONNds ~~~~~~~~~~~~~~~

    configure(bitlength, 1);

#if MILL_PRINT_TIME
    int _w1 = 8;
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    // log_time << "P" << party;
    log_time << "-NEW-MILL-";
    log_time << "TIME";
    // log_time << ": num_cmps = " << num_cmps;
    // log_time << ", bitlength = " << bitlength;
    // log_time << ", greater_than = " << std::boolalpha << greater_than;
    // log_time << ", equality = " << std::boolalpha << equality;
    // log_time << ", radix_base = " << radix_base;
    // log_time << std::endl;
#endif
#if MILL_PRINT_COMM
    int _w2 = 8;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    // log_comm << "P" << party;
    log_comm << "-NEW-MILL-";
    log_comm << "COMM";
    // log_comm << ": num_cmps = " << num_cmps;
    // log_comm << ", bitlength = " << bitlength;
    // log_comm << ", greater_than = " << std::boolalpha << greater_than;
    // log_comm << ", equality = " << std::boolalpha << equality;
    // log_comm << ", radix_base = " << radix_base;
    // log_comm << std::endl;
#endif
#if MILL_PRINT_COMP
    int _w1 = 2;
    std::stringstream log_ss;
    std::string f_tag1 = "MILL";
    std::string party_str = (party == sci::ALICE) ? "ALI" : "BOB";
    log_ss << f_tag1 << " | " << party_str 
      << " | staring with"
      << ": num_cmps = " << std::setw(_w1) << num_cmps 
      << ", bitlength = " << std::setw(_w1) << bitlength
      << ", greater_than = " << std::boolalpha << greater_than
      << ", equality = " << std::boolalpha << equality
      << ", radix_base = " << std::setw(_w1) << radix_base
      << std::endl;
#endif

    int old_num_cmps = num_cmps;
    // num_cmps should be a multiple of 8
    num_cmps = ceil(num_cmps / 8.0) * 8;

    uint64_t *data_ext;
    if (old_num_cmps == num_cmps)
      data_ext = data;
    else {
      data_ext = new uint64_t[num_cmps];
      memcpy(data_ext, data, old_num_cmps * sizeof(uint64_t));
      memset(data_ext + old_num_cmps, 0,
             (num_cmps - old_num_cmps) * sizeof(uint64_t));
    }

    // make secret shares of the input and share with the other party
    uint64_t n_bits   = bitlength * num_cmps;
    uint64_t n_cmps_8 = num_cmps >> 3;
    uint64_t n_bytes  = bitlength * n_cmps_8;

    uint8_t *inp_bits_0 = new uint8_t [n_bits];
    uint8_t *inp_bits_1 = new uint8_t [n_bits];
    std::fill_n(inp_bits_0, n_bits, 0);
    std::fill_n(inp_bits_1, n_bits, 0);
    
    if (party == sci::ALICE) {
      for (int i = 0; i < bitlength; i++) {
        for (int j = 0; j < num_cmps; j++) {
          inp_bits_0[i*num_cmps+j] = (data_ext[j] >> i) & 1;
          if(!greater_than) inp_bits_0[i*num_cmps+j] ^= 1;
        }
      }
    } else { // party = sci::BOB
      for (int i = 0; i < bitlength; i++) {
        for (int j = 0; j < num_cmps; j++) {
          inp_bits_1[i*num_cmps+j] = (data_ext[j] >> i) & 1;
          if(greater_than) inp_bits_1[i*num_cmps+j] ^= 1;
        }
      }
    }

#if MILL_PRINT_COMP
    // get BOB's inp_bits_0 for debugging
    uint8_t *inp_bits_0_ali = new uint8_t [n_bits];
    uint8_t *inp_bits_1_bob = new uint8_t [n_bits];
    if (party == sci::BOB){
      io->send_data(inp_bits_1, n_bits);
      log_ss << f_tag1 << " | " << party_str << " inp_bits_1:" << std::endl;
      for (int i = 0; i < bitlength; i++){
        for (int j = 0; j < num_cmps; j++)
            log_ss << std::setw(12) << (uint64_t) ((inp_bits_1[i*num_cmps+j]) & 1ULL) << " ";
        log_ss << std::endl;
      }
    } 
    else { //party == sci::ALICE
      io->recv_data(inp_bits_1_bob, n_bits);
      log_ss << f_tag1 << " | " << party_str << " | ALI inp_bits_0:" << std::endl;
      for (int i = 0; i < bitlength; i++){
        for (int j = 0; j < num_cmps; j++){
            inp_bits_0_ali[i*num_cmps+j] = inp_bits_0[i*num_cmps+j];
            log_ss << std::setw(12) << (uint64_t) ((inp_bits_0_ali[i*num_cmps+j]) & 1ULL) << " ";
          }
        log_ss << std::endl;
      }
      log_ss << f_tag1 << " | " << party_str << " | BOB inp_bits_1:" << std::endl;
      for (int i = 0; i < bitlength; i++){
        for (int j = 0; j < num_cmps; j++)
            log_ss << std::setw(12) << (uint64_t) ((inp_bits_1_bob[i*num_cmps+j]) & 1ULL) << " ";
        log_ss << std::endl;
      }
    }
#endif

    uint8_t *bit_res_eql = new uint8_t[bitlength * num_cmps];
    uint8_t *bit_res_cmp = new uint8_t[n_bits];

    compute_bit_eql_cmp(num_cmps, bitlength, inp_bits_0, inp_bits_1, bit_res_eql, bit_res_cmp);
    
/*
    Triple triples_cmp(bitlength*num_cmps, true);
    triple_gen->get(party, &triples_cmp);
    
    for (int i = 0; i < bitlength; i++) {
      for (int j = 0; j < n_cmps_8; j++) for (int k = 0; k < 8; k++) {
        bit_res_eql[i*num_cmps + (j<<3) + k] = inp_bits_0[i*num_cmps+(j<<3)+k] ^ inp_bits_1[i*num_cmps+(j<<3)+k];
      }
    }

    uint8_t *ei = new uint8_t[n_bytes];
    uint8_t *fi = new uint8_t[n_bytes];
    uint8_t *e  = new uint8_t[n_bytes];
    uint8_t *f  = new uint8_t[n_bytes];
  
    AND_step_1_new(ei , fi, inp_bits_0, inp_bits_1,
        (triples_cmp.ai), (triples_cmp.bi) , n_bits);
 
#if MILL_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> and_step1 = end - (start + total_time);
    total_time += and_step1;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | AND step 1 : " << and_step1.count() * 1000;
    log_time << " ms" << std::endl;
#endif

    // Communicate the computed ANDs
    if (party == sci::ALICE) {
      io->send_data(ei, n_bytes);
      io->send_data(fi, n_bytes);
      io->recv_data(e, n_bytes);
      io->recv_data(f, n_bytes);
    } else { // party = sci::BOB
      io->recv_data(e, n_bytes);
      io->recv_data(f, n_bytes);
      io->send_data(ei, n_bytes);
      io->send_data(fi, n_bytes);
    }      
    
#if MILL_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> comm_ands = end - (start + total_time);
    total_time += comm_ands;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | AND efeifi : " << comm_ands.count() * 1000;
    log_time << " ms" << std::endl;
#endif

    for (int i = 0; i < n_bytes; i++) {
      e[i] ^= ei[i];
      f[i] ^= fi[i];
    }

    AND_step_2_new(bit_res_cmp, e, f,
      (triples_cmp.ai), (triples_cmp.bi), (triples_cmp.ci), n_bits);

    delete[] ei;
    delete[] fi;
    delete[] e;
    delete[] f;

#if MILL_PRINT_TIME
    // get running time of leaf OTs in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> and_step2 = end - (start + total_time);
    total_time += and_step2;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | AND step 2 : " << and_step2.count() * 1000;
    log_time << " ms" << std::endl;
#endif
*/

#if MILL_PRINT_TIME
    // get running time of Bit comparisons in ms
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
#if MILL_PRINT_COMP
    // get BOB's inp_bits_0 for debugging
    if (party == sci::BOB){
      io->send_data(bit_res_cmp, bitlength * num_cmps);
    } 
    else { //party == sci::ALICE
      uint8_t *bit_cmp_res_bob = new uint8_t [bitlength * num_cmps];
      io->recv_data(bit_cmp_res_bob, bitlength * num_cmps);
      // log_ss << f_tag1 << " | " << party_str << " | "<<party_str<<" bit_res_cmp:" << std::endl;
      // for (int i = 0; i < bitlength; i++){
      //   for (int j = 0; j < num_cmps; j++)
      //       log_ss << std::setw(12) << (uint64_t) ((bit_res_cmp[i*num_cmps+j]) & 1ULL) << " ";
      //   log_ss << std::endl;
      // }
      // log_ss << f_tag1 << " | " << party_str << " | BOB bit_res_cmp:" << std::endl;
      // for (int i = 0; i < bitlength; i++){
      //   for (int j = 0; j < num_cmps; j++)
      //       log_ss << std::setw(12) << (uint64_t) ((bit_cmp_res_bob[i*num_cmps+j]) & 1ULL) << " ";
      //   log_ss << std::endl;
      // }
      log_ss << f_tag1 << " | " << party_str << " | XOR bit_res_cmp:" << std::endl;
      uint64_t _pass = 1;
      for (int i = 0; i < bitlength; i++){
        log_ss << "  ";
        for (int j = 0; j < num_cmps; j++){
          uint64_t xor_res = bit_res_cmp[i*num_cmps+j] ^ bit_cmp_res_bob[i*num_cmps+j];
          uint64_t inp_0 = inp_bits_0_ali[i*num_cmps+j]&1ULL;
          uint64_t inp_1 = inp_bits_1_bob[i*num_cmps+j]&1ULL;
          if (!greater_than){
            inp_0 ^= 1ULL;
          } else {
            inp_1 ^= 1ULL;
          }
          log_ss << std::setw(7) << xor_res << "~";
          log_ss << std::setw(1) << inp_0;
          log_ss << (greater_than ? ">" : "<");
          log_ss << std::setw(1) << inp_1;
          uint64_t _exp = greater_than ? (inp_0 > inp_1) : (inp_0 < inp_1);
          log_ss << "~" << std::setw(1) << _exp;
          _pass &= (xor_res == _exp);
        }
        log_ss << std::endl;
      }
      log_ss << f_tag1 << " | " << party_str 
        << " | " << (_pass ? "PASS!!!" : "FAIL!!!") 
        << std::endl;
    }
#endif

    traverse_and_compute_ANDs(num_cmps, bit_res_eql, bit_res_cmp);


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

    for (int i = 0; i < old_num_cmps; i++)
      res[i] = bit_res_cmp[i];

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
#if MILL_PRINT_COMP
    // get BOB's inputs and results for debugging
    if (party == sci::BOB){
      io->send_data(data, old_num_cmps * sizeof(uint64_t));
      io->send_data(res, old_num_cmps);
    } else { // party = sci::ALICE 
      uint64_t *data_bob = new uint64_t[old_num_cmps];
      uint8_t *res_bob = new uint8_t[old_num_cmps];
      io->recv_data(data_bob, old_num_cmps * sizeof(uint64_t));
      io->recv_data(res_bob, old_num_cmps);
      log_ss << f_tag1 << " | " << party_str << " | ALI inp:" << std::endl;
      for (int i = 0; i < old_num_cmps; i++) log_ss << std::setw(12) << data[i] << " "; log_ss << std::endl;
      log_ss << f_tag1 << " | " << party_str << " | BOB inp:" << std::endl;
      for (int i = 0; i < old_num_cmps; i++) log_ss << std::setw(12) << data_bob[i] << " "; log_ss << std::endl;
      
      log_ss << f_tag1 << " | " << party_str << " | XOR res:" << std::endl;
      uint64_t _pass = 1;
      for(int i = 0; i < old_num_cmps; i++){
        uint64_t xor_res = res[i] ^ res_bob[i];
        log_ss << std::setw(7) << xor_res << "~";
        uint64_t _exp = (greater_than ? (data[i] > data_bob[i]) : (data[i] < data_bob[i]));
        log_ss << std::setw(1) << _exp << "    ";
        _pass &= (xor_res == _exp);
      }
      log_ss << std::endl;

      log_ss << f_tag1 << " | " << party_str 
        << " | " << (_pass ? "PASS!!!" : "FAIL!!!") 
        << std::endl;
    }
#endif

#if MILL_PRINT_TIME
    std::cout << log_time.str();
#endif
#if MILL_PRINT_COMM  
    std::cout << log_comm.str();
#endif
#if MILL_PRINT_COMP 
    std::cout << log_ss.str();
    delete[] inp_bits_0_ali;
    delete[] inp_bits_1_bob;
#endif
    
    // Cleanup
    if (old_num_cmps != num_cmps)
      delete[] data_ext;

    delete[] inp_bits_0;
    delete[] inp_bits_1;
    delete[] bit_res_eql;
    delete[] bit_res_cmp;
  }

  // Computes bit-wise equals and less-than comparisons
  // between inp_bits_0 and inp_bits_1
  void compute_bit_eql_cmp(int num_cmps, int bitlength, 
      uint8_t *inp_bits_0, uint8_t *inp_bits_1, 
      uint8_t *bit_res_eql, uint8_t *bit_res_cmp){

    configure(bitlength, 1);
    uint64_t n_bits   = bitlength * num_cmps;
    uint64_t n_cmps_8 = num_cmps >> 3;
    uint64_t n_bytes  = bitlength * n_cmps_8;

    Triple triples_cmp(bitlength*num_cmps, true);
    triple_gen->get(party, &triples_cmp);

    for (int i = 0; i < bitlength; i++) {
      for (int j = 0; j < n_cmps_8; j++) for (int k = 0; k < 8; k++) {
        bit_res_eql[i*num_cmps + (j<<3) + k] = inp_bits_0[i*num_cmps+(j<<3)+k] ^ inp_bits_1[i*num_cmps+(j<<3)+k];
      }
    }

    uint8_t *ei = new uint8_t[n_bytes];
    uint8_t *fi = new uint8_t[n_bytes];
    uint8_t *e  = new uint8_t[n_bytes];
    uint8_t *f  = new uint8_t[n_bytes];
  
    AND_step_1_new(ei , fi, inp_bits_0, inp_bits_1,
        (triples_cmp.ai), (triples_cmp.bi) , n_bits);

    // Communicate the computed ANDs
    if (party == sci::ALICE) {
      io->send_data(ei, n_bytes);
      io->send_data(fi, n_bytes);
      io->recv_data(e, n_bytes);
      io->recv_data(f, n_bytes);
    } else { // party = sci::BOB
      io->recv_data(e, n_bytes);
      io->recv_data(f, n_bytes);
      io->send_data(ei, n_bytes);
      io->send_data(fi, n_bytes);
    }

    for (int i = 0; i < n_bytes; i++) {
      e[i] ^= ei[i];
      f[i] ^= fi[i];
    }


    AND_step_2_new(bit_res_cmp, e, f,
      (triples_cmp.ai), (triples_cmp.bi), (triples_cmp.ci), n_bits);

    delete[] ei;
    delete[] fi;
    delete[] e;
    delete[] f;
  }
  
  // Computes integer equals and less-than comparisons
  // using bit-wise comparisons from compute_bit_eql_cmp
  void traverse_and_compute_ANDs_new(int num_cmps, uint8_t *seg_res_eql, uint8_t *seg_res_cmp) {

#if MILL_PRINT_TIME
    // double time_start = emp::time::get_wall_time();
    // get current time using std::chrono::system_clock
    auto start = std::chrono::system_clock::now();
    auto end   = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time = end - start;
    std::stringstream log_time;
    std::string f_tag = "NEW | tcANDs";
    log_time << "P" << party << " TIME | ";
    log_time << f_tag;
    log_time << ": num_cmps = " << num_cmps;
    log_time << std::endl;
#endif
#if MILL_PRINT_COMP
    int _w1 = 2;
    std::stringstream log_ss;
    std::string f_tag1 = "tANDsT";
    std::string party_str = (party == sci::ALICE) ? "ALI" : "BOB";
    log_ss << f_tag1 << " | " << party_str
      << " | staring with"
      << ": num_cmps = " << std::setw(_w1) << num_cmps 
      << ", num_digits = " << std::setw(_w1) << num_digits
      << ", num_triples_std = " << std::setw(_w1) << num_triples_std
      << ", num_triples_corr = " << std::setw(_w1) << num_triples_corr
      << std::endl;
#endif
#if MILL_PRINT_COMM
    int _w2 = 10;
    std::stringstream log_comm;
    uint64_t comm_start = io->counter;
    uint64_t comm_total = 0;
    log_comm << "P" << party << " COMM | ";
    log_comm << "NEW | ANDsT ";
    log_comm << ": num_cmps = " << num_cmps;
    log_comm << std::endl;
#endif

    if(!this->use_low_round){
      int n_trips = num_digits - 1;
      Triple trips_seg(n_trips * num_cmps, true);
      triple_gen->get(party, &trips_seg);
    
#if MILL_PRINT_TIME
      // get running time of leaf OTs in ms
      end = std::chrono::system_clock::now();
      std::chrono::duration<double> trip_gen_time = end - (start + total_time);
      total_time += trip_gen_time;
      log_time << "P" << party << " TIME | ";
      log_time << f_tag << " | Triple GET : " << trip_gen_time.count() * 1000;
      log_time << " ms" << std::endl;
#endif
#if MILL_PRINT_COMP
      log_ss << f_tag1 << " | " << party_str 
        << " | SEG"
        << " | triples generate: trips_seg"
        << ", num_triples = " << std::setw(_w1) << trips_seg.num_triples 
        << ", num_bytes = " << std::setw(_w1) << trips_seg.num_bytes
        << ", offset = " << std::setw(_w1) << trips_seg.offset
        << ", packed = " << std::setw(_w1) << trips_seg.packed
        << std::endl;
#endif
#if MILL_PRINT_COMM
      uint64_t comm_trips = io->counter - (comm_start + comm_total);
      comm_total += comm_trips;
      log_comm << "P" << party << " COMM";
      log_comm << ": ANDtrp = " << std::setw(_w2) << comm_trips;
      log_comm << std::endl;
#endif

      for(int i = 0; i < n_trips; i++){
        uint64_t n_cmps_8 = num_cmps >> 3;

#if MILL_PRINT_COMP
      log_ss << f_tag1 << " | " << party_str 
        << " | SEG"
        << " | main loop iteration:"
        << ", i = " << std::setw(_w1) << i
        << ", n_cmps_8 = " << std::setw(_w1) << n_cmps_8
        << std::endl;
#endif 
        uint8_t *ei_ = new uint8_t[n_cmps_8];
        uint8_t *fi_ = new uint8_t[n_cmps_8];
        uint8_t *e_  = new uint8_t[n_cmps_8];
        uint8_t *f_  = new uint8_t[n_cmps_8];  

        AND_step_1_new(ei_ , fi_, 
                  seg_res_cmp + ( i      * num_cmps),
                  seg_res_eql + ((i + 1) * num_cmps),
                  (trips_seg.ai) + i * n_cmps_8,
                  (trips_seg.bi) + i * n_cmps_8, num_cmps);
#if MILL_PRINT_COMP
      log_ss << f_tag1 << " | " << party_str 
        << " | SEG"
        << " | AND_step_1 completed"
        << ": i = " << std::setw(_w1) << i 
        << std::endl;
#endif     
#if MILL_PRINT_COMP
      log_ss << f_tag1 << " | " << party_str 
        << " | starting comm"
        << ", i = " << std::setw(_w1) << i
        << ", n_cmps_8 = " << std::setw(_w1) << n_cmps_8
        << std::endl;
#endif   
        // Communicate the computed ANDs
        if (party == sci::ALICE) {
          io->send_data(ei_, n_cmps_8);
          io->send_data(fi_, n_cmps_8);
          io->recv_data(e_ , n_cmps_8);
          io->recv_data(f_ , n_cmps_8);
        } else // party = sci::BOB
        {
          io->recv_data(e_ , n_cmps_8);
          io->recv_data(f_ , n_cmps_8);
          io->send_data(ei_, n_cmps_8);
          io->send_data(fi_, n_cmps_8);
        }

#if MILL_PRINT_COMP
      log_ss << f_tag1 << " | " << party_str 
        << " | finished comm"
        << std::endl;
#endif        
      
        for (int i = 0; i < n_cmps_8; i++) {
          e_[i] ^= ei_[i];
          f_[i] ^= fi_[i];
        }

        uint8_t *lt_and_eq = new uint8_t[num_cmps];
        AND_step_2_new(lt_and_eq, e_, f_,
                  (trips_seg.ai) + i * n_cmps_8,
                  (trips_seg.bi) + i * n_cmps_8,
                  (trips_seg.ci) + i * n_cmps_8, num_cmps);

        for (int k = 0; k < num_cmps; k++)
            seg_res_cmp[(i + 1) * num_cmps + k] ^= lt_and_eq[k];
      }

      for (int k = 0; k < num_cmps; k++)
        seg_res_cmp[k] = seg_res_cmp[k + n_trips * num_cmps] & 1;


    } else { // use_low_round

      int n_trips = 2 * num_digits - 2 - sci::bitlen(sci::bitlen(ceil((double) num_digits)));

      Triple triples_std((n_trips)*num_cmps, true);

      // Generate required Bit-Triples
      triple_gen->get(party, &triples_std);

      // std::cout << "Bit Triples Generated" << std::endl;

      // Combine leaf OT results in a bottom-up fashion
      int counter_std = 0, old_counter_std = 0;
      int counter_corr = 0, old_counter_corr = 0;
      int counter_combined = 0, old_counter_combined = 0;
      uint8_t *ei = new uint8_t[(n_trips * num_cmps) / 8];
      uint8_t *fi = new uint8_t[(n_trips * num_cmps) / 8];
      uint8_t *e = new uint8_t[(n_trips * num_cmps) / 8];
      uint8_t *f = new uint8_t[(n_trips * num_cmps) / 8];

      for (int i = 1; i < num_digits; i *= 2) {
        for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i) {
          if (j == 0) {
            AND_step_1(
                ei + (counter_std * num_cmps) / 8,
                fi + (counter_std * num_cmps) / 8, seg_res_cmp + j * num_cmps,
                seg_res_eql + (j + i) * num_cmps,
                (triples_std.ai) + (counter_combined * num_cmps) / 8,
                (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
            counter_std++;
            counter_combined++;
          } else {
            AND_step_1(
                ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                seg_res_cmp + j * num_cmps, seg_res_eql + (j + i) * num_cmps,
                (triples_std.ai) + (counter_combined * num_cmps) / 8,
                (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
            counter_combined++;
            AND_step_1(
                ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                seg_res_eql + j * num_cmps, seg_res_eql + (j + i) * num_cmps,
                (triples_std.ai) + (counter_combined * num_cmps) / 8,
                (triples_std.bi) + (counter_combined * num_cmps) / 8, num_cmps);
            counter_combined++;
            counter_corr++;
          }
        }
        int offset_std = (old_counter_std * num_cmps) / 8;
        int size_std = ((counter_std - old_counter_std) * num_cmps) / 8;
        int offset_corr =
            ((num_triples_std + 2 * old_counter_corr) * num_cmps) / 8;
        int size_corr = (2 * (counter_corr - old_counter_corr) * num_cmps) / 8;

        if (party == sci::ALICE) {
          io->send_data(ei + offset_std, size_std);
          io->send_data(ei + offset_corr, size_corr);
          io->send_data(fi + offset_std, size_std);
          io->send_data(fi + offset_corr, size_corr);
          io->recv_data(e + offset_std, size_std);
          io->recv_data(e + offset_corr, size_corr);
          io->recv_data(f + offset_std, size_std);
          io->recv_data(f + offset_corr, size_corr);
        } else // party = sci::BOB
        {
          io->recv_data(e + offset_std, size_std);
          io->recv_data(e + offset_corr, size_corr);
          io->recv_data(f + offset_std, size_std);
          io->recv_data(f + offset_corr, size_corr);
          io->send_data(ei + offset_std, size_std);
          io->send_data(ei + offset_corr, size_corr);
          io->send_data(fi + offset_std, size_std);
          io->send_data(fi + offset_corr, size_corr);
        }
        for (int i = 0; i < size_std; i++) {
          e[i + offset_std] ^= ei[i + offset_std];
          f[i + offset_std] ^= fi[i + offset_std];
        }
        for (int i = 0; i < size_corr; i++) {
          e[i + offset_corr] ^= ei[i + offset_corr];
          f[i + offset_corr] ^= fi[i + offset_corr];
        }

        counter_std = old_counter_std;
        counter_corr = old_counter_corr;
        counter_combined = old_counter_combined;
        for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i) {
          if (j == 0) {
            AND_step_2(
                seg_res_cmp + j * num_cmps, e + (counter_std * num_cmps) / 8,
                f + (counter_std * num_cmps) / 8,
                ei + (counter_std * num_cmps) / 8,
                fi + (counter_std * num_cmps) / 8,
                (triples_std.ai) + (counter_combined * num_cmps) / 8,
                (triples_std.bi) + (counter_combined * num_cmps) / 8,
                (triples_std.ci) + (counter_combined * num_cmps) / 8, num_cmps);
            counter_combined++;

            for (int k = 0; k < num_cmps; k++)
              seg_res_cmp[j * num_cmps + k] ^=
                  seg_res_cmp[(j + i) * num_cmps + k];
            counter_std++;
          } else {
            AND_step_2(seg_res_cmp + j * num_cmps,
                      e + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                      f + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                      ei + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                      fi + ((num_triples_std + 2 * counter_corr) * num_cmps) / 8,
                      (triples_std.ai) + (counter_combined * num_cmps) / 8,
                      (triples_std.bi) + (counter_combined * num_cmps) / 8,
                      (triples_std.ci) + (counter_combined * num_cmps) / 8,
                      num_cmps);
            counter_combined++;
            AND_step_2(
                seg_res_eql + j * num_cmps,
                e + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                f + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                ei + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                fi + ((num_triples_std + (2 * counter_corr + 1)) * num_cmps) / 8,
                (triples_std.ai) + (counter_combined * num_cmps) / 8,
                (triples_std.bi) + (counter_combined * num_cmps) / 8,
                (triples_std.ci) + (counter_combined * num_cmps) / 8, num_cmps);
            counter_combined++;

            for (int k = 0; k < num_cmps; k++)
              seg_res_cmp[j * num_cmps + k] ^=
                  seg_res_cmp[(j + i) * num_cmps + k];
            counter_corr++;
          }
        }
        old_counter_std = counter_std;
        old_counter_corr = counter_corr;
        old_counter_combined = counter_combined;
      }
    }

#if MILL_PRINT_TIME
    // get running time of the entire function in ms
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> total_time_ = end - start;
    log_time << "P" << party << " TIME | ";
    log_time << f_tag << " | Total      : " << total_time_.count() * 1000;
    log_time << " ms" << std::endl;
#endif
#if MILL_PRINT_COMM
    uint64_t comm_efeifi = io->counter - (comm_start + comm_total);
    comm_total += comm_efeifi;
    log_comm << "P" << party << " COMM";
    log_comm << ": efeifi = " << std::setw(_w2) << comm_efeifi;
    log_comm << std::endl;
#endif
#if MILL_PRINT_COMM
    log_comm << "P" << party << " COMM";
    log_comm << ": ANDNEW = " << std::setw(_w2) << comm_total;
    log_comm << std::endl;
    std::cout << log_comm.str();
#endif
#if MILL_PRINT_COMP
    log_ss << f_tag1 << " | " << party_str 
      << " | FINISHED!"
      << std::endl;
    std::cout << log_ss.str();
#endif
#if MILL_PRINT_TIME
    std::cout << log_time.str();
#endif
  }

  void AND_step_1_new(uint8_t *ei, // evaluates batch of 8 ANDs
                  uint8_t *fi, 
                  uint8_t *xi, 
                  uint8_t *yi, 
                  uint8_t *ai,
                  uint8_t *bi, 
                  int num_ANDs) {
    assert(num_ANDs % 8 == 0);
    for (int i = 0, j = 0; i < num_ANDs; i += 8, j++) {
      ei[j] = ai[j];
      fi[j] = bi[j];
      ei[j] ^= sci::bool_to_uint8(xi + i, 8);
      fi[j] ^= sci::bool_to_uint8(yi + i, 8);
    }
  }
  
  void AND_step_2_new(uint8_t *zi, // evaluates batch of 8 ANDs
                  uint8_t *e, 
                  uint8_t *f,
                  uint8_t *ai,
                  uint8_t *bi, 
                  uint8_t *ci, 
                  int num_ANDs) {
    assert(num_ANDs % 8 == 0);
    for (int i = 0, j = 0; i < num_ANDs; i += 8, j++) {

      uint8_t temp_z;
      if (party == sci::ALICE)
        temp_z = e[j] & f[j];
      else
        temp_z = 0;
      temp_z ^= f[j] & ai[j];
      temp_z ^= e[j] & bi[j];
      temp_z ^= ci[j];
      sci::uint8_to_bool(zi + i, temp_z, 8);
    }
  }
};

#endif // MILLIONAIRE_H__
