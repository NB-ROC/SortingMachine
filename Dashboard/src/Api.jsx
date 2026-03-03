// Api.jsx
import { useState, useEffect } from "react";

function Api({ setColors }) {
  const ARDUINO_IP = "http://192.168.1.XX";
  const [connected, setConnected] = useState(null); // null = pending

  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${ARDUINO_IP}/data`);
        const json = await res.json();
        setColors(json.colors);
        setConnected(true);
      } catch {
        setConnected(false);
      }
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  if (connected === null) return <div>Connecting to sensor...</div>;
  if (!connected) return <div>⚠️ Cannot receive data!</div>;
  return <div>✅ Sensor live</div>;
}

export default Api;
