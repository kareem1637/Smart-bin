import serial
import numpy as np
from PIL import Image

# Configuration
ser = serial.Serial('COM3', 115200)  # Replace with your actual COM port
width, height = 96, 96  # Match the resolution set in ESP32 code
image_data = []
i=0
try:
    while True:
        # Prompt user to enter 'C' to capture an image
        command = input("Enter 'C' to capture an image: ").strip()
        if command.upper() == 'C':
            # Send request to ESP32-CAM to capture an image
            ser.write(b'C')  # Send command as bytes
            
            print("Waiting for image data...")

            # Read RGB values
            for _ in range(width * height):
                # Ensure we read one pixel's worth of RGB data
                r = ser.read()
                g = ser.read()
                b = ser.read()
                
                # Convert bytes to integer values
                r = int.from_bytes(r, 'little')
                g = int.from_bytes(g, 'little')
                b = int.from_bytes(b, 'little')
                
                # Append RGB values to the list
                image_data.append((r, g, b))

            # Convert to numpy array
            img_array = np.array(image_data, dtype=np.uint8).reshape((height, width, 3))
            img = Image.fromarray(img_array, 'RGB')
            img.save('captured_images/captured_image'+str(i)+'.jpg')  # Save as JPEG
            i+=1
            print("Image saved as 'captured_image.jpg'")
            image_data.clear()  # Clear the list for the next image

except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
