from flask import Flask, request, jsonify, send_from_directory
from flask_sqlalchemy import SQLAlchemy
import datetime
import json

app = Flask(__name__)

# SQLite database configuration
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///esp_data.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Define the database model
class ESPData(db.Model):
    id = db.Column(db.Integer, primary_key=True)  # Primary key
    timestamp = db.Column(db.DateTime, default=datetime.datetime.utcnow)  # Timestamp
    data = db.Column(db.Text, nullable=False)  # Data field

# Create the database tables
with app.app_context():
    db.create_all()
    print("Database and tables created successfully.")


@app.route('/data', methods=['GET', 'POST'])
def data():
    if request.method == 'GET':
        # Retrieve all stored data
        try:
            records = ESPData.query.all()
            result = [
                {"server": "connected"}
            ]
            return jsonify({"status": "success", "data": result}), 200
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500

    elif request.method == 'POST':
        # Handle POST request (data submission)
        data = request.json  # Get JSON data from the request
        if not data:
            return jsonify({"status": "error", "message": "No data provided"}), 400
    
        try:
            # Convert JSON to a properly formatted string
            formatted_data = json.dumps(data)
    
            # Save the formatted JSON string to the database
            new_entry = ESPData(data=formatted_data)
            db.session.add(new_entry)
            db.session.commit()
            return jsonify({"status": "success", "message": "Data stored successfully"}), 200
        except Exception as e:
            return jsonify({"status": "error", "message": f"Failed to store data: {str(e)}"}), 500

@app.route('/sensors', methods=['POST'])
def get_sensor():
    # Handle POST request (data submission)
    data = request.json  # Get JSON data from the request
    if not data:
        return jsonify({"status": "error", "message": "No data provided"}), 400

    try:
        # Convert JSON to a properly formatted string
        formatted_data = json.dumps(data)

        # Save the formatted JSON string to the database
        new_entry = ESPData(data=formatted_data)
        db.session.add(new_entry)
        db.session.commit()
        return jsonify({"status": "success", "message": "Data stored successfully"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": f"Failed to store data: {str(e)}"}), 500

# Dashboard API endpoints

@app.route('/latest', methods=['GET'])
def get_latest():
    # Fetch the most recent row in the database
    try:
        latest_data = ESPData.query.order_by(ESPData.timestamp.desc()).first()
        if not latest_data:
            return jsonify({"status": "error", "message": "No data available"}), 404

        # Convert the JSON string back into a dictionary

        structured_data = json.loads(latest_data.data)
        return jsonify({"status": "success", "data": structured_data}), 200
    except json.JSONDecodeError as e:
        return jsonify({"status": "error", "message": f"JSON decode error: {str(e)}"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"Failed to retrieve data: {str(e)}"}), 500

@app.route('/dashboard', methods=['GET'])
def serve_dashboard():
    return send_from_directory('.', 'dashboard.html')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
