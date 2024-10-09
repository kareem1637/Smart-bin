import serial
import time

# Configure the serial connection
ser = serial.Serial(
    port='COM3',    # Replace with your COM port
    baudrate=9600,  # Adjust according to your setup
    timeout=1
)

# The messages to be sent
messages = [
    "+RCV=1, 5, Bin Level: 71% modelPrediction: dirty,-99, 40",
    "+RCV=2, 5, Bin Level: 20% modelPrediction: clean,-99, 40",
    "+RCV=3, 5, Bin Level: 35% modelPrediction: dirty,-99, 40",
    "+RCV=3, 5, Bin Level: 80% modelPrediction: failed ,-99, 40",
    "+RCV=5, 5, Bin Level: 100% modelPrediction: clean ,-99, 40"
    "+RCV=1, 5, Bin Level: 71% modelPrediction: dirty,-99, 40",
    "+RCV=2, 5, Bin Level: 40% modelPrediction: clean,-99, 40",
    "+RCV=3, 5, Bin Level: 35% modelPrediction: dirty,-99, 40",
    "+RCV=3, 5, Bin Level: 23% modelPrediction: failed ,-99, 40",
    "+RCV=5, 5, Bin Level: 28% modelPrediction: clean ,-99, 40"
]

print("Sending messages...")

i = 0
max_retries = 5  # Maximum retries if no ACK is received
retry_delay = 5  # Seconds to wait before retrying

try:
    while i < len(messages):  # Continue as long as there are messages to send
        ACK = False
        retries = 0

        # Clear the input buffer
        ser.reset_input_buffer()

        print(f"Sending message {i + 1}: {messages[i]}")
        ser.write(messages[i].encode('utf-8'))  # Send the message

        # Wait for ACK or retry
        while not ACK and retries < max_retries:
            print("Waiting for ACK...")
            time.sleep(1)  # Wait for some time before checking for a response

            if ser.in_waiting > 0:
                response = ser.readline().decode('utf-8', errors='replace').strip()
                print(f"Received: {response}")
                
                if "ACK" in response:
                    print("ACK received!")
                    ACK = True
                else:
                    print("No ACK, received something else. Retrying...")

            if not ACK:
                retries += 1
                print(f"Retrying {retries}/{max_retries}...")
                time.sleep(retry_delay)

        if ACK:
            # If ACK received, wait for 1 minute before sending the next message
            print(f"Message {i + 1} sent successfully. Waiting for 1 minute...")
            time.sleep(60)
            i += 1  # Move to the next message
        else:
            # If maximum retries reached without ACK
            print(f"Max retries reached for message {i + 1}. Moving to the next message...")
            i += 1  # Move to the next message

except KeyboardInterrupt:
    print("Program interrupted by user")
finally:
    # Close the serial connection
    ser.close()
    print("Serial connection closed")
