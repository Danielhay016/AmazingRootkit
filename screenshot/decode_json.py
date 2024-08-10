# python3 decode_json.py screenshot.json aa.png

import json
import base64
import argparse
import os

def save_screenshot_from_json(json_file_path, output_image_path):
    """
    Loads a JSON file, decodes the 'screenshot' key from base64, and saves it to disk.

    :param json_file_path: The path to the JSON file.
    :param output_image_path: The path where the decoded image will be saved.
    """
    try:
        # Load the JSON data from the file
        with open(json_file_path, 'r') as file:
            data = json.load(file)
        
        # Extract the base64 encoded screenshot
        base64_screenshot = data.get('screenshot')
        if base64_screenshot is None:
            raise ValueError("The 'screenshot' key is not found in the JSON file.")
        
        screenshot_data = base64.b64decode(base64_screenshot)
        
        with open(output_image_path, 'wb') as image_file:
            image_file.write(screenshot_data)
        
        print(f"Screenshot saved successfully to {output_image_path}")
    
    except FileNotFoundError:
        print(f"Error: The file {json_file_path} was not found.")
    except json.JSONDecodeError:
        print(f"Error: Failed to decode JSON from the file {json_file_path}.")
    except Exception as e:
        print(f"An error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(description='Decode and save a screenshot from a JSON file.')
    parser.add_argument('json_file_path', type=str, help='The path to the JSON file.')
    parser.add_argument('output_image_path', type=str, help='The path where the decoded image will be saved.')

    args = parser.parse_args()

    save_screenshot_from_json(args.json_file_path, args.output_image_path)

if __name__ == '__main__':
    main()
