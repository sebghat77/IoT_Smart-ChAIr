import time
import json
import paho.mqtt.client as mqtt

# ------- Configuración MQTT (Ajustada a tu Arduino) -------
BROKER = "localhost"  # IP de tu broker
PORT = 1883
SEAT_TOPIC = "sensors/seat/data"
BACK_TOPIC = "sensors/back/data"
MOVEMENT_TOPIC = "sensors/movement/data"

MQTT_PUBLISH_INTERVAL = 2.0  # Cada 2 segundos
MIN_WARNING_DURATION = 5.0   # 5 segundos para pasar de WARNING a ALERT

# Umbrales lógicos del ESP32
FSR_THRESHOLD = 500
FSR_BALANCE_TOLERANCE = 20

# Estados de alerta (Réplica del Enum de C++)
OK_STATE = "OK"
WARNING_STATE = "WARNING"
ALERT_STATE = "ALERT"

# Variables de control de tiempo para las alertas
start_seat_warning_time = 0
start_back_warning_time = 0
start_movement_warning_time = 0

# Inicializar cliente MQTT con la API v2
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

print(f"🔄 Conectando al broker {BROKER}...")
try:
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    print("¡Conectado al Broker con éxito!")
except Exception as e:
    print(f"❌ No se pudo conectar al broker: {e}")
    exit(1)

# ------- Funciones de Lógica de Negocio -------

def are_fsr_balanced(val1, val2):
    if val1 < FSR_THRESHOLD or val2 < FSR_THRESHOLD:
        return False
    average = (val1 + val2) / 2.0
    if average < FSR_THRESHOLD:
        return False
    difference = abs(val1 - val2)
    allowed_difference = average * FSR_BALANCE_TOLERANCE / 100.0
    return difference <= allowed_difference

def update_alert_state(is_ok, start_warning_time, current_time):
    """Réplica exacta de la función updateAlertState de Arduino"""
    if is_ok:
        return OK_STATE, 0
    else:
        if start_warning_time == 0:
            start_warning_time = current_time  # Guarda el momento exacto del fallo
        
        if (current_time - start_warning_time) >= MIN_WARNING_DURATION:
            return ALERT_STATE, start_warning_time
        else:
            return WARNING_STATE, start_warning_time

# ------- Bucle Principal Simulado -------

last_publish_time = time.time()
start_simulation_time = time.time()

try:
    while True:
        current_time = time.time()
        elapsed = current_time - start_simulation_time

        # --- SIMULACIÓN DE SENSORES EN TIEMPO REAL ---
        # El ciclo cambia de estado de forma automática cada 15 segundos
        if elapsed < 15:
            # Estado 1: Postura correcta
            seat_FL_val, seat_FR_val = 600, 610   # Equilibrado
            seat_BL_val, seat_BR_val = 700, 690   # Equilibrado
            back_UP_val, back_LOW_val = 800, 850  # Presión correcta
            pir_val = False                       # Sin movimiento
            print("\n--- [Simulación] Postura correcta y calma ---")
        elif elapsed < 30:
            # Estado 2: Asiento desequilibrado en la parte delantera
            seat_FL_val, seat_FR_val = 800, 300   # DESEQUILIBRADO
            seat_BL_val, seat_BR_val = 700, 690 
            back_UP_val, back_LOW_val = 800, 850
            pir_val = False
            print("\n--- [Simulación] Asiento desequilibrado (WARNING -> ALERT) ---")
        elif elapsed < 45:
            # Estado 3: Respaldo libre y mucho movimiento
            seat_FL_val, seat_FR_val = 600, 610
            seat_BL_val, seat_BR_val = 700, 690
            back_UP_val, back_LOW_val = 200, 150  # Sin apoyar espalda
            pir_val = True                        # Movimiento detectado
            print("\n--- [Simulación] Sin apoyar espalda y con movimiento continuo ---")
        else:
            # Reiniciar la línea de tiempo de la simulación
            start_simulation_time = time.time()
            continue

        # --- EVALUACIÓN DE REGLAS DE ARDUINO ---
        seat_ok = are_fsr_balanced(seat_FL_val, seat_FR_val) and are_fsr_balanced(seat_BL_val, seat_BR_val)
        back_ok = (back_UP_val > FSR_THRESHOLD) and (back_LOW_val > FSR_THRESHOLD)
        movement_ok = not pir_val

        # --- ACTUALIZACIÓN DE ESTADOS TEMPORALES ---
        seatState, start_seat_warning_time = update_alert_state(seat_ok, start_seat_warning_time, current_time)
        backState, start_back_warning_time = update_alert_state(back_ok, start_back_warning_time, current_time)
        movementState, start_movement_warning_time = update_alert_state(movement_ok, start_movement_warning_time, current_time)

        # Monitor serial simulado en consola
        print(f"  [Estados actuales] Asiento: {seatState} | Respaldo: {backState} | Movimiento: {movementState}")

        # --- PUBLICACIÓN MQTT EN FORMATO JSON (IDÉNTICO AL ESP32) ---
        if current_time - last_publish_time >= MQTT_PUBLISH_INTERVAL:
            last_publish_time = current_time
            
            print("  📤 Publicando JSONs estructurados vía MQTT...")

            # 1. JSON del Asiento
            payload_seat = {
                "state": seatState,
                "fl": seat_FL_val,
                "fr": seat_FR_val,
                "bl": seat_BL_val,
                "br": seat_BR_val
            }
            client.publish(SEAT_TOPIC, json.dumps(payload_seat))

            # 2. JSON del Respaldo
            payload_back = {
                "state": backState,
                "up": back_UP_val,
                "low": back_LOW_val
            }
            client.publish(BACK_TOPIC, json.dumps(payload_back))

            # 3. JSON de Movimiento
            payload_movement = {
                "state": movementState,
                "movement_detected": pir_val
            }
            client.publish(MOVEMENT_TOPIC, json.dumps(payload_movement))

        # Frecuencia de muestreo (1 segundo para evaluar las alertas de tiempo con precisión)
        time.sleep(1) 

except KeyboardInterrupt:
    print("\n👋 Deteniendo el simulador de la silla inteligente...")
    client.loop_stop()
    client.disconnect()