import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
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
  Filler,
);

const data = {
  labels: ["Mon", "Tue", "Wed", "Thu", "Fri"],
  datasets: [
    {
      label: "Red",
      data: [0, 15, 29, 35, 45],
      backgroundColor: "rgba(239, 68, 68, 0.1)",
      borderColor: "rgb(239, 68, 68)",
      borderWidth: 2.5,
      pointBackgroundColor: "rgb(239, 68, 68)",
      pointRadius: 5,
      pointHoverRadius: 7,
      tension: 0.4,
      fill: true,
    },
    {
      label: "Blue",
      data: [2, 14, 26, 36, 38],
      backgroundColor: "rgba(59, 130, 246, 0.1)",
      borderColor: "rgb(59, 130, 246)",
      borderWidth: 2.5,
      pointBackgroundColor: "rgb(59, 130, 246)",
      pointRadius: 5,
      pointHoverRadius: 7,
      tension: 0.4,
      fill: true,
    },
    {
      label: "Green",
      data: [5, 9, 14, 18, 22],
      backgroundColor: "rgba(34, 197, 94, 0.1)",
      borderColor: "rgb(34, 197, 94)",
      borderWidth: 2.5,
      pointBackgroundColor: "rgb(34, 197, 94)",
      pointRadius: 5,
      pointHoverRadius: 7,
      tension: 0.4,
      fill: true,
    }
  ],
};

const options = {
  responsive: true,
  interaction: {
    mode: "index",
    intersect: false,
  },
  plugins: {
    legend: {
      position: "top",
      labels: {
        usePointStyle: true,
        pointStyle: "circle",
        padding: 20,
        font: { size: 13 },
      },
    },
    title: {
      display: true,
      text: "Colors Detected",
      font: { size: 18, weight: "bold" },
      padding: { bottom: 20 },
    },
    tooltip: {
      backgroundColor: "rgba(0,0,0,0.75)",
      padding: 12,
      cornerRadius: 8,
      titleFont: { size: 13 },
      bodyFont: { size: 13 },
    },
  },
  scales: {
    x: {
      grid: { display: false },
      ticks: { font: { size: 12 } },
    },
    y: {
      beginAtZero: true,
      grid: { color: "rgba(0,0,0,0.06)" },
      ticks: { font: { size: 12 } },
    },
  },
};

export default function ColorChart() {
  return <Line data={data} options={options} />;
}
