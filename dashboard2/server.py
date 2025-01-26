#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Flask Dashboard with Detailed Charts
====================================

This Flask application:
  - Parses sensor logs with a robust regex-based parser (for irregular lines).
  - Displays an HTML dashboard with Chart.js line graphs first (detailed lines,
    larger points, distinct colors).
  - Then shows tables of raw sensor data.

Usage:
1. Put this file in the same directory as your log files or adjust
   LOG_FILE_PATHS as needed.
2. Install Flask: pip install flask
3. Run: python server.py
4. Browse: http://127.0.0.1:5000/
"""

import os
import re
import json
import logging
from flask import Flask, render_template

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(name)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Dictionary mapping sensor names to their log file paths.
LOG_FILE_PATHS = {
    "BH1750": "bh1750.log",
    "DHT22": "dht22.log",
    "MPU6050": "mpu6050.log",
    "MQ135": "mq135.log",
    "QMC5883L": "qmc5883l.log"
}

def parse_log_file_robust(log_file_path, sensor_name):
    """
    Parse the specified log file and return a list of dicts.
    Each line is expected to have:
      - A prefix (used as "timestamp")
      - A JSON object in { ... }

    Lines that don't match or that fail JSON parsing are skipped.
    """
    entries = []
    if not os.path.isfile(log_file_path):
        logger.warning(f"Log file not found: {log_file_path}")
        return entries

    pattern = re.compile(r"^(.*?)(\{.*\})\s*$")

    try:
        with open(log_file_path, "r") as file:
            for line in file:
                line = line.strip()
                if not line:
                    continue

                match = pattern.match(line)
                if not match:
                    logger.warning(f"Malformed or no-JSON line in {log_file_path}: {line}")
                    continue

                prefix_str = match.group(1).strip()
                json_str   = match.group(2).strip()

                try:
                    data = json.loads(json_str)
                except json.JSONDecodeError:
                    logger.warning(f"JSON parse error in {log_file_path}: {json_str}")
                    continue

                sensor_entry = {"timestamp": prefix_str}
                sensor_entry.update(data)
                entries.append(sensor_entry)

    except Exception as e:
        logger.error(f"Error reading {log_file_path}: {e}")

    logger.info(f"Loaded {len(entries)} entries from {sensor_name} log.")
    return entries

@app.route("/")
def dashboard():
    """
    Gathers log data for all sensors, passes it to the template.
    The template then displays charts first, followed by data tables.
    """
    all_data = {}
    for sensor, path in LOG_FILE_PATHS.items():
        parsed_entries = parse_log_file_robust(path, sensor)
        all_data[sensor] = parsed_entries

    return render_template("dashboard.html", all_data=all_data)

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=True)

