// Inicialización de la pasarela de SocketIO
const socket = io();

/**
 * Muestra u oculta el panel interactivo del diagrama de la silla.
 */
function toggleInsights() {
    const panel = document.getElementById('debugPanel');
    const button = document.querySelector('.btn-insights');
    
    if (panel.style.display === 'none' || panel.style.display === '') {
        panel.style.display = 'block';
        button.innerText = "❌ Hide Insights & Diagram";
    } else {
        panel.style.display = 'none';
        button.innerText = "🔍 Show Insights & Diagram";
    }
}

/**
 * Helper para actualizar las clases de color de un set de nodos específicos.
 * @param {Array} nodeIds - IDs de los contenedores HTML de los sensores.
 * @param {String} state - Estado actual ("OK", "WARNING", "ALERT").
 */
function applyStateToNodes(nodeIds, state) {
    nodeIds.forEach(id => {
        const element = document.getElementById(id);
        if (element) {
            element.classList.remove('node-ok', 'node-warning', 'node-alert');
            if (state === "OK") element.classList.add('node-ok');
            else if (state === "WARNING") element.classList.add('node-warning');
            else if (state === "ALERT") element.classList.add('node-alert');
        }
    });
}

// Receptor global de actualizaciones del servidor
socket.on('update_chair', function(data) {
    let idPrefix = "";

    // 1. Enrutamiento por tópicos de datos
    if (data.topic === "sensors/seat/data") {
        idPrefix = "seat";
        // Actualización de valores numéricos individuales en el diagrama
        document.getElementById('v-seat-fl').innerText = data.raw_data.fl;
        document.getElementById('v-seat-fr').innerText = data.raw_data.fr;
        document.getElementById('v-seat-bl').innerText = data.raw_data.bl;
        document.getElementById('v-seat-br').innerText = data.raw_data.br;
        
        // Sincronizar colores de los nodos del asiento
        applyStateToNodes(['node-seat-fl', 'node-seat-fr', 'node-seat-bl', 'node-seat-br'], data.state);
    } 
    else if (data.topic === "sensors/back/data") {
        idPrefix = "back";
        // Actualiza el número de presión
        document.getElementById('v-back-up').innerText = data.raw_data.up;
        document.getElementById('v-back-low').innerText = data.raw_data.low;
        
        applyStateToNodes(['node-back-up', 'node-back-low'], data.state);
    } 
    else if (data.topic === "sensors/distance/data") {
        idPrefix = "distance";
        // Actualiza el número de distancia ToF
        document.getElementById('v-dist-up').innerText = data.raw_data.up;
        document.getElementById('v-dist-low').innerText = data.raw_data.low;
        
        applyStateToNodes(['node-dist-up', 'node-dist-low'], data.state);
    }
    else if (data.topic === "sensors/movement/data") {
        idPrefix = "movement";
        const isMotion = data.raw_data.movement_detected;
        const pirSpan = document.getElementById('v-pir');
        const pirBox = document.getElementById('node-pir');
        
        pirSpan.innerText = isMotion ? "MOTION DETECTED" : "CALM (NO MOTION)";

        // Limpiamos estilos de color previos y la clase de parpadeo
        pirBox.classList.remove('state-ok', 'state-warning', 'state-alert', 'pir-alert');
        
        applyStateToNodes(['node-pir'], data.state);
    }

    // 2. Actualización de las tarjetas principales del Dashboard superior
    if (idPrefix !== "") {
        const stateElement = document.getElementById(`${idPrefix}-state`);
        const timeElement = document.getElementById(`${idPrefix}-time`);

        stateElement.innerText = data.state;
        timeElement.innerText = data.time;

        stateElement.classList.remove('state-ok', 'state-warning', 'state-alert');

        if (data.state === "OK") stateElement.classList.add('state-ok');
        else if (data.state === "WARNING") stateElement.classList.add('state-warning');
        else if (data.state === "ALERT") stateElement.classList.add('state-alert');
    }
});