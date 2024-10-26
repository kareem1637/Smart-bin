import numpy as np
from PIL import Image
def read_rgb888_from_file(file_path):
    """Reads RGB888 values from a text file and returns them as a NumPy array 96*96*3."""
    rgb888_values = []
    
    with open(file_path, 'r') as file:
        for line in file:
            rgb_values = line.strip().split()  # Split the line into RGB components
            if len(rgb_values) == 3:  # Check if there are 3 values (R, G, B)
                r = int(rgb_values[0])
                g = int(rgb_values[1])
                b = int(rgb_values[2])
                rgb888_values.append((r, g, b))  # Append tuple of RGB values
        rgb888_values=np.array(rgb888_values)    
        rgb888_values=rgb888_values.reshape(240, 240, 3)
    return np.array(rgb888_values, dtype=np.uint8)  # Ensure dtype is uint8 for image
def save_image_from_rgb888(rgb888_array, width, height, filename):
    """Saves the RGB888 array as an image file."""
    img_array = rgb888_array.reshape((height, width, 3))  # Reshape to (height, width, 3)
    img = Image.fromarray(img_array, 'RGB')
    
    # Rotate the image by 180 degrees
    img = img.rotate(180)


    img.save(filename)
    print(f"Image saved as {filename}")
# Testing the functions
if __name__ == "__main__":
    test_file_path="picture1_888.txt"
        # Read the RGB888 values from the test file
    rgb888_array = read_rgb888_from_file(test_file_path)

    # Save the image
    output_filename = 'output_image.jpg'
    save_image_from_rgb888(rgb888_array, width=240, height=240, filename=output_filename)    