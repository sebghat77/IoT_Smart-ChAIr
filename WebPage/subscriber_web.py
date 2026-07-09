import datetime
import paho.mqtt.client as mqtt
from flask import Flask, render_template
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'silla_inteligente_secret_key_abc123'
socketio = SocketIO(app, cors_allowed_origins="*")

BROKER = "localhost"
PORT = 1883

TOPICS = [
    "sensors/seat/state",
    "sensors/back/state",
    "sensors/movement/state"
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
    payload = msg.payload.decode('utf-8')
    
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    print(f"📥 [{now}] {topic} -> {payload}")
    
    # Send a JSON packet to the frontend via SocketIO
    socketio.emit('update_chair', {
        'topic': topic,
        'state': payload,
        'time': now
    })


mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(BROKER, PORT, 60)
mqtt_client.loop_start()


@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    print("🚀 Servidor de Monitoreo corriendo en http://localhost:5000")
    # Note: 'debug=True' is disabled to prevent duplicate MQTT background threads and strange double connections.
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)