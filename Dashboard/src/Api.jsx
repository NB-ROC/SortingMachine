import { useState, useEffect } from "react";

function Api() {
  const [sensorData, setSensorData] = useState(null);

  const ARDUINO_IP = "http://192.168.1.XX"; //To be filled in with correct information

  useEffect(() => {
    const interval = setInterval(async () => {
      const res = await fetch(`${ARDUINO_IP}/data`);

      const json = await res.json();

      setSensorData(json.sensor);
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  if (!sensorData) {
    return <div>Cannot receive data!</div>;
  }

  return <div>Sensor Value: {sensorData}</div>;
}

export default Api;
