/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AV1_COMMON_SCAN_H_
#define AV1_COMMON_SCAN_H_

#include "aom/aom_integer.h"
#include "aom_ports/mem.h"

#include "av1/common/enums.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NEIGHBORS 2

extern const SCAN_ORDER av1_default_scan_orders[TX_SIZES];
extern const SCAN_ORDER av1_intra_scan_orders[TX_SIZES_ALL][TX_TYPES];
extern const SCAN_ORDER av1_inter_scan_orders[TX_SIZES_ALL][TX_TYPES];

void av1_deliver_eob_threshold(const AV1_COMMON *cm, MACROBLOCKD *xd);

static INLINE int get_coef_context(const int16_t *neighbors,
                                   const uint8_t *token_cache, int c) {
  return (1 + token_cache[neighbors[MAX_NEIGHBORS * c + 0]] +
          token_cache[neighbors[MAX_NEIGHBORS * c + 1]]) >>
         1;
}

static INLINE const SCAN_ORDER *get_default_scan(TX_SIZE tx_size,
                                                 TX_TYPE tx_type,
                                                 int is_inter) {
  return is_inter ? &av1_inter_scan_orders[tx_size][tx_type]
                  : &av1_intra_scan_orders[tx_size][tx_type];
}

static INLINE const SCAN_ORDER *get_scan(const AV1_COMMON *cm, TX_SIZE tx_size,
                                         TX_TYPE tx_type,
                                         const MB_MODE_INFO *mbmi) {
  const int is_inter = is_inter_block(mbmi);
  (void)cm;
  return get_default_scan(tx_size, tx_type, is_inter);
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AV1_COMMON_SCAN_H_
