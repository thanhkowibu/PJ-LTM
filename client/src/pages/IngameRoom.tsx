import { cn } from "@/lib/utils"
import { useEffect, useState } from "react";
import axios from 'axios';
import { useNavigate, useParams } from "react-router-dom";
import toast from "react-hot-toast";

const BASE_URL = import.meta.env.VITE_SERVER_URL

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
  const [eventSource, setEventSource] = useState<EventSource | null>(null);
  const [questionIndex, setQuestionIndex] = useState(0);
  const [streak, setStreak] = useState(0);

  const username = localStorage.getItem("username")

  // animation
  const [countdown, setCountdown] = useState(200); // 200 for 20 seconds with 0.1s interval
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
    if (countdown <= 0) {
      setIsShown(true);
    }
  }, [countdown]);

  // game logic
  const fetchData = async () => {
    try {
      const response = await axios.post(`${BASE_URL}/game`, { room_name: id }, {
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true // Ensure cookies are sent with the request
      });
      setName1(response.data.name1);
      setName2(response.data.name2);
      setPic1(response.data.pic1);
      setPic2(response.data.pic2);
      setUnit(response.data.unit);
      
    } catch (error) {
      console.error(error);
    }
  };
    // Start Server-Sent Events (SSE)
    const startSSE = () => {
      if (eventSource) {
        return;
      }

      const es = new EventSource(`${BASE_URL}/subscribe`);
      setEventSource(es);
    
      es.onmessage = (event) => {
        // console.log("SSE event data:", event.data);
        const data = JSON.parse(event.data);
        if (data.action === "update" && data.room_name === id) {
          if (data.remain_time < 0) setCountdown(0)
          else setCountdown(data.remain_time * 10); // Update countdown with remaining time
          if (data.question_index !== questionIndex) {
            console.log("sv indx: ",data.question_index," ,cli idx: ",questionIndex)
            setQuestionIndex(data.question_index);
            fetchData();
            setIsShown(false);
          }
        } else if (data.action === "score_update" && data.room_name === id) {
          if (data.username === username) {
            setScore(data.score);
            setStreak(data.streak);
            setValue1(data.value1);
            setValue2(data.value2);
          }
        } else if (data.action === "finish" && data.room_name === id) {
          console.log("Navigating to /result/1 due to Finish event");
          es.close();
          navigate(`/result/${id}`);
        }
      };
    
      es.onerror = () => {
        console.error('SSE connection error. Reconnecting...');
        es.close();
    
        // Retry connection after a delay
        setTimeout(startSSE, 5000);
      };
    };

    useEffect(() => {
      fetchData();
      startSSE();
  
      return () => {
        if (eventSource) {
          eventSource.close();
          console.log("EventSource closed in IngameRoom");
        }
      };
    }, []);

  const handleChoice = async (choice: number) => {
    if (countdown > 0) {
      setIsShown(true);
      // console.log(choice);
      try {
        const remainingTime = Math.floor(countdown / 10);
        const response = await axios.post(`${BASE_URL}/game/choice`, {
          choice,
          room_name: id,
          remaining_time: remainingTime
        }, {
          headers: {
            'Content-Type': 'application/json',
          },
          withCredentials: true
        });
        console.log(response.data);
        if (response.data.base_score === 1000) {
          toast.success(`Bonus + ${response.data.bonus}`);
          toast.success(`Correct + ${response.data.base_score}`);
        } else {
          toast.error(`Incorrect + ${response.data.base_score}`);
        }
        setScore((pv) => pv + response.data.total_score);
        setStreak(response.data.streak);
        setValue1(response.data.value1);
        setValue2(response.data.value2);
        setCountdown(response.data.remaining_time * 10); // Update countdown with remaining time
      } catch (error) {
        console.error(error);
      }
    }
  };

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
      <div className="fixed bottom-4 right-8 transform -translate-y-1/2 flex flex-col gap-4">
        <div className="text-xl bg-white/90 text-black rounded-xl px-4 py-1 font-bold flex items-center justify-center">
          <span>Streak: {streak}🔥</span>
        </div>
      </div>
      <div className="fixed top-14 left-24 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="text-xl bg-white/90 text-black rounded-xl px-4 py-1 font-bold flex items-center justify-center">
          <span>Question {questionIndex+1}</span>
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
        <div className="text-2xl bg-white/90 text-black rounded-xl px-2 py-1 font-bold flex items-center justify-center">
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
      {/* Add the container for the three circle divs */}
      <div className="fixed bottom-8 left-1/2 transform -translate-x-1/2 flex gap-4">
        <div className="size-16 bg-white rounded-full"></div>
        <div className="size-16 bg-white rounded-full"></div>
        <div className="size-16 bg-white rounded-full"></div>
      </div>
    </div>
  )
}