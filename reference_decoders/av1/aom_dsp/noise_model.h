/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_DSP_NOISE_MODEL_H_
#define AOM_DSP_NOISE_MODEL_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>

/*!\brief Wrapper of data required to represent linear system of eqns and soln.
 */
typedef struct {
  double *A;
  double *b;
  double *x;
  int n;
} aom_equation_system_t;

/*!\brief Representation of a piecewise linear curve
 *
 * Holds n points as (x, y) pairs, that store the curve.
 */
typedef struct {
  double (*points)[2];
  int num_points;
} aom_noise_strength_lut_t;

/*!\brief Init the noise strength lut with the given number of points*/
int aom_noise_strength_lut_init(aom_noise_strength_lut_t *lut, int num_points);

/*!\brief Frees the noise strength lut. */
void aom_noise_strength_lut_free(aom_noise_strength_lut_t *lut);

/*!\brief Evaluate the lut at the point x.
 *
 * \param[in] lut  The lut data.
 * \param[in] x    The coordinate to evaluate the lut.
 */
double aom_noise_strength_lut_eval(const aom_noise_strength_lut_t *lut,
                                   double x);

/*!\brief Helper struct to model noise strength as a function of intensity.
 *
 * Internally, this structure holds a representation of a linear system
 * of equations that models noise strength (standard deviation) as a
 * function of intensity. The mapping is initially stored using a
 * piecewise representation with evenly spaced bins that cover the entire
 * domain from [min_intensity, max_intensity]. Each observation (x,y) gives a
 * constraint of the form:
 *   y_{i} (1 - a) + y_{i+1} a = y
 * where y_{i} is the value of bin i and x_{i} <= x <= x_{i+1} and
 * a = x/(x_{i+1} - x{i}). The equation system holds the corresponding
 * normal equations.
 *
 * As there may be missing data, the solution is regularized to get a
 * complete set of values for the bins. A reduced representation after
 * solving can be obtained by getting the corresponding noise_strength_lut_t.
 */
typedef struct {
  aom_equation_system_t eqns;
  double min_intensity;
  double max_intensity;
  int num_bins;
  int num_equations;
  double total;
} aom_noise_strength_solver_t;

/*!\brief Initializes the noise solver with the given number of bins.
 *
 * Returns 0 if initialization fails.
 *
 * \param[in]  solver    The noise solver to be initialized.
 * \param[in]  num_bins  Number of bins to use in the internal representation.
 */
int aom_noise_strength_solver_init(aom_noise_strength_solver_t *solver,
                                   int num_bins);
void aom_noise_strength_solver_free(aom_noise_strength_solver_t *solver);

/*!\brief Gets the x coordinate of bin i.
 *
 * \param[in]  i  The bin whose coordinate to query.
 */
double aom_noise_strength_solver_get_center(
    const aom_noise_strength_solver_t *solver, int i);

/*!\brief Add an observation of the block mean intensity to its noise strength.
 *
 * \param[in]  block_mean  The average block intensity,
 * \param[in]  noise_std   The observed noise strength.
 */
void aom_noise_strength_solver_add_measurement(
    aom_noise_strength_solver_t *solver, double block_mean, double noise_std);

/*!\brief Solves the current set of equations for the noise strength. */
int aom_noise_strength_solver_solve(aom_noise_strength_solver_t *solver);

/*!\brief Fits a reduced piecewise linear lut to the internal solution
 *
 * \param[out] lut  The output piecewise linear lut.
 */
int aom_noise_strength_solver_fit_piecewise(
    const aom_noise_strength_solver_t *solver, aom_noise_strength_lut_t *lut);

/*!\brief Helper for holding precomputed data for finding flat blocks.
 *
 * Internally a block is modeled with a low-order polynomial model. A
 * planar model would be a bunch of equations like:
 * <[y_i x_i 1], [a_1, a_2, a_3]>  = b_i
 * for each point in the block. The system matrix A with row i as [y_i x_i 1]
 * is maintained as is the inverse, inv(A'*A), so that the plane parameters
 * can be fit for each block.
 */
typedef struct {
  double *AtA_inv;
  double *A;
  int num_params;  // The number of parameters used for internal low-order model
  int block_size;  // The block size the finder was initialized with
} aom_flat_block_finder_t;

/*!\brief Init the block_finder with the given block size */
int aom_flat_block_finder_init(aom_flat_block_finder_t *block_finder,
                               int block_size);
void aom_flat_block_finder_free(aom_flat_block_finder_t *block_finder);

/*!\brief Helper to extract a block and low order "planar" model. */
void aom_flat_block_finder_extract_block(
    const aom_flat_block_finder_t *block_finder, const uint8_t *const data,
    int w, int h, int stride, int offsx, int offsy, double *plane,
    double *block);

int aom_flat_block_finder_run(const aom_flat_block_finder_t *block_finder,
                              const uint8_t *const data, int w, int h,
                              int stride, uint8_t *flat_blocks);

// The noise shape indicates the allowed coefficients in the AR model.
typedef enum {
  AOM_NOISE_SHAPE_DIAMOND = 0,
  AOM_NOISE_SHAPE_SQUARE = 1
} aom_noise_shape;

// The parameters of the noise model include the shape type and the lag.
typedef struct {
  aom_noise_shape shape;
  int lag;
} aom_noise_model_params_t;

/*!\brief State of a noise model estimate for a single channel.
 *
 * This contains a system of equations that can be used to solve
 * for the auto-regressive coefficients as well as a noise strength
 * solver that can be used to model noise strength as a function of
 * intensity.
 */
typedef struct {
  aom_equation_system_t eqns;
  aom_noise_strength_solver_t strength_solver;
} aom_noise_state_t;

/*!\brief Complete model of noise for a planar video
 *
 * This includes a noise model for the latest frame and an aggregated
 * estimate over all previous frames that had similar parameters.
 */
typedef struct {
  aom_noise_model_params_t params;
  aom_noise_state_t combined_state[3];  // Combined state per channel
  aom_noise_state_t latest_state[3];    // Latest state per channel
  int (*coords)[2];  // Offsets (x,y) of the coefficient samples
  int n;             // Number of parameters (size of coords)
} aom_noise_model_t;

/*!\brief Result of a noise model update. */
typedef enum {
  AOM_NOISE_STATUS_OK = 0,
  AOM_NOISE_STATUS_INVALID_ARGUMENT,
  AOM_NOISE_STATUS_INSUFFICIENT_FLAT_BLOCKS,
  AOM_NOISE_STATUS_DIFFERENT_NOISE_TYPE,
  AOM_NOISE_STATUS_INTERNAL_ERROR,
} aom_noise_status_t;

/*!\brief Initializes a noise model with the given parameters.
 *
 * Returns 0 on failure.
 */
int aom_noise_model_init(aom_noise_model_t *model,
                         const aom_noise_model_params_t params);
void aom_noise_model_free(aom_noise_model_t *model);

/*!\brief Updates the noise model with a new frame observation.
 *
 * Updates the noise model with measurements from the given input frame and a
 * denoised variant of it. Noise is sampled from flat blocks using the flat
 * block map.
 *
 * Returns a noise_status indicating if the update was successful. If the
 * Update was successful, the combined_state is updated with measurements from
 * the provided frame. If status is OK or DIFFERENT_NOISE_TYPE, the latest noise
 * state will be updated with measurements from the provided frame.
 *
 * \param[in,out] noise_model     The noise model to be updated
 * \param[in]     data            Raw frame data
 * \param[in]     denoised        Denoised frame data.
 * \param[in]     w               Frame width
 * \param[in]     h               Frame height
 * \param[in]     strides         Stride of the planes
 * \param[in]     chroma_sub_log2 Chroma subsampling for planes != 0.
 * \param[in]     flat_blocks     A map to blocks that have been determined flat
 * \param[in]     block_size      The size of blocks.
 */
aom_noise_status_t aom_noise_model_update(
    aom_noise_model_t *const noise_model, const uint8_t *const data[3],
    const uint8_t *const denoised[3], int w, int h, int strides[3],
    int chroma_sub_log2[2], const uint8_t *const flat_blocks, int block_size);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
#endif  // AOM_DSP_NOISE_MODEL_H_
