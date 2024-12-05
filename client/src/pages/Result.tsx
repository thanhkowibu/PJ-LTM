import axios from "axios";
import { useEffect, useState } from "react";

const BASE_URL = "http://127.0.0.1:8080/api"

export const Result = () => {
  const [results, setResults] = useState([]);

  const fetchData = async () => {
    try {
      const response = await axios.get(`${BASE_URL}/game/1/result`, {
        headers: {
          'Content-Type': 'application/json',
        },
      });
      console.log(response.data);
      setResults(response.data);
    } catch (error) {
      console.error(error);
    }
  };

  useEffect(() => {
    fetchData();
  }, []);

  return (
    <div className="h-dvh relative">
      <div className="size-full brightness-[.25] transition duration-300">
        <img className="object-cover size-full" src="/result.jpg" alt="result" />
      </div>
      <div className="fixed top-16 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="text-4xl font-bold text-white">
          <span>Game Result</span>
        </div>
      </div>
      <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <table className="table-auto text-white text-4xl text-center font-semibold">
          <thead>
            <tr>
              <th className="px-8 py-6">Player ID</th>
              <th className="px-8 py-6">Score</th>
            </tr>
          </thead>
          <tbody>
            {results.map((result:any) => (
              <tr key={result.user_id}>
                <td className="border px-8 py-6">Player {result.user_id}</td>
                <td className="border px-4 py-2">{result.score}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}
