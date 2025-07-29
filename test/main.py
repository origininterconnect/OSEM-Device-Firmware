import websocket
import socket
import json
import time
import datetime
import math
from pylsl import StreamInfo, StreamOutlet

def calculate_rate(data_size, elapsed_time):
    rate = data_size / elapsed_time
    return rate

blockSize = 32
samples_per_buffer = 10
accel_data_size = 6
expected_packet_size = (blockSize * samples_per_buffer) + accel_data_size  # 326 bytes

data_size = 0
sample_size = 0
packet_size = 0
previousSampleNumber = -1
previousTimeStamp = -1
previousData = []

stream_name = 'ORIC2'
data = StreamInfo(stream_name, 'EEG', 8, 500, 'float32', 'uid007')
outlet = StreamOutlet(data)

# Add accelerometer stream (50Hz since it's updated once per packet of 10 samples at 500Hz)
accel_stream_name = 'ORIC_Accel'
accel_data = StreamInfo(accel_stream_name, 'Accelerometer', 3, 50, 'float32', 'uid008')
accel_outlet = StreamOutlet(accel_data)

ws = websocket.WebSocket()
ws.connect("ws://"+socket.gethostbyname("oric.local")+":81")
# ws.connect("ws://192.168.0.23:81")

ws.send_text(json.dumps({"command":"sdatac", "parameters":[]}))
# ws.send_text(json.dumps({"command":"wreg", "parameters":[0x01, 0b10010100]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x01, 0x95]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x02, 0xC0]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x03, 0xEC]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x15, 0b00100000]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x05, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x06, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x07, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x08, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x09, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x0A, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x0B, 0x00]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x0C, 0x00]}))
ws.send_text(json.dumps({"command":"status", "parameters":[]}))
ws.send_text(json.dumps({"command":"rdatac", "parameters":[]}))

print("Setup Done!")

start_time = time.time()

while 1:
    data = ws.recv()
    data_size += len(data)

    current_time = time.time()
    elapsed_time = current_time - start_time
    if elapsed_time >= 1.0:
        samples_per_second = calculate_rate(sample_size, elapsed_time)
        refresh_rate = calculate_rate(packet_size, elapsed_time)
        bytes_per_second = calculate_rate(data_size, elapsed_time)

        # Get the current local time
        local_time = datetime.datetime.now()
        # Extract hours, minutes, and seconds
        hours = local_time.hour
        minutes = local_time.minute
        seconds = local_time.second

        print(f"{math.ceil(refresh_rate)} Packets : {math.ceil(samples_per_second)} Samples : {math.ceil(bytes_per_second)} Bytes")
        packet_size = 0
        sample_size = 0
        data_size = 0
        start_time = current_time

    if data and (type(data) is list or type(data) is bytes):
        packet_size += 1

        # Check if packet size is correct
        if len(data) != expected_packet_size:
            print(f"Error: Expected packet size {expected_packet_size}, got {len(data)}")
            continue

        # Extract accelerometer data from the end of the packet
        accel_data = data[-accel_data_size:]  # Last 6 bytes
        accel_x = int.from_bytes(accel_data[0:2], byteorder='little', signed=True)
        accel_y = int.from_bytes(accel_data[2:4], byteorder='little', signed=True)
        accel_z = int.from_bytes(accel_data[4:6], byteorder='little', signed=True)

        # Process only the EEG data part (exclude accelerometer data)
        eeg_data = data[:-accel_data_size]  # All except last 6 bytes

        for blockLocation in range(0, len(eeg_data), blockSize):
            sample_size += 1
            block = eeg_data[blockLocation:blockLocation + blockSize]

            # data_hex = ":".join("{:02x}".format(c) for c in data)
            timestamp = int.from_bytes(block[0:4], byteorder='little')
            sample_number = int.from_bytes(block[4:8], byteorder='little')
            channel_data = []

            for channel in range(0, 8):
                channel_offset = 8 + (channel * 3)
                sample = int.from_bytes(block[channel_offset:channel_offset + 3], byteorder='big', signed=True)
                channel_data.append(sample)

            if previousSampleNumber == -1:
                previousSampleNumber = sample_number
                previousTimeStamp = timestamp
                previousData = channel_data
            else:
                if sample_number - previousSampleNumber > 1:
                    print("Error: Sample Lost")
                    exit()
                elif sample_number == previousSampleNumber:
                    print("Error: Duplicate sample")
                    exit()
                elif sample_number - previousSampleNumber < 1:
                    print("Error: Sample order missed")
                    exit()
                else:
                    # print(timestamp - previousTimeStamp)
                    previousTimeStamp = timestamp
                    previousSampleNumber = sample_number
                    previousData = channel_data

            # Check only EEG channels for blank data (first 8 channels)
            if(all(v == 0 for v in channel_data[:3]) and all(v > 0 for v in channel_data[4:8])):
                print("Blank Data: ",timestamp, sample_number, channel_data[0], channel_data[1], channel_data[2], channel_data[3], channel_data[4], channel_data[5], channel_data[6], channel_data[7])
                # print(f"Accel Data: X={accel_x:.3f}, Y={accel_y:.3f}, Z={accel_z:.3f}")
                exit()
            else:
                # Push all 11 channels: 8 EEG + 3 accelerometer
                outlet.push_sample(channel_data)
                # Push accelerometer data once per packet
                accel_outlet.push_sample([accel_x, accel_y, accel_z])

                # Optional: Print data periodically for debugging
                print(f"EEG: {channel_data[:8]}, Accel: [{accel_x:.3f}, {accel_y:.3f}, {accel_z:.3f}]")
