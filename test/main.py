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
data_size = 0
sample_size = 0
packet_size = 0
previousSampleNumber = -1
previousTimeStamp = -1
previousData = []
strean_name = 'ORIC2'
data = StreamInfo(strean_name, 'EEG', 8, 250, 'float32', 'uid007')
outlet = StreamOutlet(data)

ws = websocket.WebSocket()
ws.connect("ws://"+socket.gethostbyname("oric.local")+":81")
# ws.connect("ws://192.168.0.23:81")

ws.send_text(json.dumps({"command":"sdatac", "parameters":[]}))
# ws.send_text(json.dumps({"command":"wreg", "parameters":[0x01, 0b10010100]}))
ws.send_text(json.dumps({"command":"wreg", "parameters":[0x01, 0x96]}))
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
    # print(data)  # REMOVE THIS LINE - this was printing raw bytes
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
        # print(f"Local Time: {hours:02d}:{minutes:02d}:{seconds:02d}")
        # print(f"Bytes per second: {bytes_per_second} BPS")
        # print(f"Samples per second: {math.ceil(samples_per_second)}")
        # print(f"SPS Refresh rate: {math.ceil(refresh_rate)}")
        print(f"{math.ceil(refresh_rate)} Packets : {math.ceil(samples_per_second)} Samples : {math.ceil(bytes_per_second)} Bytes")
        packet_size = 0
        sample_size = 0
        data_size = 0
        start_time = current_time
    # print(len(data))
    if data and (type(data) is list or type(data) is bytes):
        # print(len(data))
        # ws.send_text(json.dumps({"command":"status", "parameters":[]}))
        # status = ws.recv()
        # print(status)
        packet_size += 1
        # print("Packet size: ", len(data), "Bytes")

        # Extract and print battery percentage after each packet
        # if len(data) > 8000:
        battery_percentage = data[-1]
        print(f"Battery: {battery_percentage}%")

        for blockLocation in range(0, len(data)-1, blockSize):
            sample_size += 1
            block = data[blockLocation:blockLocation + blockSize]
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
            outlet.push_sample(channel_data)
            if(all(v == 0 for v in channel_data[:3]) and all(v > 0 for v in channel_data[4:])):
                print("Blank Data: ",timestamp, sample_number, channel_data[0], channel_data[1], channel_data[2], channel_data[3], channel_data[4], channel_data[5], channel_data[6], channel_data[7])
                exit()
            else:
                print("EEG Data: ",timestamp, sample_number, channel_data[0], channel_data[1], channel_data[2], channel_data[3], channel_data[4], channel_data[5], channel_data[6], channel_data[7])  # UNCOMMENT THIS LINE
                outlet.push_sample(channel_data)
