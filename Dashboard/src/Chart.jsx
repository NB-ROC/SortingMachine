import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from "chart.js";
import { Line } from "react-chartjs-2";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
);

const data = {
  labels: ["", "", "", "", "", "", "", "", ""],
  datasets: [
    {
      label: "Red Detected",
      data: [0, 1, 3, 3, 7],
      backgroundColor: "rgba(241, 99, 99, 0.6)",
      borderColor: "rgb(241, 99, 99)",
      borderWidth: 1,
    },
    {
      label: "Blue Detected",
      data: [2, 4, 6, 8, 11],
      backgroundColor: "rgba(99, 149, 241, 0.6)",
      borderColor: "rgb(99, 149, 241)",
      borderWidth: 1,
    },
    {
      label: "Green Detected",
      data: [5, 9, 14, 18, 28],
      backgroundColor: "rgba(99, 211, 99, 0.6)",
      borderColor: "rgb(99, 211, 99)",
      borderWidth: 1,
    },
  ],
};

const options = {
  responsive: true,
  plugins: {
    legend: { position: "top" },
    title: { display: true, text: "Colors Detected" },
  },
};

export default function MyChart() {
  return <Line data={data} options={options} />;
}
