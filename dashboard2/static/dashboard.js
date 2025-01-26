/**
 * dashboard.js
 * Client-side script to fetch sensor data from /data
 * and display them using Chart.js.
 *
 * For each sensor_type in the data, we create a chart that
 * shows the relevant numeric fields vs. timestamp.
 */

async function fetchSensorData() {
  // Get JSON from the server endpoint /data
  const response = await fetch("/data");
  const data = await response.json(); // e.g. {light: [...], gas: [...], ...}
  return data;
}

/**
 * Creates a chart for one sensor type.
 * @param {string} sensorType - e.g. "light"
 * @param {Object[]} records   - array of { timestamp: ISOString, data: {...} }
 *
 * We'll try to identify numeric fields in 'data' object
 * (except sensor_type) and plot them over time.
 */
function createSensorChart(sensorType, records) {
  // 1. Identify all numeric keys (besides "sensor_type") in the data
  //    We assume each record.data has consistent keys for that sensor type
  //    e.g. for "temperature_humidity", keys: ["temperature_c", "humidity", ...]
  const numericKeys = new Set();
  records.forEach((rec) => {
    for (let [k, v] of Object.entries(rec.data)) {
      if (k !== "sensor_type" && typeof v === "number") {
        numericKeys.add(k);
      }
    }
  });

  // Convert Set to Array
  const fields = Array.from(numericKeys);

  // 2. Prepare x-axis (timestamps) and y-values for each field
  //    We'll create one line dataset per numeric field
  const datasets = fields.map((fieldName) => {
    return {
      label: fieldName,
      data: records.map((r) => {
        return {
          x: new Date(r.timestamp), // x is the timestamp
          y: r.data[fieldName]      // y is the numeric value
        };
      }),
      borderWidth: 1,
      borderColor: getRandomColor(),
      fill: false
    };
  });

  // 3. Create a <canvas> element for this chart
  const chartContainer = document.getElementById("charts-area");
  const canvas = document.createElement("canvas");
  canvas.style.maxWidth = "1000px"; // limit width
  canvas.style.height = "400px";    // set a height
  chartContainer.appendChild(canvas);

  // 4. Create Chart.js line chart
  new Chart(canvas, {
    type: "line",
    data: {
      datasets: datasets
    },
    options: {
      plugins: {
        title: {
          display: true,
          text: `Sensor Type: ${sensorType}`
        }
      },
      responsive: true,
      scales: {
        x: {
          type: "time",
          time: {
            unit: "minute"
          },
          title: {
            display: true,
            text: "Timestamp"
          }
        },
        y: {
          title: {
            display: true,
            text: "Value"
          },
          beginAtZero: false
        }
      }
    }
  });
}

/**
 * Returns a random color string (e.g. 'rgb(255, 99, 132)').
 * Useful for distinguishing multiple lines in a chart.
 */
function getRandomColor() {
  const r = Math.floor(Math.random() * 200);
  const g = Math.floor(Math.random() * 200);
  const b = Math.floor(Math.random() * 200);
  return `rgb(${r}, ${g}, ${b})`;
}

window.addEventListener("DOMContentLoaded", async () => {
  const data = await fetchSensorData(); // e.g. {light: [...], temperature_humidity: [...], ...}

  // data is structured by sensor_type
  // e.g. data.light = [ {timestamp: "...", data: {sensor_type:"light", lux:...}}, ... ]
  // We'll create one chart per sensor_type

  for (let sensorType in data) {
    const records = data[sensorType];
    if (records.length > 0) {
      createSensorChart(sensorType, records);
    }
  }
});

