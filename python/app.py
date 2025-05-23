# Bronnen:
# chatgpt.com (18/05)
# copilot.github.com (18/05)
# https://docs.python.org/3/library/csv.html (18/05)

from flask import Flask, request, render_template
import csv
import os
from datetime import datetime

app = Flask(__name__)
CSV_FILE = 'data.csv'
HEADERS = ['Date', 'Sensor Outside', 'Sensor Inside', 'Sensor Hive']

# Make sure CSV exists with headers
if not os.path.isfile(CSV_FILE):
    with open(CSV_FILE, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(HEADERS)

# Helper function to parse sensor data string into 3 columns
def parse_sensor_data(raw_string):
    parts = [p.strip() for p in raw_string.split(',')]
    outside = ', '.join([p for p in parts if p.startswith('DHT1')])
    inside  = ', '.join([p for p in parts if p.startswith('DHT2')])
    hive    = ', '.join([p for p in parts if p.startswith('SHT')])
    return outside, inside, hive

@app.route('/add-data', methods=['POST'])
def add_data():
    try:
        data = request.get_json()
        line = data.get("data")

        if not line:
            return 'Missing "data" field', 400

        # Parse into columns
        outside, inside, hive = parse_sensor_data(line)

        # Write to CSV
        with open(CSV_FILE, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                outside,
                inside,
                hive
            ])

        return 'Data saved', 200

    except Exception as e:
        return f'Error: {str(e)}', 500

@app.route('/', methods=['GET'])
def index():
    try:
        with open(CSV_FILE, newline='') as csvfile:
            reader = csv.reader(csvfile)
            rows = list(reader)

        headers = rows[0] if rows else HEADERS
        data = rows[1:] if len(rows) > 1 else []
        return render_template('index.html', headers=headers, rows=data)
    except Exception as e:
        return f'Error: {str(e)}', 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)