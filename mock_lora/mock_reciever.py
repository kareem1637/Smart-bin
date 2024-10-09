import serial
import time

# Configure the serial connection
ser = serial.Serial(
    port='COM3',    # Replace with your COM port
    baudrate=9600,  # Adjust according to your setup
    timeout=1
)

# The messages to be sent
messages = "+RCV=50, 5, ACK ,-99, 40"



i = 0
try:
    while True:
        # get the message
        response = ser.readline().decode('utf-8').strip()
        print(f"Received: {response}")
        if response != "":
            # send confirm message
            text = messages
            ser.write(text.encode('utf-8'))
            print(f"Sent: {text}")

except KeyboardInterrupt:
    print("Program interrupted by user")
finally:
    # Close the serial connection
    ser.close()
    print("Serial connection closed")
