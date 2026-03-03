// TopColors.jsx
export default function TopColors({ colors }) {
  const sorted = [...colors].sort((a, b) => b.amount - a.amount);
  const max = sorted[0]?.amount ?? 1;
  const medals = ["🥇", "🥈", "🥉", "🎗️"];

  return (
    <article>
      <hgroup>
        <h2>🎨 Top Colors</h2>
        <p>
          <small>Ranked by popularity · {sorted.length} colors total</small>
        </p>
      </hgroup>
      <hr />
      <table>
        <thead>
          <tr>
            <th>Rank</th>
            <th>Color</th>
            <th>Score</th>
            <th>Progress</th>
          </tr>
        </thead>
        <tbody>
          {sorted.map((color, i) => (
            <tr key={color.name}>
              <td>
                <strong>{medals[i]}</strong>
              </td>
              <td>
                <strong>
                  {color.name[0].toUpperCase() + color.name.slice(1)}
                </strong>
              </td>
              <td>{color.amount}</td>
              <td>
                <meter
                  value={color.amount}
                  min={0}
                  max={max}
                  low={max * 0.4}
                  high={max * 0.7}
                  optimum={max}
                />
              </td>
            </tr>
          ))}
        </tbody>
        <tfoot>
          <tr>
            <td colSpan={2}>
              <small>Total</small>
            </td>
            <td>
              <strong>{sorted.reduce((s, c) => s + c.amount, 0)}</strong>
            </td>
            <td />
          </tr>
        </tfoot>
      </table>
      <hr />
    </article>
  );
}
