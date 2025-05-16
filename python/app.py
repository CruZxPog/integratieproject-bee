from flask import Flask, request
import csv
from datetime import datetime

app = Flask(__name__)
csv_file = 'data.csv'

@app.route('/add-data', methods=['POST'])
def add_data():
    try:
        data = request.get_json()
        line = data.get("data")

        if not line:
            return 'Missing "data" field', 400

        with open(csv_file, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([datetime.now().strftime('%Y-%m-%d %H:%M:%S'), line])

        return 'Data saved', 200

    except Exception as e:
        return f'Error: {str(e)}', 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
