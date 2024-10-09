#ifndef PREPROCESSING_H
#define PREPROCESSING_H
void preprocess_image(uint8_t *buf, size_t len, File &rgb888File, File &quantizedFile) {
    
    for (size_t i = 0; i < len; i += 2) {
        uint16_t rgb565 = buf[i + 1] | (buf[i] << 8);

        // Extract RGB components from RGB565
        uint8_t r = (rgb565 >> 11) & 0x1F;  // 5 bits for Red
        uint8_t g = (rgb565 >> 5) & 0x3F;   // 6 bits for Green
        uint8_t b = rgb565 & 0x1F;          // 5 bits for Blue

        // Convert to 8 bits
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;

        // Save RGB888 data to the file
        rgb888File.printf("%03d %03d %03d\n", r, g, b);

        // Normalize and quantize the image data to int8 [-128, 127]
        float normalized_r = (r / 127.5f) - 1.0f;
        float normalized_g = (g / 127.5f) - 1.0f;
        float normalized_b = (b / 127.5f) - 1.0f;

        int8_t quantized_r = static_cast<int8_t>(round(normalized_r / 0.007843137718737125f) - 1);
        int8_t quantized_g = static_cast<int8_t>(round(normalized_g / 0.007843137718737125f) - 1);
        int8_t quantized_b = static_cast<int8_t>(round(normalized_b / 0.007843137718737125f) - 1);

        // Save quantized data to the file
        quantizedFile.printf("%03d %03d %03d\n", quantized_r, quantized_g, quantized_b);
    }
}

#endif // PREPROCESSING_H
