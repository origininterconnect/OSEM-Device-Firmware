from pylsl import StreamInfo, StreamOutlet
import websocket
import socket
import time
import math
import statistics
from pynput import keyboard as pyn_keyboard
import csv
import struct
from websocket import ABNF

stop_flag = False

def on_press(key):
    global stop_flag
    try:
        if key.char == 'q':
            print("Shutdown requested (q pressed). Exiting gracefully.")
            stop_flag = True
    except AttributeError:
        if key == pyn_keyboard.Key.esc:
            print("Shutdown requested (ESC pressed). Exiting gracefully.")
            stop_flag = True

def calculate_rate(data_size, elapsed_time):
    return data_size / elapsed_time if elapsed_time > 0 else 0

def analyze_timestamp_format(timestamp):
    current_time = time.time()
    if abs(timestamp - current_time) < 86400:
        return "unix_seconds", timestamp
    timestamp_ms = timestamp / 1000.0
    if abs(timestamp_ms - current_time) < 86400:
        return "unix_milliseconds", timestamp_ms
    timestamp_us = timestamp / 1000000.0
    if abs(timestamp_us - current_time) < 86400:
        return "unix_microseconds", timestamp_us
    if timestamp > 1000 and timestamp < 86400000:
        return "relative_milliseconds", None
    return "unknown", None

def calculate_relative_latency(device_timestamps, receive_times):
    if len(device_timestamps) < 2 or len(receive_times) < 2:
        return None
    device_interval = device_timestamps[-1] - device_timestamps[-2]  # in milliseconds
    receive_interval = (receive_times[-1] - receive_times[-2]) * 1000  # ms
    return receive_interval - device_interval

def calculate_latency(device_timestamp, receive_timestamp, timestamp_format="auto"):
    if timestamp_format == "auto":
        fmt, converted_timestamp = analyze_timestamp_format(device_timestamp)
        if fmt == "unknown" or fmt == "relative_milliseconds":
            return None, fmt
        else:
            return receive_timestamp - converted_timestamp, fmt
    else:
        if timestamp_format == "unix_seconds":
            return receive_timestamp - device_timestamp, timestamp_format
        elif timestamp_format == "unix_milliseconds":
            return receive_timestamp - (device_timestamp / 1000.0), timestamp_format
        elif timestamp_format == "unix_microseconds":
            return receive_timestamp - (device_timestamp / 1000000.0), timestamp_format
        else:
            return None, "unsupported"

def main():
    global stop_flag

    # LSL setup
    stream_name = 'ORIC'
    info = StreamInfo(stream_name, 'EEG', 8, 250, 'float32', 'uid007')
    outlet = StreamOutlet(info)
    print("Trying to connect")
    ws = websocket.create_connection("ws://" + socket.gethostbyname("oric.local") + ":81")
    print(f"{stream_name} LSL Stream started!")

    # —— SYNC handshake —— 
    # 1) Notify device to prepare for sync
    ws.send("SYNC")
    resp = ws.recv()
    if resp != "SYNC_READY":
        print("⚠️  Unexpected handshake response:", resp)
    else:
        # 2) Send current host UTC in ms (little-endian u64)
        utc_ms = int(time.time() * 1000)
        payload = struct.pack("<Q", utc_ms)
        ws.send(payload, opcode=ABNF.OPCODE_BINARY)

        # 3) Wait for confirmation
        resp = ws.recv()
        if resp == "TIME_SYNCED":
            print("✅  Device time synced to", utc_ms)
        else:
            print("⚠️  Sync failed, got:", resp)
    # — end SYNC handshake —

    # Start keyboard listener
    listener = pyn_keyboard.Listener(on_press=on_press)
    listener.start()

    # Open CSVs
    packet_csvfile = open("packet_log.csv", "w", newline='')
    packet_writer = csv.writer(packet_csvfile)
    packet_writer.writerow([
        "packet_number", "packet_receive_time", "samples_in_packet", "packet_latency_ms",
        "interval_since_last_packet_ms", "bytes_in_packet", "timestamp_format"
    ])

    sample_csvfile = open("sample_log.csv", "w", newline='')
    sample_writer = csv.writer(sample_csvfile)
    sample_writer.writerow([
        "packet_number", "sample_in_packet", "global_sample_idx", "sample_receive_time",
        "device_timestamp", "sample_number", "latency_ms", "duplicate", "lost",
        "order_error", "channel_data"
    ])

    # Stats & state
    data_size = sample_size = packet_size = 0
    lost_samples = duplicate_samples = out_of_order_samples = 0
    start_time = time.time()
    blockSize = 32
    previousSampleNumber = -1
    previousTimeStamp = -1
    previousData = []
    latency_list = []
    packet_intervals = []
    packet_latencies = []
    last_packet_time = time.time()
    timestamp_format = "auto"
    format_detected = False
    device_timestamps = []
    receive_times = []
    relative_latency_changes = []
    global_sample_idx = 0
    packet_number = 0

    try:
        while not stop_flag:
            packet_receive_time = time.time()
            data = ws.recv()
            data_size += len(data)
            elapsed_time = packet_receive_time - start_time

            packet_number += 1
            samples_in_this_packet = len(data) // blockSize
            interval_since_last_packet_ms = (packet_receive_time - last_packet_time) * 1000
            last_packet_time = packet_receive_time
            if interval_since_last_packet_ms < 1000:
                packet_intervals.append(interval_since_last_packet_ms / 1000.0)

            packet_first_sample_latency = None
            first_sample_in_packet = True

            if isinstance(data, (bytes, bytearray, list)):
                packet_size += 1
                for sample_in_packet, blockLocation in enumerate(range(0, len(data), blockSize), 1):
                    sample_size += 1
                    global_sample_idx += 1
                    block = data[blockLocation:blockLocation + blockSize]

                    # parse timestamp & sample number
                    timestamp = int.from_bytes(block[0:4], byteorder='little') / 1000.0
                    sample_number = int.from_bytes(block[4:8], byteorder='little')

                    # channel data
                    channel_data = []
                    for ch in range(8):
                        offset = 8 + ch*3
                        sample = int.from_bytes(block[offset:offset+3], byteorder='big', signed=True)
                        channel_data.append(sample)

                    # latency analysis
                    device_timestamps.append(timestamp)
                    receive_times.append(packet_receive_time)
                    if len(device_timestamps) >= 2:
                        rel = calculate_relative_latency(device_timestamps, receive_times)
                        if rel is not None:
                            relative_latency_changes.append(rel)
                        if len(device_timestamps) > 100:
                            device_timestamps.pop(0)
                            receive_times.pop(0)

                    latency, detected_format = calculate_latency(timestamp, packet_receive_time, timestamp_format)
                    if not format_detected and detected_format != "unknown":
                        timestamp_format = detected_format
                        format_detected = True
                        print(f"Detected timestamp format: {timestamp_format}")

                    # sample ordering checks
                    duplicate = lost = order_error = 0
                    if previousSampleNumber == -1:
                        previousSampleNumber = sample_number
                        previousTimeStamp = timestamp
                    else:
                        if sample_number - previousSampleNumber > 1:
                            lost = sample_number - previousSampleNumber - 1
                            lost_samples += lost
                        elif sample_number == previousSampleNumber:
                            duplicate = 1
                            duplicate_samples += 1
                        elif sample_number - previousSampleNumber < 1:
                            order_error = 1
                            out_of_order_samples += 1
                        previousSampleNumber = sample_number
                        previousTimeStamp = timestamp

                    # data quality & LSL push
                    if not (all(v == 0 for v in channel_data[:3]) and all(v > 0 for v in channel_data[4:])):
                        outlet.push_sample(channel_data)

                    # write sample log
                    sample_writer.writerow([
                        packet_number, sample_in_packet, global_sample_idx,
                        packet_receive_time, timestamp, sample_number,
                        (latency*1000) if latency is not None else None,
                        duplicate, lost, order_error, str(channel_data)
                    ])

                    if first_sample_in_packet:
                        packet_first_sample_latency = (latency*1000) if latency is not None else None
                        first_sample_in_packet = False

                # write packet log
                packet_writer.writerow([
                    packet_number, packet_receive_time, samples_in_this_packet,
                    packet_first_sample_latency,
                    interval_since_last_packet_ms, len(data),
                    timestamp_format
                ])

            # periodic stats
            if elapsed_time >= 1.0:
                sps = calculate_rate(sample_size, elapsed_time)
                fps = calculate_rate(packet_size, elapsed_time)
                bps = calculate_rate(data_size, elapsed_time)
                avg_sample_lat = statistics.mean(latency_list) if latency_list else 0
                avg_packet_lat = statistics.mean(packet_latencies) if packet_latencies else 0
                avg_rel = statistics.mean(relative_latency_changes) if relative_latency_changes else 0
                jitter = statistics.stdev(packet_intervals)*1000 if len(packet_intervals)>1 else 0

                if timestamp_format == "relative_milliseconds":
                    print(f"{math.ceil(fps)} FPS : {math.ceil(sps)} SPS : {math.ceil(bps)} BPS | "
                          f"Rel Latency Δ: {avg_rel:.2f} ms | Jitter: {jitter:.2f} ms | "
                          f"Lost: {lost_samples} | Dup: {duplicate_samples} | Format: {timestamp_format}")
                else:
                    print(f"{math.ceil(fps)} FPS : {math.ceil(sps)} SPS : {math.ceil(bps)} BPS | "
                          f"Sample Lat: {avg_sample_lat*1000:.2f} ms | "
                          f"Packet Lat: {avg_packet_lat*1000:.2f} ms | "
                          f"Jitter: {jitter:.2f} ms | Lost: {lost_samples} | Dup: {duplicate_samples} | Format: {timestamp_format}")

                # reset stats counters
                packet_size = sample_size = data_size = 0
                latency_list.clear()
                packet_intervals.clear()
                packet_latencies.clear()
                relative_latency_changes.clear()
                start_time = packet_receive_time

    except Exception as e:
        print(f"Error: {e}")

    finally:
        ws.close()
        listener.stop()
        packet_csvfile.close()
        sample_csvfile.close()
        print("Resources closed. Program ended.")

if __name__ == "__main__":
    main()
