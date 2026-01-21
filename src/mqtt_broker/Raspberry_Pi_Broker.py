#Objective: 1. Start serial communication 2. Start MQTT 3. Configure AWS Client, Key and Certificate 4. Transmitt data 5. Save data on local disc.

import serial
from datetime import datetime
import json
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder

# Serial port configuration
serial_port = '/dev/ttyUSB0'
baud_rate = 115200

# AWS IoT configuration
aws_endpoint = "*****************"
aws_publish_topic = "*****************"
cert_filepath = ".crt file path"
key_filepath = ".key file path"
ca_filepath = "rootCA.pem file path"

# Mapping of keywords to descriptions for data saving
keyword_map_save = {
    "P1": "Phase 1 line to neutral voltage",
    "P2": "Phase 2 line to neutral voltage",
    "P3": "Phase 3 line to neutral voltage",
    "P4": "Phase 1 current",
    "P5": "Phase 2 current",
    "P6": "Phase 3 current",
    "P7": "Phase 1 reactive power",
    "P8": "Phase 2 reactive power",
    "P9": "Phase 3 reactive power",
    "P10": "Phase 1 power factor",
    "P11": "Phase 2 power factor",
    "P12": "Phase 3 power factor",
    "P13": "Line 1 to Line 2 voltage",
    "P14": "Line 2 to Line 3 voltage",
    "P15": "Line 3 to Line 1 voltage",
    "P16": "Neutral current",
    "P17": "Frequency"
}

# Mapping of keywords to descriptions for data publishing
keyword_map_publish = {
    "P1": "V1_Volt",
    "P2": "V2_Volt",
    "P3": "V3_Volt",
    "P4": "I1_Amp",
    "P5": "I2_Amp",
    "P6": "I3_Amp",
    "P7": "W1_Watt",
    "P8": "W2_Watt",
    "P9": "W3_Watt",
    "P10": "PF1",
    "P11": "PF2",
    "P12": "PF3",
    "P13": "L1V-L2V_Volt",
    "P14": "L2V-L3V_Volt",
    "P15": "L3V-L1V_Volt",
    "P16": "NC_Amp",
    "P17": "Hz"
}

# Set up serial communication
ser = serial.Serial(serial_port, baud_rate, timeout=1)

# AWS IoT MQTT connection setup
mqtt_connection = mqtt_connection_builder.mtls_from_path(
    endpoint=aws_endpoint,
    cert_filepath=cert_filepath,
    pri_key_filepath=key_filepath,
    ca_filepath=ca_filepath,
    client_id="Lora1",
    clean_session=False,
    keep_alive_secs=30
)

def on_connection_interrupted(connection, error, **kwargs):
    print(f"Connection interrupted. Error: {error}")

def on_connection_resumed(connection, return_code, session_present, **kwargs):
    print(f"Connection resumed. Return code: {return_code}, Session present: {session_present}")

def on_publish(topic, payload, **kwargs):
    print(f"Published to topic {topic}: {payload}")

def make_publish_callback(payload_str):
    def callback(future):
        try:
            future.result()  # This will raise an exception if the publish failed
            print(f"Published to AWS IoT: {payload_str}")
        except Exception as e:
            print(f"Publish failed: {e}")
    return callback

# Add connection callbacks
mqtt_connection.on_interrupted = on_connection_interrupted
mqtt_connection.on_resumed = on_connection_resumed

# Connect to AWS IoT
print(f"Connecting to AWS IoT endpoint {aws_endpoint}...")
connect_future = mqtt_connection.connect()

# Wait for the connection to be established
try:
    connect_future.result()
    print("Connected to AWS IoT!")
except Exception as e:
    print(f"Failed to connect: {e}")
    exit(1)

# Open a file to log MQTT messages
log_file = open('mqtt_log.txt', 'a')  # 'a' for append mode

# Temporary storage for received data
data_buffer = []

try:
    while True:
        if ser.in_waiting > 0:
            # Read data from the serial port
            lora_data = ser.readline().decode('utf-8').strip()
            
            # Check if the received data is not empty
            if lora_data:
                # Add data to buffer
                data_buffer.append(lora_data)

                # Create a copy to keep for logging purposes
                log_data = lora_data

                # Replace exact keywords with descriptions for logging
                for keyword, description in keyword_map_save.items():
                    log_data = log_data.replace(f"{keyword}:", f"{description}:")

                # Get the current date and time
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                
                # Print received data with timestamp
                print(f"Received [{timestamp}]: {log_data}")
                
                # Write log_data (modified for readability) to the log file with timestamp
                log_file.write(f"{timestamp} {log_data}\n")

                # Check if 'P17' is in the received data
                if 'P17' in lora_data:
                    # Create a dictionary to store the JSON data
                    json_data = {
                        "ID": "EnergyMeter",
                        "time": int(datetime.now().timestamp())
                    }

                    # Update the dictionary with the received data
                    for data in data_buffer:
                        # Remove extra information (e.g., RSSI and length)
                        data = data.split('with')[0].strip()
                        parts = data.split(",")
                        for part in parts:
                            sub_parts = part.split(":")
                            if len(sub_parts) == 2:
                                keyword = sub_parts[0].strip()
                                value = sub_parts[1].strip()
                                if keyword in keyword_map_publish:
                                    description = keyword_map_publish[keyword]

                                    json_data[description] = value
                    
                    # Publish the JSON data to AWS IoT
                    payload_str = json.dumps(json_data)
                    publish_future, _ = mqtt_connection.publish(
                        topic=aws_publish_topic,
                        payload=payload_str,
                        qos=mqtt.QoS.AT_LEAST_ONCE
                    )

                    # Add callback to handle result asynchronously
                    publish_future.add_done_callback(make_publish_callback(payload_str))
                    
                    # Clear the buffer after publishing
                    data_buffer.clear()

except KeyboardInterrupt:
    pass
finally:
    ser.close()
    disconnect_future = mqtt_connection.disconnect()
    disconnect_future.result()
    log_file.close()  # close the log file when exiting
    print("Disconnected from AWS IoT and closed log file.")
