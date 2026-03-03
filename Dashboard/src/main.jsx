import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import Embed from "./App.jsx";
import MyChart from "./Chart.jsx";

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <div className="grid">
      <Embed color={"white"}>
        <MyChart></MyChart>
      </Embed>
    </div>
  </StrictMode>,
);
