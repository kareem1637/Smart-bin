#ifndef TF_LITE_MODEL_H
#define TF_LITE_MODEL_H
#include "common.h"

// Model parameters
constexpr int kNumCols = 240;
constexpr int kNumRows = 240;
constexpr int kNumChannels = 3;
constexpr int kMaxImageSize = kNumCols * kNumRows * kNumChannels;

// TensorFlow Lite variables
extern const tflite::Model* model;
extern tflite::MicroInterpreter* interpreter;
extern TfLiteTensor* input;
extern TfLiteTensor* output;

// Tensor arena configuration
constexpr int kTensorArenaSize = 1500000;
extern uint8_t* tensor_arena;
extern const uint8_t TF_LITE_MODEL_data[]; // Adjust type based on your model data
extern unsigned int TF_LITE_MODEL_data_len;
// Functions to initialize and clean up TensorFlow Lite model
void initializeTensorFlowModel(const unsigned char TF_LITE_MODEL_data[]);
void cleanup();

#endif  // TF_LITE_MODEL_H
