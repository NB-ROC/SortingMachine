// App.jsx
import "./App.css";

function Embed({ color, children }) {
  return (
    <div className="embed" color={color}>
      {children}
    </div>
  );
}

export default Embed;
