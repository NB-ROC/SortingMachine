import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import "./index.css";
import Embed from "./App.jsx";

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <div className="grid">
      <Embed color={"red"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"teal"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"green"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"yellow"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"blue"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"dark"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
      <Embed color={"white"}>
        <h1>Titel</h1>
        <h2>Test1Test2Test3</h2>
      </Embed>
    </div>
  </StrictMode>,
);
