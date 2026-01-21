import time
import json
import threading

# Mocks
class MockFuture:
    def __init__(self):
        self._done_event = threading.Event()
        # Simulate network delay in a separate thread so non-blocking doesn't block
        threading.Timer(0.1, self._complete).start()

    def _complete(self):
        self._done_event.set()
        # In a real future, callbacks are called here
        if hasattr(self, '_callbacks'):
            for cb in self._callbacks:
                cb(self)

    def result(self):
        self._done_event.wait()
        return None

    def add_done_callback(self, callback):
        if not hasattr(self, '_callbacks'):
            self._callbacks = []
        self._callbacks.append(callback)

class MockMqttConnection:
    def publish(self, topic, payload, qos):
        return MockFuture(), 1

class MockSerial:
    def __init__(self, data_lines):
        self.data_lines = data_lines
        self.index = 0

    @property
    def in_waiting(self):
        return 1 if self.index < len(self.data_lines) else 0

    def readline(self):
        if self.index < len(self.data_lines):
            line = self.data_lines[self.index]
            self.index += 1
            return line.encode('utf-8')
        return b""

    def close(self):
        pass

# Constants and Setup
keyword_map_publish = {
    "P1": "V1_Volt",
    "P17": "Hz"
}

# The new helper function from Raspberry_Pi_Broker.py
def make_publish_callback(payload_str):
    def callback(future):
        try:
            future.result()  # This will raise an exception if the publish failed
            # print(f"Published to AWS IoT: {payload_str}")
        except Exception as e:
            print(f"Publish failed: {e}")
    return callback

def run_benchmark(blocking=True):
    # Prepare mock data
    iterations = 10
    lines = []
    for _ in range(iterations):
        lines.append("P1: 230, RSSI: -40")
        lines.append("P17: 50, RSSI: -40") # Triggers publish

    ser = MockSerial(lines)
    mqtt_connection = MockMqttConnection()
    data_buffer = []

    start_time = time.time()

    try:
        while True:
            if ser.in_waiting > 0:
                lora_data = ser.readline().decode('utf-8').strip()

                if lora_data:
                    data_buffer.append(lora_data)

                    if 'P17' in lora_data:
                        json_data = {"ID": "EnergyMeter", "time": 12345}

                        # Simplified logic from original
                        for data in data_buffer:
                            data = data.split('with')[0].strip()
                            parts = data.split(",")
                            for part in parts:
                                sub_parts = part.split(":")
                                if len(sub_parts) == 2:
                                    keyword = sub_parts[0].strip()
                                    value = sub_parts[1].strip()
                                    if keyword in keyword_map_publish:
                                        json_data[keyword_map_publish[keyword]] = value

                        payload_str = json.dumps(json_data)
                        publish_future, _ = mqtt_connection.publish(
                            topic="test/topic",
                            payload=payload_str,
                            qos=1
                        )

                        if blocking:
                            publish_future.result()
                        else:
                            # Using the structure from the updated file
                            publish_future.add_done_callback(make_publish_callback(payload_str))

                        data_buffer.clear()
            else:
                break

    finally:
        ser.close()

    end_time = time.time()
    return end_time - start_time

if __name__ == "__main__":
    print("Running Verification Benchmark...")

    blocking_time = run_benchmark(blocking=True)
    print(f"Blocking Mode Time: {blocking_time:.4f} seconds")

    non_blocking_time = run_benchmark(blocking=False)
    print(f"Non-Blocking Mode Time: {non_blocking_time:.4f} seconds")

    improvement = blocking_time - non_blocking_time
    print(f"Improvement: {improvement:.4f} seconds")
