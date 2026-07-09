import time
import random
import paho.mqtt.client as mqtt

BROKER = "localhost"
PORT = 1883
SEAT_TOPIC = "sensors/seat/state"
BACK_TOPIC = "sensors/back/state"
MOVEMENT_TOPIC = "sensors/movement/state"

MQTT_PUBLISH_INTERVAL = 2.0  # 2 segundos
MIN_ALERT_DURATION = 5.0     # 5 segundos para que pase a VALID ALERT

# Umbrales lógicos del ESP32
FSR_THRESHOLD = 500
FSR_BALANCE_TOLERANCE = 20

# Estados de alerta (Réplica del Enum de C++)
OK_STATE = "OK"
PRE_ALERT_STATE = "WARNING"
VALID_ALERT_STATE = "ALERT"

# Variables de control de tiempo para las alertas
start_seat_alert_time = 0
start_back_alert_time = 0
start_movement_alert_time = 0

# Inicializar cliente MQTT
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

print(f"🔄 Conectando al broker {BROKER}...")
try:
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    print("¡Conectado al Broker!")
except Exception as e:
    print(f"No se pudo conectar al broker: {e}")
    exit(1)

# ------- Funciones de Postura Simulada -------

def are_fsr_balanced(val1, val2):
    if val1 < FSR_THRESHOLD or val2 < FSR_THRESHOLD:
        return False
    average = (val1 + val2) / 2.0
    if average < FSR_THRESHOLD:
        return False
    difference = abs(val1 - val2)
    allowed_difference = average * FSR_BALANCE_TOLERANCE / 100.0
    return difference <= allowed_difference

def update_alert_state(is_ok, start_alert_time, current_time):
    """Réplica exacta de la lógica por referencia del ESP32"""
    if is_ok:
        return OK_STATE, 0
    else:
        if start_alert_time == 0:
            start_alert_time = current_time  # Guarda cuando empezó el fallo
        
        if (current_time - start_alert_time) >= MIN_ALERT_DURATION:
            return VALID_ALERT_STATE, start_alert_time
        else:
            return PRE_ALERT_STATE, start_alert_time

# ------- Bucle Principal Simulado -------

last_publish_time = time.time()
start_simulation_time = time.time()

try:
    while True:
        current_time = time.time()
        elapsed = current_time - start_simulation_time

        # --- SIMULACIÓN DE SENSORES EN TIEMPO REAL ---
        # Cambiamos las posturas automáticamente cada 15 segundos para probar los estados
        if elapsed < 15:
            # Estado 1: Todo perfecto
            fsr_fl, fsr_fr = 600, 610   # Asiento equilibrado
            fsr_bl, fsr_br = 700, 690   # Asiento equilibrado
            fsr_up, fsr_low = 800, 850  # Respaldo presionado (OK)
            pir_motion = False          # Calmado
            print("\n--- [Simulación] Postura correcta y calma ---")
        elif elapsed < 30:
            # Estado 2: Mala postura en el asiento (Desequilibrado delantero)
            fsr_fl, fsr_fr = 800, 300   # DESEQUILIBRADO
            fsr_bl, fsr_br = 700, 690 
            fsr_up, fsr_low = 800, 850
            pir_motion = False
            print("\n--- [Simulación] Asiento desequilibrado (Debe generar PRE -> VALID) ---")
        elif elapsed < 45:
            # Estado 3: No apoya la espalda y se mueve mucho
            fsr_fl, fsr_fr = 600, 610
            fsr_bl, fsr_br = 700, 690
            fsr_up, fsr_low = 200, 150  # Respaldo sin presión (Fallo)
            pir_motion = True           # Nervioso / Movimiento (Fallo)
            print("\n--- [Simulación] Sin respaldar y con mucho movimiento ---")
        else:
            # Reiniciar ciclo de simulación
            start_simulation_time = time.time()
            continue

        # --- EVALUACIÓN DE REGLAS DE NEGOCIO ---
        seat_ok = are_fsr_balanced(fsr_fl, fsr_fr) and are_fsr_balanced(fsr_bl, fsr_br)
        back_ok = (fsr_up > FSR_THRESHOLD) and (fsr_low > FSR_THRESHOLD)
        movement_ok = not pir_motion

        # --- ACTUALIZACIÓN DE ESTADOS (Manejo del tiempo) ---
        seat_state, start_seat_alert_time = update_alert_state(seat_ok, start_seat_alert_time, current_time)
        back_state, start_back_alert_time = update_alert_state(back_ok, start_back_alert_time, current_time)
        movement_state, start_movement_alert_time = update_alert_state(movement_ok, start_movement_alert_time, current_time)

        # Imprimir Debug por consola local (como el ESP32)
        print(f"  [Valores] Asiento FL:{fsr_fl} FR:{fsr_fr} | Respaldo UP:{fsr_up} LOW:{fsr_low} | PIR Movimiento: {pir_motion}")
        print(f"  [Estados] Asiento: {seat_state} | Respaldo: {back_state} | Movimiento: {movement_state}")

        # --- PUBLICACIÓN MQTT CADA 2 SEGUNDOS ---
        if current_time - last_publish_time >= MQTT_PUBLISH_INTERVAL:
            last_publish_time = current_time
            
            print("  📤 Publicando estados vía MQTT...")
            client.publish(SEAT_TOPIC, seat_state)
            client.publish(BACK_TOPIC, back_state)
            client.publish(MOVEMENT_TOPIC, movement_state)

        time.sleep(1) # El bucle corre cada segundo para evaluar el tiempo de PRE-ALERT con precisión

except KeyboardInterrupt:
    print("\n👋 Deteniendo el simulador...")
    client.loop_stop()
    client.disconnect()