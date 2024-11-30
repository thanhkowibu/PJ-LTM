import { cn } from "@/lib/utils"
import { useEffect, useState } from "react";

export const IngameRoom = () => {
  const [isShown, setIsShown] = useState(false);
  const [countdown, setCountdown] = useState(200); // 200 for 20 seconds with 0.1s interval
  const [isTimerRunning, setIsTimerRunning] = useState(true);
  const [animatedNumber1, setAnimatedNumber1] = useState(0);
  const [animatedNumber2, setAnimatedNumber2] = useState(0);
  const [endNumber1, setEndNumber1] = useState(1000000);
  const [endNumber2, setEndNumber2] = useState(997000);

  useEffect(() => {
    if (isShown) {
      const duration = 1500;
      const startTime = performance.now();

      const animate1 = (currentTime: number) => {
        const elapsedTime = currentTime - startTime;
        const progress = Math.min(elapsedTime / duration, 1);
        const currentNumber = Math.floor(progress * endNumber1);
        setAnimatedNumber1(currentNumber);

        if (progress < 1) {
          requestAnimationFrame(animate1);
        }
      };

      const animate2 = (currentTime: number) => {
        const elapsedTime = currentTime - startTime;
        const progress = Math.min(elapsedTime / duration, 1);
        const currentNumber = Math.floor(progress * endNumber2);
        setAnimatedNumber2(currentNumber);

        if (progress < 1) {
          requestAnimationFrame(animate2);
        }
      };

      requestAnimationFrame(animate1);
      requestAnimationFrame(animate2);
    }
  }, [isShown, endNumber1, endNumber2]);

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

  const handleChoice = async (choice: number) => {
    if (countdown > 0) {
      setIsShown(true);
      setIsTimerRunning(false);
      console.log(choice);
    }
  }

  const circumference = 2 * Math.PI * 60; // 60 is the radius of the outer circle
  const offset = circumference - (countdown / 200) * circumference;

  return (
    <div className="h-dvh grid grid-cols-2 relative">
      <div className={cn("brightness-50 hover:brightness-[.4] transition duration-300 cursor-pointer",
        {"cursor-default select-none hover:brightness-50": isShown}
      )}>
        <img onClick={() => !isShown && countdown > 0 && handleChoice(1)} className="object-cover size-full" src="https://wallpaper.forfun.com/fetch/32/32149d4d96e53453c26d8b1fed07094f.jpeg" alt="" />
      </div>
      <div className={cn("brightness-50 hover:brightness-[.4] transition duration-300 cursor-pointer",
        {"cursor-default select-none hover:brightness-50": isShown}
      )}>
        <img onClick={() => !isShown && countdown > 0 && handleChoice(2)} className="object-cover size-full" src="https://images4.alphacoders.com/130/1307264.jpg" alt="" />
      </div>
      <div className="fixed top-16 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div className="text-4xl bg-white/90 text-black rounded-xl px-4 py-1 font-bold flex items-center justify-center">
          <span>Which of two has more subscribers ?</span>
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
          <span>Score: 2</span>
        </div>
      </div>
      <div className="fixed top-1/2 left-1/4 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-20 text-center">
        <div className="text-4xl font-bold text-white">
          "Hoshimachi Suisei"
        </div>
        <div className={cn("flex flex-col gap-4",
          {"hidden":!isShown}
        )}>
          <div className="text-5xl font-bold text-yellow-200">
            {animatedNumber1.toLocaleString()}
          </div>
          <div className="text-xl font-bold text-white">
            subscribers
          </div>
        </div>
      </div>
      <div className="fixed top-1/2 left-3/4 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-20 text-center">
        <div className="text-4xl font-bold text-white">
            "Sakura Miko"
        </div>
        <div className={cn("flex flex-col gap-4",
          {"hidden":!isShown}
        )}>
          <div className="text-5xl font-bold text-yellow-200">
            {animatedNumber2.toLocaleString()}
          </div>
          <div className="text-xl font-bold text-white">
            subscribers
          </div>
        </div>
      </div>
    </div>
  )
}