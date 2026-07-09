import time
import json
import random
import paho.mqtt.client as mqtt

# ------- Configuración MQTT -------
BROKER = "localhost"  # Cambiar por la IP del Broker si es necesario
PORT = 1883
SEAT_TOPIC = "sensors/seat/data"
BACK_TOPIC = "sensors/back/data"
MOVEMENT_TOPIC = "sensors/movement/data"
DISTANCE_TOPIC = "sensors/distance/data"

MQTT_PUBLISH_INTERVAL = 4.0  # Publicación estricta cada 2 segundos

# Estados del sistema
OK_STATE = "OK"
WARNING_STATE = "WARNING"
ALERT_STATE = "ALERT"

# Inicializar cliente MQTT
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

print(f"🔄 Conectando al broker MQTT en {BROKER}:{PORT}...")
try:
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    print("✅ ¡Conectado al Broker con éxito!")
except Exception as e:
    print(f"❌ No se pudo conectar al broker: {e}")
    exit(1)

print("🚀 Iniciando bucle de envío continuo de datos (Ctrl+C para detener)...")

try:
    while True:
        # --- 1. SEAT DATA (Combinación aleatoria simple) ---
        seat_state = random.choice([OK_STATE, WARNING_STATE, ALERT_STATE])
        payload_seat = {
            "state": seat_state,
            "fl": random.randint(550, 750) if seat_state == OK_STATE else random.randint(100, 450),
            "fr": random.randint(550, 750),
            "bl": random.randint(550, 750),
            "br": random.randint(550, 750)
        }

        # --- 2. MOVEMENT DATA (Combinación aleatoria simple) ---
        movement_state = random.choice([OK_STATE, WARNING_STATE, ALERT_STATE])
        payload_movement = {
            "state": movement_state,
            "movement_detected": True if movement_state in [WARNING_STATE, ALERT_STATE] else False
        }

        # --- 3. LOGICA EXCLUSIVA: BACK VS DISTANCE ---
        # Decidimos de forma aleatoria cuál de los dos va a estar en "OK"
        # Esto asegura que si uno falla, el otro obligatoriamente se mantiene a salvo.
        if random.choice([True, False]):
            # Caso A: Espalda está OK, Distancia puede fallar
            back_state = OK_STATE
            distance_state = random.choice([WARNING_STATE, ALERT_STATE])
        else:
            # Caso B: Distancia está OK, Espalda puede fallar
            distance_state = OK_STATE
            back_state = random.choice([WARNING_STATE, ALERT_STATE])

        # Generamos valores coherentes para el respaldo según su estado dictaminado
        if back_state == OK_STATE:
            up_press, low_press = random.randint(600, 900), random.randint(600, 900)
        else:
            up_press, low_press = random.randint(100, 400), random.randint(100, 400)

        payload_back = {
            "state": back_state,
            "up": up_press,
            "low": low_press
        }

        # Generamos valores coherentes para la distancia (ToF) según su estado dictaminado
        if distance_state == OK_STATE:
            up_dist, low_dist = random.randint(10, 30), random.randint(10, 30) # mm (Cerca)
        else:
            up_dist, low_dist = random.randint(150, 300), random.randint(150, 300) # mm (Lejos)

        payload_distance = {
            "state": WARNING_STATE,
            "up": up_dist,
            "low": low_dist
        }

        # --- 4. PUBLICACIÓN SIMULTÁNEA VÍA MQTT ---
        print(f"\n📤 [{time.strftime('%H:%M:%S')}] Enviando ráfaga de datos:")
        print(f"   Seat: {seat_state} | Back: {back_state} | Distance: {distance_state} | Move: {movement_state}")
        
        client.publish(SEAT_TOPIC, json.dumps(payload_seat))
        client.publish(BACK_TOPIC, json.dumps(payload_back))
        client.publish(MOVEMENT_TOPIC, json.dumps(payload_movement))
        client.publish(DISTANCE_TOPIC, json.dumps(payload_distance))

        # Pausa estricta de 2 segundos antes de la siguiente combinación
        time.sleep(MQTT_PUBLISH_INTERVAL)

except KeyboardInterrupt:
    print("\n👋 Deteniendo el publicador de pruebas de la silla...")
    client.loop_stop()
    client.disconnect()