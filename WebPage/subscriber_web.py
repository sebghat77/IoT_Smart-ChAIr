import datetime
import json
import paho.mqtt.client as mqtt
from flask import Flask, render_template
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'intelligent_chair_secret_key_abc123'
socketio = SocketIO(app, cors_allowed_origins="*")

BROKER = "localhost"
PORT = 1883

# Tópicos idénticos a los del ESP32
TOPICS = [
    "sensors/seat/data",
    "sensors/back/data",
    "sensors/movement/data",
    "sensors/distance/data"
]

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("✅ Server connected to the MQTT Broker!")
        for topic in TOPICS:
            client.subscribe(topic)
            print(f"   Subscribed to: {topic}")
    else:
        print(f"❌ MQTT Connection Error. Code: {rc}")

def on_message(client, userdata, msg):
    topic = msg.topic
    payload_str = msg.payload.decode('utf-8')
    
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"📥 [{now}] {topic} -> {payload_str}")
    
    try:
        data_json = json.loads(payload_str)
        
        socketio.emit('update_chair', {
            'topic': topic,
            'state': data_json.get('state', 'UNKNOWN'),
            'raw_data': data_json,
            'time': now
        })
    except Exception as e:
        print(f"❌ Error decoding JSON: {e}")


mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(BROKER, PORT, 60)
mqtt_client.loop_start()


@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    print("🚀 Monitoring server running in http://localhost:5000")
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)