import numpy as np
from PIL import Image, ImageEnhance, ImageFilter  # Ensure Pillow is installed: `pip install Pillow`

def read_rgb888_from_file(file_path):
    """Reads RGB888 values from a text file and returns them as a NumPy array."""
    rgb888_values = []
    
    with open(file_path, 'r') as file:
        for line in file:
            rgb_values = line.strip().split()  # Split the line into RGB components
            if len(rgb_values) == 3:  # Check if there are 3 values (R, G, B)
                r = int(rgb_values[0])
                g = int(rgb_values[1])
                b = int(rgb_values[2])
                rgb888_values.append((r, g, b))  # Append tuple of RGB values
    
    return np.array(rgb888_values, dtype=np.uint8)  # Ensure dtype is uint8 for image

def save_image_from_rgb888(rgb888_array, width, height, filename):
    """Saves the RGB888 array as an image file."""
    img_array = rgb888_array.reshape((height, width, 3))  # Reshape to (height, width, 3)
    img = Image.fromarray(img_array, 'RGB')
    
    # Rotate the image by 180 degrees
    img = img.rotate(180)

    # # Enhance image quality
    # img = enhance_image(img)

    # # Apply filter
    # img = apply_filters(img)

    img.save(filename)
    print(f"Image saved as {filename}")

def enhance_image(img):
    """Enhances the brightness and color of the image."""
    # Enhance the brightness
    enhancer = ImageEnhance.Brightness(img)
    img = enhancer.enhance(1.2)  # Increase brightness by 20%

    # Enhance the color
    enhancer = ImageEnhance.Color(img)
    img = enhancer.enhance(1.5)  # Increase color saturation by 50%

    return img

def apply_filters(img):
    """Applies filters to improve the image quality."""
    img = img.filter(ImageFilter.SHARPEN)  # Sharpen the image
    return img

def main():
    # Set the path to your text file containing RGB888 values
    file_path = 'tests/picture3_888.txt'  # Replace with the path to your text file

    # Set the expected dimensions of the image
    width = 96  # Replace with your image width
    height = 96  # Replace with your image height

    # Read RGB888 values from the text file
    rgb888_array = read_rgb888_from_file(file_path)
    print(rgb888_array.shape)
    rgb888_array=rgb888_array.reshape(height, width, 3)
    print(rgb888_array[0][0])
    # Save the enhanced and rotated RGB888 image
    # save_image_from_rgb888(rgb888_array, width, height, "converted_image_rgb888.jpeg")

if __name__ == "__main__":
    main()
