document.addEventListener("DOMContentLoaded", () => {
    const TOTAL_ARC_LENGTH = 400; 
    const NEEDLE_MIN_DEGREE = -120;
    const NEEDLE_MAX_DEGREE = 120;
    function updateDashboard(telemetryData) {
        if (!telemetryData) return;
        if (typeof telemetryData.batteryPercent !== 'undefined') {
            const bp = telemetryData.batteryPercent;
            let stateClass = 'state-green';
            let statusText = 'EXCELLENT';
            if (bp < 30) {
                stateClass = 'state-red';
                statusText = 'CRITICAL';
            } else if (bp <= 60) {
                stateClass = 'state-yellow';
                statusText = 'WARNING';
            }
            updateCardState('battery-card', stateClass);
            animateDigitalNumber('battery-value', bp);
            updateGaugeGraphics('battery-arc', 'battery-needle-group', bp, 0, 100);
            const fillWidth = Math.max(0, Math.min(10, (bp / 100) * 10));
            document.getElementById('icon-battery-fill').setAttribute('width', fillWidth.toString());
            document.getElementById('battery-status').innerText = statusText;
        }
        if (typeof telemetryData.rangeKm !== 'undefined') {
            const range = telemetryData.rangeKm;
            let stateClass = 'state-green';
            let statusText = 'OPTIMAL';            
            if (range < 80) {
                stateClass = 'state-red';
                statusText = 'LOW RANGE';
            } else if (range <= 200) {
                stateClass = 'state-yellow';
                statusText = 'ATTENTION';
            }           
            updateCardState('range-card', stateClass);
            animateDigitalNumber('range-value', Math.round(range));
            updateGaugeGraphics('range-arc', 'range-needle-group', range, 0, 500);
            document.getElementById('range-status').innerText = statusText;
        }
        if (typeof telemetryData.healthPercent !== 'undefined') {
            const health = telemetryData.healthPercent;
            let stateClass = 'state-green';
            let statusText = 'EXCELLENT';          
            if (health < 60) {
                stateClass = 'state-red';
                statusText = 'DEGRADED';
            } else if (health <= 80) {
                stateClass = 'state-yellow';
                statusText = 'SERVICE REQ';
            }      
            updateCardState('health-card', stateClass);
            animateDigitalNumber('health-value', health);
            updateGaugeGraphics('health-arc', 'health-needle-group', health, 0, 100);
            document.getElementById('health-status').innerText = statusText;
        }
        if (typeof telemetryData.temperatureC !== 'undefined') {
            const temp = telemetryData.temperatureC;
            let stateClass = 'state-green';
            let statusText = 'NOMINAL';
            
            if (temp > 50) {
                stateClass = 'state-red';
                statusText = 'OVERHEATING';
            } else if (temp >= 40) {
                stateClass = 'state-yellow';
                statusText = 'ELEVATED';
            }
            
            updateCardState('temp-card', stateClass);
            animateDigitalNumber('temp-value', temp);
            updateGaugeGraphics('temp-arc', 'temp-needle-group', temp, 0, 100);
            document.getElementById('temp-status').innerText = statusText;
            
            // Thermometer SVG dynamic path rendering update
            const mercuryPath = document.getElementById('icon-temp-mercury');
            if (temp > 45) {
                mercuryPath.setAttribute('d', 'M12 15V7'); // Extended visualization mapping
            } else {
                mercuryPath.setAttribute('d', 'M12 15V10'); // Standard visualization mapping
            }
        }
    }

    /**
     * Card Modular Structural Modifier Engine
     */
    function updateCardState(cardId, newClass) {
        const card = document.getElementById(cardId);
        if (!card) return;
        
        const trackingClasses = ['state-green', 'state-yellow', 'state-red'];
        trackingClasses.forEach(cls => {
            if (cls !== newClass) card.classList.remove(cls);
        });
        
        if (!card.classList.contains(newClass)) {
            card.classList.add(newClass);
        }
    }
    function updateGaugeGraphics(arcId, needleId, value, minVal, maxVal) {
        const arc = document.getElementById(arcId);
        const needle = document.getElementById(needleId);
        let percentage = (value - minVal) / (maxVal - minVal);
        percentage = Math.max(0, Math.min(1, percentage));
        const dashOffset = TOTAL_ARC_LENGTH * (1 - percentage);
        if (arc) arc.style.strokeDashoffset = dashOffset;
        const absoluteDegrees = NEEDLE_MIN_DEGREE + (percentage * (NEEDLE_MAX_DEGREE - NEEDLE_MIN_DEGREE));
        if (needle) needle.style.transform = `rotate(${absoluteDegrees}deg)`;
    }
    function animateDigitalNumber(targetId, targetValue) {
        const element = document.getElementById(targetId);
        if (!element) return;    
        const currentValue = parseFloat(element.innerText) || 0;
        if (currentValue === targetValue) return;      
        const duration = 450; // Milliseconds 
        const startTime = performance.now();     
        function step(now) {
            const elapsed = now - startTime;
            const progress = Math.min(elapsed / duration, 1);
            const easeProgress = 1 - Math.pow(1 - progress, 3);
            const currentRenderValue = currentValue + (targetValue - currentValue) * easeProgress;
            if (targetValue % 1 === 0) {
                element.innerText = Math.round(currentRenderValue).toString();
            } else {
                element.innerText = currentRenderValue.toFixed(1);
            }           
            if (progress < 1) {
                requestAnimationFrame(step);
            }
        }
        
        requestAnimationFrame(step);
    }
    let liveState = {
        batteryPercent: 82.0,
        rangeKm: 344.4,
        healthPercent: 96.0,
        temperatureC: 34.0
    }; 
    let simCycleTick = 0;
    function runTelemetrySimulation() {
        simCycleTick++;
        liveState.batteryPercent -= 0.04;
        if (liveState.batteryPercent <= 0) {
            liveState.batteryPercent = 100; 
        }
        const dischargeEfficiencyFactor = 4.2 - (0.002 * liveState.temperatureC);
        liveState.rangeKm = liveState.batteryPercent * dischargeEfficiencyFactor;
        if (simCycleTick % 30 === 0) {
            liveState.healthPercent -= 0.1;
            if (liveState.healthPercent < 45) liveState.healthPercent = 99.0;
        }
        const thermalLoadFactor = Math.sin(simCycleTick * 0.08) * 2.5;
        const speedDriftFactor = Math.cos(simCycleTick * 0.2) * 0.8;
        liveState.temperatureC = parseFloat((35.0 + thermalLoadFactor + speedDriftFactor).toFixed(1));
        liveState.batteryPercent = parseFloat(Math.max(0, Math.min(100, liveState.batteryPercent)).toFixed(1));
        liveState.healthPercent = parseFloat(Math.max(0, Math.min(100, liveState.healthPercent)).toFixed(1));
        updateDashboard(liveState);
    }
    // Stop the local simulation
// updateDashboard(liveState);
// setInterval(runTelemetrySimulation, 1000);

window.EV_DigitalTwin_Interface = {
    injectTelemetry: function(externalJsonData) {
        if (externalJsonData) {
            updateDashboard(externalJsonData);
        }
    }
};

// ================================
// Fetch data from Spring Boot API
// ================================
async function loadBatteryData() {
    try {
        const response = await fetch("http://localhost:8080/battery");
        const data = await response.json();

        updateDashboard({
            batteryPercent: data.batteryPercentage,
            rangeKm: data.remainingRange,
            healthPercent: data.batteryHealth,
            temperatureC: data.temperature
        });

    } catch (error) {
        console.error("Error fetching battery data:", error);
    }
}

// Load immediately
loadBatteryData();

// Refresh every second
setInterval(loadBatteryData, 1000);
});