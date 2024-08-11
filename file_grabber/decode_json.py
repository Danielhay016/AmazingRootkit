# python3 decode_json.py grabbed.json grabbed gg.zip

import json
import base64
import argparse
import os

def save_data_from_json(json_file_path, key, output_image_path):
    try:
        # Load the JSON data from the file
        with open(json_file_path, 'r') as file:
            data = json.load(file)
        
        base64_data = data.get(key)
        if base64_data is None:
            raise ValueError("The' key is not found in the JSON file.")
        
        decoded_data = base64.b64decode(base64_data)
        
        with open(output_image_path, 'wb') as image_file:
            image_file.write(decoded_data)
        
        print(f"decoded_data saved successfully to {output_image_path}")
    
    except FileNotFoundError:
        print(f"Error: The file {json_file_path} was not found.")
    except json.JSONDecodeError:
        print(f"Error: Failed to decode JSON from the file {json_file_path}.")
    except Exception as e:
        print(f"An error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(description='Decode and save encoded data from a JSON file.')
    parser.add_argument('json_file_path', type=str, help='The path to the JSON file.')
    parser.add_argument('key', type=str, help='Valuw of which key to decode')
    parser.add_argument('output_path', type=str, help='The path where the decoded image will be saved.')

    args = parser.parse_args()

    save_data_from_json(args.json_file_path, args.key, args.output_path)

if __name__ == '__main__':
    main()
