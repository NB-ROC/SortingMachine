import "./App.css";

function App() {
  return (
    <>
      <div>
        <p>Test</p>
      </div>
    </>
  );
}

function Embed(color) {
  return (
    <>
      <div className="embed" color={color.color}>
        <p>EMBED TEST</p>
      </div>
    </>
  );
}

export { App };
export default Embed;
