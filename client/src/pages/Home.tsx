import { useState, useEffect } from 'react';
import axios from 'axios';
import Login from '@/modals/Login';
import Register from '@/modals/Register';
import toast from 'react-hot-toast';

const BASE_URL = "http://127.0.0.1:8080/api"

export const Home = () => {
  const [isLoginOpen, setIsLoginOpen] = useState(false);
  const [isRegisterOpen, setIsRegisterOpen] = useState(false);
  const [isRoomListOpen, setIsRoomListOpen] = useState(false);
  const [loading, setLoading] = useState(false);

  const SERVER_URL = window.location.hostname === 'localhost'
    ? import.meta.env.VITE_LOCAL_SERVER_URL
    : import.meta.env.VITE_GUEST_SERVER_URL;

  const username = localStorage.getItem("username")

  const handleLogout = async () => {
    if (loading) return;

    setLoading(true);
    try {
      const res = await axios.get(`${BASE_URL}/auth/logout`, {
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true
      });
      console.log(res.data);
      if (res.data.message){
        toast.success(res.data.message);
      }
      if (res.data.status === "success"){
        localStorage.removeItem("username")
      }
    } catch (err: any) {
      console.log(err)
      if (err.message){
        toast.error(err.message);
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="h-dvh relative">
      <div className="size-full brightness-[.25] transition duration-300">
        <img className="object-cover size-full" src="/result.jpg" alt="result" />
      </div>
      <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-20 items-center">
        <div className="text-5xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
          <span>Welcome to</span>
        </div>
        <div className='h-52'>
          <img className="object-cover size-full" src="/cover.png" alt="cover" />
        </div>
        <div className="flex gap-32 items-center text-2xl font-semibold text-white text-center">
          <button className='px-8 py-4 rounded-full border-4 hover:bg-white hover:text-black transition duration-300'>
            Join room
          </button>
          <button className='px-8 py-4 rounded-full border-4 hover:bg-white hover:text-black transition duration-300'>
            Create room
          </button>
        </div>
      </div>
      <div className="fixed top-8 right-6 flex gap-8 text-white rounded-xl px-4 py-1">
        <div className="text-xl font-semibold underline underline-offset-4 flex items-center justify-center">
          <span>Welcome,   {username ? username : "guest"}</span>
        </div>
        {!username ? <>
          <button onClick={() => setIsLoginOpen(true)} className='text-lg font-semibold px-4 py-1 rounded-full border-2 hover:bg-white hover:text-black transition duration-300'>
            Login
          </button>
          <button onClick={() => setIsRegisterOpen(true)} className='text-lg font-semibold px-4 py-1 rounded-full border-2 hover:bg-white hover:text-black transition duration-300'>
            Register
          </button>
        </> : <>
          <button onClick={handleLogout} className='text-lg font-semibold px-4 py-1 rounded-full border-2 hover:bg-white hover:text-black transition duration-300'>
            Logout
          </button>
        </>}
      </div>
      <Login
        isOpen={isLoginOpen}
        setIsOpen={setIsLoginOpen}
        setOtherOpen={setIsRegisterOpen}
      />
      <Register
        isOpen={isRegisterOpen}
        setIsOpen={setIsRegisterOpen}
        setOtherOpen={setIsLoginOpen}
      />
    </div>
  );
};