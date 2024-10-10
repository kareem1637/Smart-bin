import asyncio
import websockets
import json
import random

# Dummy data generation function for different cases
def generate_dummy_data():
    bin_level = random.randint(0, 100)  # Random bin level between 0 and 100
    model_prediction = random.choice(["dirty", "clean"])  # Randomly predict clean or dirty
    bin_id = random.randint(1, 5)  # Random bin ID between 1 and 5

    return {
        "bin level": str(bin_level),
        "model prediction": model_prediction,
        "bin ID": str(bin_id)
    }

# Function to send data to the web app
async def send_data(websocket):
    while True:
        data = generate_dummy_data()  # Generate random dummy data
        await websocket.send(json.dumps(data))  # Send data to the web app
        print(f"Sent data: {data}")  # Print the sent data for debugging
        await asyncio.sleep(5)  # Send data every 5 seconds

async def handler(websocket, path):
    print("New client connected")  # Indicate a new connection
    try:
        await send_data(websocket)
        async for message in websocket:
            print(f"Received message: {message}")

            # Check if the message is a heartbeat
            try:
                heartbeat_msg = json.loads(message)
                if heartbeat_msg.get("type") == "heartbeat":
                    # Send heartbeat acknowledgment
                    await websocket.send(json.dumps({"type": "heartbeat_ack"}))
                    print("Sent heartbeat acknowledgment")
                else:
                    print("Received unknown message type")
            except json.JSONDecodeError:
                print("Received non-JSON message")

    except websockets.ConnectionClosed:
        print("Connection closed")


# Start the server
print("Server started on ws://172.31.80.1:8080")

start_server = websockets.serve(handler, "192.168.1.104", 8080)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
