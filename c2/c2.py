from flask import Flask, request, jsonify

app = Flask(__name__)

# Route to handle registration
@app.route('/c2/register', methods=['POST'])
def register():
    data = request.json
    print("New Cliend id: " + data["client_id"])
    response = {
        'status': 'ok',
    }
    return jsonify(response), 200

# Route to handle new commands
@app.route('/c2/new_command', methods=['POST'])
def new_command():
    data = request.json  # Get the JSON data from the request
    # Process the command data (e.g., execute it, store it, etc.)
    response = {
        'status': 'success',
        'message': 'Command received',
        'received_data': data
    }
    return jsonify(response), 200

# Route to handle ping requests
@app.route('/c2/ping', methods=['POST'])
def ping():
    # You might return a simple acknowledgment for a ping
    response = {
        'status': 'success',
        'message': 'Ping acknowledged'
    }
    return jsonify(response), 200

# Route to handle sending artifacts
@app.route('/c2/send_artifact', methods=['POST'])
def send_artifact():
    data = request.json  # Get the JSON data from the request
    # Process the artifact data (e.g., save the artifact, validate it, etc.)
    response = {
        'status': 'success',
        'message': 'Artifact received',
        'received_data': data
    }
    return jsonify(response), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=1234)
