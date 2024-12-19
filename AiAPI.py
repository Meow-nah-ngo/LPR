from flask import Flask, request, jsonify
from google.oauth2 import service_account
from googleapiclient.discovery import build
import json
app = Flask(__name__)
# กำหนดข้อมูลไฟล์ credentials
SERVICE_ACCOUNT_FILE = "C:\Users\USER\Downloads\fabled-progress-445115-h7-e5474ab6cdbc.json" # แก้ไขที่อยู่ไฟล์ JSON ของคุณ
SCOPES = ['https://www.googleapis.com/auth/spreadsheets']
# กำหนด Spreadsheet ID และ Range ที่ต้องการ
SPREADSHEET_ID = "fabled-progress-445115-h7" # แก้ไขเป็น ID ของ Google Sheets ของคุณ
RANGE_NAME = 'Sheet1!A2:H2' # แก้ไขเป็น Range ที่คุณต้องการ
# ฟังก์ชันการเชื่อมต่อกับ Google Sheets API
def get_sheets_service():
    credentials = service_account.Credentials.from_service_account_file(
        SERVICE_ACCOUNT_FILE, scopes=SCOPES)
    service = build('sheets', 'v4', credentials=credentials)
    return service
# ฟังก์ชันเพื่อเขียนข้อมูลลง Google Sheets
def write_to_sheets(data):
    service = get_sheets_service()
    values = [
        [data['timestamp'], data['license_plate'], data['province'], data['entry_time'], data['exit_time'], data['image_link'], data['available_spaces'], data['vehicle_count']]
    ]
    body = {'values': values}
    service.spreadsheets().values().append(
        spreadsheetId=SPREADSHEET_ID, range=RANGE_NAME,
        valueInputOption="RAW", body=body).execute()
@app.route('/process_image', methods=['POST'])
def process_image():
    if 'json' not in request.json:
        return jsonify({"error": "No JSON data provided"}), 400
    # รับข้อมูล JSON จาก request
    data = request.json
    # เขียนข้อมูลลง Google Sheets
    try:
        write_to_sheets(data)
        return jsonify({"status": "Data written to Google Sheets successfully!"})
    except Exception as e:
        return jsonify({"error": str(e)}), 500
if __name__ == '__main__':
    app.run(host="0.0.0.0", port=8080)