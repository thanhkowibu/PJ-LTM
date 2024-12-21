import { cn } from "@/lib/utils"
import { useEffect, useState } from "react";
import axios from 'axios';
import { useNavigate, useParams } from "react-router-dom";

const BASE_URL = "http://localhost:8080/api"

export const IngameRoom = () => {
  const navigate = useNavigate();
  const { id } = useParams<{ id: string }>();

  const [isShown, setIsShown] = useState(false);
  const [name1, setName1] = useState("");
  const [name2, setName2] = useState("");
  const [pic1, setPic1] = useState("");
  const [pic2, setPic2] = useState("");
  const [unit, setUnit] = useState("");
  const [score, setScore] = useState(0);

  const username = localStorage.getItem("username")

  // animation
  const [countdown, setCountdown] = useState(200); // 200 for 20 seconds with 0.1s interval
  const [isTimerRunning, setIsTimerRunning] = useState(true);
  const [animatedNumber1, setAnimatedNumber1] = useState(0);
  const [animatedNumber2, setAnimatedNumber2] = useState(0);
  const [value1, setValue1] = useState(0);
  const [value2, setValue2] = useState(0);
  
  useEffect(() => {
    if (isShown) {
      const duration = 1500;
      const startTime = performance.now();

      const animate1 = (currentTime: number) => {
        const elapsedTime = currentTime - startTime;
        const progress = Math.min(elapsedTime / duration, 1);
        const currentNumber = Math.floor(progress * value1);
        setAnimatedNumber1(currentNumber);

        if (progress < 1) {
          requestAnimationFrame(animate1);
        }
      };

      const animate2 = (currentTime: number) => {
        const elapsedTime = currentTime - startTime;
        const progress = Math.min(elapsedTime / duration, 1);
        const currentNumber = Math.floor(progress * value2);
        setAnimatedNumber2(currentNumber);

        if (progress < 1) {
          requestAnimationFrame(animate2);
        }
      };

      requestAnimationFrame(animate1);
      requestAnimationFrame(animate2);
    }
  }, [isShown, value1, value2]);

  useEffect(() => {
    if (countdown > 0 && isTimerRunning) {
      const timer = setInterval(() => {
        setCountdown(prevCountdown => prevCountdown - 1);
      }, 100);
      return () => clearInterval(timer);
    } else if (countdown === 0) {
      setIsShown(true);
    }
  }, [countdown, isTimerRunning]);

  // game logic
  const fetchData = async () => {

    try {
      const response = await axios.post(`${BASE_URL}/game`, { room_name: id }, {
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true // Ensure cookies are sent with the request
      });
      console.log(response.data);
      setName1(response.data.name1);
      setName2(response.data.name2);
      setPic1(response.data.pic1);
      setPic2(response.data.pic2);
      setUnit(response.data.unit);
      setIsShown(false);
      setIsTimerRunning(true);
      setCountdown(200);
    } catch (error) {
      console.error(error);
    }
  };
    // Start Server-Sent Events (SSE)
    const startSSE = () => {
      const eventSource = new EventSource(`${BASE_URL}/subscribe`);
    
      eventSource.onmessage = (event) => {
        console.log("SSE event data:", event.data);
        const data = JSON.parse(event.data);
        if (data.action === "next" && data.room_name === id) {
          console.log("Calling fetchData due to Next event");
          setTimeout(() => {
            fetchData();
          }, 3500);
        } else if (data.action === "finish" && data.room_name === id) {
          console.log("Navigating to /result/1 due to Finish event");
          setTimeout(() => {
            navigate(`/result/${id}`);
          }, 3500);
        }
      };
    
      eventSource.onerror = () => {
        console.error('SSE connection error. Reconnecting...');
        eventSource.close();
    
        // Retry connection after a delay
        setTimeout(startSSE, 5000);
      };
    };

  useEffect(() => {
    fetchData();
    startSSE();
  }, []);

  const handleChoice = async (choice: number) => {
    if (countdown > 0) {
      setIsShown(true);
      setIsTimerRunning(false);
      console.log(choice);
      try {
        const response = await axios.post(`${BASE_URL}/game/choice`, {
          choice,
          room_name: id
        }, {
          headers: {
            'Content-Type': 'application/json',
          },
          withCredentials: true
        });
        console.log(response.data);
        setScore((pv) => pv + response.data.score);
        setValue1(response.data.value1);
        setValue2(response.data.value2);
      } catch (error) {
        console.error(error);
      }
    }
  }

  const circumference = 2 * Math.PI * 60; // 60 is the radius of the outer circle
  const offset = circumference - (countdown / 200) * circumference;

  return (
    <div className="h-dvh grid grid-cols-2 relative">
      <div className={cn("brightness-50 hover:brightness-[.4] transition duration-300 cursor-pointer",
        {"cursor-default select-none hover:brightness-50": isShown}
      )}>
        <img onClick={() => !isShown && countdown > 0 && handleChoice(1)} className="object-cover size-full" src={pic1} alt="" />
      </div>
      <div className={cn("brightness-50 hover:brightness-[.4] transition duration-300 cursor-pointer",
        {"cursor-default select-none hover:brightness-50": isShown}
      )}>
        <img onClick={() => !isShown && countdown > 0 && handleChoice(2)} className="object-cover size-full" src={pic2}alt="" />
      </div>
      <div className="fixed top-16 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="text-4xl bg-white/90 text-black rounded-xl px-4 py-1 font-bold flex items-center justify-center">
          <span>Which of two is higher in {unit} ?</span>
        </div>
      </div>
      <div className="fixed bottom-4 left-28 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="text-xl bg-white/90 text-black rounded-xl px-4 py-1 font-bold flex items-center justify-center">
          <span>Username: {username}</span>
        </div>
      </div>
      <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="relative size-32 rounded-full bg-white/90 flex justify-center items-center">
          <svg className="absolute w-full h-full">
            <circle
              className="text-sky-300"
              stroke="currentColor"
              strokeWidth="7"
              fill="transparent"
              r="60"
              cx="50%"
              cy="50%"
              style={{ strokeDasharray: circumference, strokeDashoffset: offset, transform: 'rotate(-90deg)', transformOrigin: '50% 50%' }}
            />
          </svg>
          <div className="text-slate-900 text-3xl font-bold">{Math.floor(countdown / 10)}</div>
        </div>
        <div className="text-2xl bg-white/90 text-black rounded-xl py-1 font-bold flex items-center justify-center">
          <span>Score: {score}</span>
        </div>
      </div>
      <div className="fixed top-1/2 left-1/4 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-20 text-center">
        <div className="text-4xl font-bold text-white">
          "{name1}"
        </div>
        <div className={cn("flex flex-col gap-4",
          {"hidden":!isShown}
        )}>
          <div className="text-5xl font-bold text-yellow-200">
            {animatedNumber1.toLocaleString()}
          </div>
          <div className="text-xl font-bold text-white">
            {unit}
          </div>
        </div>
      </div>
      <div className="fixed top-1/2 left-3/4 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-20 text-center">
        <div className="text-4xl font-bold text-white">
            "{name2}"
        </div>
        <div className={cn("flex flex-col gap-4",
          {"hidden":!isShown}
        )}>
          <div className="text-5xl font-bold text-yellow-200">
            {animatedNumber2.toLocaleString()}
          </div>
          <div className="text-xl font-bold text-white">
            {unit}
          </div>
        </div>
      </div>
    </div>
  )
}