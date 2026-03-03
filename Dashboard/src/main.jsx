// index.jsx
import { StrictMode, useState } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import Embed from "./App.jsx";
import ColorChart from "./Chart.jsx";
import TopColors from "./TopColors.jsx";
import Api from "./Api.jsx";

function Root() {
  const [colors, setColors] = useState([
    { name: "red", amount: 45 },
    { name: "blue", amount: 38 },
    { name: "green", amount: 32 },
  ]);

  return (
    <div className="grid">
      <Embed color="dark">
        <Api setColors={setColors} />
      </Embed>
      <Embed color="white">
        <ColorChart />
      </Embed>
      <Embed color="slate">
        <TopColors colors={colors} />
      </Embed>
    </div>
  );
}

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <Root />
  </StrictMode>,
);
