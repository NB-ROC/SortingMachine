import "./App.css";

function App() {
  return <></>;
}

function Embed({color, children}) {
  return (
    <>
      <div className="embed" color={color}>
        {children}
      </div>
    </>
  );
}

export { App };
export default Embed;
