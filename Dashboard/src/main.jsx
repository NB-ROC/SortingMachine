import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import Embed from "./App.jsx";
import Api from "./Api.jsx";

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <div className="grid">
      <Embed color="yellow"> <Api /> </Embed>
      <Embed color="green"> <Api /> </Embed>

    </div>
  </StrictMode>,
);
