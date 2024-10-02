import serial
import time
import os
import re

ser = serial.Serial("/dev/ttyUSB0", 115200, timeout=1)
time.sleep(1)

ansi_escape_pattern = re.compile(r"\x1b\[[0-9;]*[a-zA-Z]")
log_pattern = re.compile(r"^[IWE] \(\d+\) \w+: .*")

def clean_line(line: str):
    return ansi_escape_pattern.sub("", line).strip()

def print_log_in_green(log_message):
    print(f"\033[32m{log_message}\033[0m")

def send_command(command):
    ser.write(command.encode())
    # print(f"Sent to ESP: {repr(command)}")
    while True:
        raw_line = ser.readline().decode("utf-8").strip()
        cleaned_line = clean_line(raw_line)
        if not cleaned_line:
            continue
        if log_pattern.match(cleaned_line):
            print_log_in_green(cleaned_line)
            continue
        # print(f"Received non-log response: {repr(cleaned_line)}")
        return cleaned_line

def scan_networks():
    print("Starting Wi-Fi scan..")
    ser.write(b"scan\n")
    networks = []
    while True:
        line = clean_line(ser.readline().decode("utf-8").strip())
        if not line:
            continue
        if line.lower() == "Wi-Fi scan completed.".lower():
            break
        networks.append(line)
    print("Available Wi-Fi Networks:")
    for network in networks:
        print(network)

def connect_to_wifi(ssid, password):
    print(f"Connecting to {ssid}..")
    send_command(f"connect {ssid} {password}\n")

def continuous_dump():
    try:
        while True:
            line = clean_line(ser.readline().decode("utf-8").strip())
            if line:
                print_log_in_green(line)
    except KeyboardInterrupt:
        print("Stopped continuous dump.")

def check_esp_status():
    return send_command("status\n").lower() == "ready"

try:
    if not check_esp_status():
        print("ESP32 is not ready. Exiting.")
        exit(1)

    scan_networks()

    ssid = os.getenv("ESP_SSID")
    password = os.getenv("ESP_PASSWORD")
    if ssid and password:
        print(f"Connecting to Wi-Fi network: {ssid}")
        connect_to_wifi(ssid, password)
        continuous_dump()

except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
